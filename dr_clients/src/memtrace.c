#include "memtrace.h"
#include <string.h> /* for memset */
#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "utilities.h"
#include "moduleinfo.h"
#include "defines.h"

/*************************defines******************************/

#ifdef WINDOWS
# define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
# define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#define NULL_TERMINATE(buf) buf[(sizeof(buf)/sizeof(buf[0])) - 1] = '\0'

/*stack demarcation only for 32 bit applications ??? */

/* Control the format of memory trace: readable or hexl */
#define READABLE_TRACE
/* Max number of mem_ref a buffer can have */
#define MAX_NUM_MEM_REFS 8192
/* The size of memory buffer for holding mem_refs. When it fills up,
 * we dump data from the buffer to the file.
 */
#define MEM_BUF_SIZE (sizeof(mem_ref_t) * MAX_NUM_MEM_REFS)

/************************typedefs***********************************/

/* Each mem_ref_t includes the type of reference (read or write),
* the address referenced, and the size of the reference.
*/
typedef struct _mem_ref_t {
	bool  write;
	void *addr;
	size_t size;
	app_pc pc;
} mem_ref_t;


/* thread private log file and counter */
typedef struct {
    char   *buf_ptr;
    char   *buf_base;
    /* buf_end holds the negative value of real address of buffer end. */
    ptr_int_t buf_end;
    void   *cache;
    file_t  logfile;
	file_t  outfile;
    uint64  num_refs;

	uint stack_base;
	uint stack_limit;

} per_thread_t;

typedef struct _client_arg_t{
	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
	char output_folder[MAX_STRING_LENGTH];

} client_arg_t;

/***************************global variables**********************/

static client_id_t client_id;
static app_pc code_cache;
static void  *mutex;    /* for multithread support */
static uint64 num_refs; /* keep a global memory reference count */
static int tls_index;

static client_arg_t * client_arg;
static module_t * head;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];

/**********************function prototypes***********************/

static void clean_call(void);
static void memtrace(void *drcontext);
static void code_cache_init(void);
static void code_cache_exit(void);
static void instrument_mem(void        *drcontext,
                           instrlist_t *ilist,
                           instr_t     *where,
                           int          pos,
                           bool         write);
static bool parse_commandline_args (const char * args);


/*********************function implementation*******************/

static bool parse_commandline_args (const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));
	if(dr_sscanf(args,"%s %d %s",&client_arg->filter_filename,
								&client_arg->filter_mode,
								&client_arg->output_folder)!=3){
		return false;
	}
	
	return true;
}

void memtrace_init(client_id_t id,const char * name, const char * arguments)
{

	char logfilename[MAX_STRING_LENGTH];
	file_t in_file;

	DEBUG_PRINT("memtrace - initializing\n");

	drmgr_init();
    drutil_init();

    client_id = id;
    mutex = dr_mutex_create();

	DR_ASSERT(parse_commandline_args(arguments) == true);
	
	head = md_initialize();


	if(client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename,DR_FILE_READ);
		DR_ASSERT(in_file != INVALID_FILE);
		md_read_from_file(head,in_file,false);
		dr_close_file(in_file);
	}
    
    tls_index = drmgr_register_tls_field();
    DR_ASSERT(tls_index != -1);

    code_cache_init();

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);

	DEBUG_PRINT("memtrace - initialized\n");
}

void memtrace_exit_event()
{

	md_delete_list(head,false);
    code_cache_exit();
    drmgr_unregister_tls_field(tls_index);
	if (log_mode){
		dr_close_file(logfile);
	}
    dr_mutex_destroy(mutex);
    drutil_exit();
    drmgr_exit();
}


void memtrace_thread_init(void *drcontext)
{
    char logfilename[MAX_STRING_LENGTH];
	char outfilename[MAX_STRING_LENGTH];
	char thread_id[MAX_STRING_LENGTH];

    char *dirsep;
    int len;
    per_thread_t *data;

	uint * stack_base;
	uint * deallocation_stack;

	int i = 0;

    /* allocate thread private data */
    data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
    drmgr_set_tls_field(drcontext, tls_index, data);
    data->buf_base = dr_thread_alloc(drcontext, MEM_BUF_SIZE);
    data->buf_ptr  = data->buf_base;
    /* set buf_end to be negative of address of buffer end for the lea later */
    data->buf_end  = -(ptr_int_t)(data->buf_base + MEM_BUF_SIZE);
    data->num_refs = 0;

    /* We're going to dump our data to a per-thread file.
     * On Windows we need an absolute path so we place it in
     * the same directory as our library. We could also pass
     * in a path and retrieve with dr_get_options().
     */
	if (log_mode){
		dr_snprintf(thread_id, MAX_STRING_LENGTH, "%d", dr_get_thread_id(drcontext));
		populate_conv_filename(logfilename, logdir, ins_pass_name, thread_id);
		data->logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
	}

	populate_conv_filename(outfilename, client_arg->output_folder, ins_pass_name, thread_id);
	data->outfile = dr_open_file(outfilename, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
	DR_ASSERT(data->outfile != INVALID_FILE);

	/* this is done for 32 bit applications */
	deallocation_stack = &data->stack_limit;
	stack_base = &data->stack_base;

	__asm{
			mov EAX, FS : [0x04]
			mov EBX, stack_base
			mov[EBX], EAX
			mov EAX, FS : [0xE0C]
			mov EBX, deallocation_stack
			mov[EBX], EAX
	}

	DEBUG_PRINT("stack boundaries - %x,%x\n", data->stack_base, data->stack_limit);


}


void memtrace_thread_exit(void *drcontext)
{
    per_thread_t *data;

    memtrace(drcontext);
    data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mutex_lock(mutex);
    num_refs += data->num_refs;
    dr_mutex_unlock(mutex);
	if (log_mode){
		dr_close_file(data->logfile);
	}
	dr_close_file(data->outfile);
    dr_thread_free(drcontext, data->buf_base, MEM_BUF_SIZE);
    dr_thread_free(drcontext, data, sizeof(per_thread_t));
}


/* we transform string loops into regular loops so we can more easily
 * monitor every memory reference they make
 */
dr_emit_flags_t
memtrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating)
{
    
    return DR_EMIT_DEFAULT;
}

/* our operations here only need to see a single-instruction window so
 * we do not need to do any whole-bb analysis
 */
dr_emit_flags_t
memtrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data)
{
    return DR_EMIT_DEFAULT;
}

/* event_bb_insert calls instrument_mem to instrument every
 * application memory reference.
 */
dr_emit_flags_t
memtrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data)
{
    int i;
	reg_id_t reg;
	file_t out_file;

	DR_ASSERT(instr_ok_to_mangle(instr));

	if (instr_ok_to_mangle(instr)){
		if (filter_from_list(head, instr, client_arg->filter_mode)){


			if (instr_reads_memory(instr)) {
				for (i = 0; i < instr_num_srcs(instr); i++) {
					if (opnd_is_memory_reference(instr_get_src(instr, i))) {
						instrument_mem(drcontext, bb, instr, i, false);
					}
				}
			}
			if (instr_writes_memory(instr)) {
				for (i = 0; i < instr_num_dsts(instr); i++) {
					if (opnd_is_memory_reference(instr_get_dst(instr, i))) {
						instrument_mem(drcontext, bb, instr, i, true);
					}
				}
			}

		}
	}

    return DR_EMIT_DEFAULT;
}


static void
memtrace(void *drcontext)
{
    per_thread_t *data;
    int num_refs;
    mem_ref_t *mem_ref;
#ifdef READABLE_TRACE
    int i;
#endif

	module_data_t * mdata;

    data      = drmgr_get_tls_field(drcontext, tls_index);
    mem_ref   = (mem_ref_t *)data->buf_base;
    num_refs  = (int)((mem_ref_t *)data->buf_ptr - mem_ref);

#ifdef READABLE_TRACE
    /*dr_fprintf(data->log,
               "Format: <instr address>,<(r)ead/(w)rite>,<data size>,<data address>\n");*/

	for (i = 0; i < num_refs; i++) {
		mdata = dr_lookup_module(mem_ref->pc);
		if (mdata != NULL){
			if (((uint)mem_ref->addr > data->stack_base) || ((uint)mem_ref->addr < data->stack_limit)){
				dr_fprintf(data->outfile, "%x,%x,%d,%d,"PFX"\n", mdata->start, mem_ref->pc - mdata->start
					, mem_ref->write ? 1 : 0 , mem_ref->size, mem_ref->addr);
			}
			dr_free_module_data(mdata);
		}
			
		++mem_ref;
	}

#else
    dr_write_file(data->log, data->buf_base,
                  (size_t)(data->buf_ptr - data->buf_base));
#endif

    memset(data->buf_base, 0, MEM_BUF_SIZE);
    data->num_refs += num_refs;
    data->buf_ptr   = data->buf_base;
}

/* clean_call dumps the memory reference info to the log file */
static void
clean_call(void)
{
    void *drcontext = dr_get_current_drcontext();
    memtrace(drcontext);
}

static void
code_cache_init(void)
{
    void         *drcontext;
    instrlist_t  *ilist;
    instr_t      *where;
    byte         *end;

    drcontext  = dr_get_current_drcontext();
    code_cache = dr_nonheap_alloc(PAGE_SIZE,
                                  DR_MEMPROT_READ  |
                                  DR_MEMPROT_WRITE |
                                  DR_MEMPROT_EXEC);
    ilist = instrlist_create(drcontext);
    /* The lean procecure simply performs a clean call, and then jump back */
    /* jump back to the DR's code cache */
    where = INSTR_CREATE_jmp_ind(drcontext, opnd_create_reg(DR_REG_XCX));
    instrlist_meta_append(ilist, where);
    /* clean call */
    dr_insert_clean_call(drcontext, ilist, where, (void *)clean_call, false, 0);
    /* Encodes the instructions into memory and then cleans up. */
    end = instrlist_encode(drcontext, ilist, code_cache, false);
    DR_ASSERT((end - code_cache) < PAGE_SIZE);
    instrlist_clear_and_destroy(drcontext, ilist);
    /* set the memory as just +rx now */
    dr_memory_protect(code_cache, PAGE_SIZE, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
}


static void
code_cache_exit(void)
{
    dr_nonheap_free(code_cache, PAGE_SIZE);
}


/*
 * instrument_mem is called whenever a memory reference is identified.
 * It inserts code before the memory reference to to fill the memory buffer
 * and jump to our own code cache to call the clean_call when the buffer is full.
 */
static void
instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where,
               int pos, bool write)
{
    instr_t *instr, *call, *restore, *first, *second;
    opnd_t   ref, opnd1, opnd2;
    reg_id_t reg1 = DR_REG_XBX; /* We can optimize it by picking dead reg */
    reg_id_t reg2 = DR_REG_XCX; /* reg2 must be ECX or RCX for jecxz */
    per_thread_t *data;
    app_pc pc;

    data = drmgr_get_tls_field(drcontext, tls_index);

    /* Steal the register for memory reference address *
     * We can optimize away the unnecessary register save and restore
     * by analyzing the code and finding the register is dead.
     */
    dr_save_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
    dr_save_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);

    if (write)
       ref = instr_get_dst(where, pos);
    else
       ref = instr_get_src(where, pos);

    /* use drutil to get mem address */
    drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);

    /* The following assembly performs the following instructions
     * buf_ptr->write = write;
     * buf_ptr->addr  = addr;
     * buf_ptr->size  = size;
     * buf_ptr->pc    = pc;
     * buf_ptr++;
     * if (buf_ptr >= buf_end_ptr)
     *    clean_call();
     */
    drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);
    /* Load data->buf_ptr into reg2 */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
    instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* Move write/read to write field */
    opnd1 = OPND_CREATE_MEM32(reg2, offsetof(mem_ref_t, write));
    opnd2 = OPND_CREATE_INT32(write);
    instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* Store address in memory ref */
    opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, addr));
    opnd2 = opnd_create_reg(reg1);
    instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* Store size in memory ref */
    opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, size));
    /* drutil_opnd_mem_size_in_bytes handles OP_enter */
    opnd2 = OPND_CREATE_INT32(drutil_opnd_mem_size_in_bytes(ref, where));
    instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* Store pc in memory ref */
    pc = instr_get_app_pc(where);
    /* For 64-bit, we can't use a 64-bit immediate so we split pc into two halves.
     * We could alternatively load it into reg1 and then store reg1.
     * We use a convenience routine that does the two-step store for us.
     */
    opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(mem_ref_t, pc));
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t) pc, opnd1,
                                     ilist, where, &first, &second);
    instr_set_ok_to_mangle(first, false/*meta*/);
    if (second != NULL)
        instr_set_ok_to_mangle(second, false/*meta*/);

    /* Increment reg value by pointer size using lea instr */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = opnd_create_base_disp(reg2, DR_REG_NULL, 0,
                                  sizeof(mem_ref_t),
                                  OPSZ_lea);
    instr = INSTR_CREATE_lea(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* Update the data->buf_ptr */
    drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg1);
    opnd1 = OPND_CREATE_MEMPTR(reg1, offsetof(per_thread_t, buf_ptr));
    opnd2 = opnd_create_reg(reg2);
    instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* we use lea + jecxz trick for better performance
     * lea and jecxz won't disturb the eflags, so we won't insert
     * code to save and restore application's eflags.
     */
    /* lea [reg2 - buf_end] => reg2 */
    opnd1 = opnd_create_reg(reg1);
    opnd2 = OPND_CREATE_MEMPTR(reg1, offsetof(per_thread_t, buf_end));
    instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);
    opnd1 = opnd_create_reg(reg2);
    opnd2 = opnd_create_base_disp(reg1, reg2, 1, 0, OPSZ_lea);
    instr = INSTR_CREATE_lea(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

    /* jecxz call */
    call  = INSTR_CREATE_label(drcontext);
    opnd1 = opnd_create_instr(call);
    instr = INSTR_CREATE_jecxz(drcontext, opnd1);
    instrlist_meta_preinsert(ilist, where, instr);

    /* jump restore to skip clean call */
    restore = INSTR_CREATE_label(drcontext);
    opnd1 = opnd_create_instr(restore);
    instr = INSTR_CREATE_jmp(drcontext, opnd1);
    instrlist_meta_preinsert(ilist, where, instr);

    /* clean call */
    /* We jump to lean procedure which performs full context switch and
     * clean call invocation. This is to reduce the code cache size.
     */
    instrlist_meta_preinsert(ilist, where, call);
    /* mov restore DR_REG_XCX */
    opnd1 = opnd_create_reg(reg2);
    /* this is the return address for jumping back from lean procedure */
    opnd2 = opnd_create_instr(restore);
    /* We could use instrlist_insert_mov_instr_addr(), but with a register
     * destination we know we can use a 64-bit immediate.
     */
    instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);
    /* jmp code_cache */
    opnd1 = opnd_create_pc(code_cache);
    instr = INSTR_CREATE_jmp(drcontext, opnd1);
    instrlist_meta_preinsert(ilist, where, instr);

    /* restore %reg */
    instrlist_meta_preinsert(ilist, where, restore);
    dr_restore_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
    dr_restore_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);
}






