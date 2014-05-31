#include <string.h> /* for memset */
#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "defines.h"
#include "utilities.h"

#ifdef WINDOWS
# define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
# define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#ifdef WINDOWS
# define IF_WINDOWS(x) x
#else
# define IF_WINDOWS(x) /* nothing */
#endif

#define NULL_TERMINATE(buf) buf[(sizeof(buf)/sizeof(buf[0])) - 1] = '\0'

/* modes for filtering the instructions */
#define NO_FILTERING			1
#define PROFILED_FILTERING		2
#define MODULE_FILTERING		3


/*
	destination and source encoding
	
	seperate field to signify reg or mem

	reg (type & 0b11 = 0)
	---
	eax,...,edi - set according to the reg number (reg_id I think)
	register width - is encoded in the type information
	8 bit high - 8th bit high
	8 bit low - 7th bit high
	16 bit - 6th bit high
	32 bit - 5th bit high
	64 bit - 4th bit high

	mem  (type & 0b11 = 1)
	---
	absolute mem address (virtual) - assuming we are only tracking one process's dependancies - aliasing??
	memory width - will be stored in the location position -> actual location will not be updated statically.
	mem_addr will not be stored statically, but will be carried dynamically in instr_trace_t data structure

	immediate  (type & 0b11 = 2)
	---------
	the location field will have the value of it

*/

#define WIDTH_8L				(1<<7)
#define WIDTH_8H				(1<<6)
#define WIDTH_16				(1<<5)
#define WIDTH_32				(1<<4)
#define WIDTH_64				(1<<3)

#define GET_TYPE(x)				( x & 0x3 )
#define SET_TYPE(x,type)		x = ( x | type ) 
#define REG_TYPE				0
#define MEM_TYPE				1
#define IMM_TYPE				2

typedef struct _operand_t {

	unsigned char type;
	unsigned char location;

} operand_t;

/*
	destination struct - contains the operands as well as the operation performed on the value stored at this destination.
*/

typedef struct _dst_t {
	
	operand_t opnd;
	unsigned short operation;

} dst_t;

typedef operand_t src_t;

#define MAX_SRCS 2

/*
 info that can be statically determined - will assume a canonical form 2/1 srcs and 1 dest
*/

typedef struct _static_info_t {
    
	unsigned char src_num;
	unsigned char dest_num;
	src_t src[MAX_SRCS];
	dst_t dst;
	app_pc pc;

} static_info_t;

/* 
	instrace main structure  
*/

typedef struct _instr_trace_t {

	static_info_t * static_info_instr;
	uint mem_addr;						//mem_addr if it has a memory reference
	uint size;

} instr_trace_t;


/* Control the format of memory trace: readable or hexl */
#define READABLE_TRACE
/* Max number of mem_ref a buffer can have */
#define MAX_NUM_INSTR_TRACES 8192
/* The size of memory buffer for holding mem_refs. When it fills up,
 * we dump data from the buffer to the file.
 */
#define INSTR_BUF_SIZE (sizeof(instr_trace_t) * MAX_NUM_INSTR_TRACES)

/* thread private log file and counter */
typedef struct {
    char   *buf_ptr;
    char   *buf_base;
    /* buf_end holds the negative value of real address of buffer end. */
    ptr_int_t buf_end;
    void   *cache;
    file_t  log;
    uint64  num_refs;
} per_thread_t;


/* main global structure that will carry all the static information about the instruction */
static static_info_t * global_static_info;
static uint global_static_ptr;
static uint global_static_array_size;

static client_id_t client_id;
static app_pc code_cache;
static void  *mutex;    /* for multithread support */
static uint64 num_refs; /* keep a global memory reference count */
static int tls_index;

typedef struct _client_arg_t {

	uint filter_mode;
	char folder[MAX_STRING_LENGTH];
	char in_filename[MAX_STRING_LENGTH];
	uint static_info_size;

} client_arg_t;

static client_arg_t * client_arg;
static module_t * head;

static void clean_call(void);
static void print_trace(void *drcontext);
static void code_cache_init(void);
static void code_cache_exit(void);
static static_info_t * static_info_instrumentation(void * drcontext, instr_t* instr, unsigned char * done, uint * phase,
								uint * dest_no, uint * src_no);
static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               static_info_t * static_info, uint * dest_no, uint * src_no);
static void debug_static_info(void * drcontext,instr_t * instr);
bool parse_commandline_args (const char * args);

/* function implementation */

bool parse_commandline_args (const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));
	if(dr_sscanf(args,"%d %s %s %d",
									   &client_arg->filter_mode,
									   &client_arg->folder,
									   &client_arg->in_filename,
									   &client_arg->static_info_size)!=4){
		return false;
	}

	dr_printf("enter %s\n",args);
	dr_printf("%d %s %s %d\n",client_arg->filter_mode,client_arg->folder,client_arg->in_filename,client_arg->static_info_size);
	dr_printf("exit\n",args);

	return true;
}

void instrace_init(client_id_t id, char * arguments)
{

	file_t in_file;

    drmgr_init();
    drutil_init();
    client_id = id;

	DR_ASSERT(parse_commandline_args(arguments)==true);

	head = md_initialize();
	if(client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->in_filename,DR_FILE_READ);
		md_read_from_file(head,in_file,false);
		dr_close_file(in_file);
	}

	global_static_info = (static_info_t * )dr_global_alloc(sizeof(static_info_t)*client_arg->static_info_size);
	global_static_array_size = client_arg->static_info_size;
	global_static_ptr = 0;

	mutex = dr_mutex_create();
    tls_index = drmgr_register_tls_field();
    DR_ASSERT(tls_index != -1);
    code_cache_init();

}

void instrace_exit_event()
{
	dr_global_free(global_static_info,sizeof(static_info_t)*global_static_array_size);
	md_delete_list(head);
	dr_global_free(client_arg,sizeof(client_arg_t));
    code_cache_exit();
    drmgr_unregister_tls_field(tls_index);
    dr_mutex_destroy(mutex);
    drutil_exit();
    drmgr_exit();
}

void instrace_thread_init(void *drcontext)
{
    char logname[MAXIMUM_PATH];
    char *dirsep;
    int len;
    per_thread_t *data;

    /* allocate thread private data */
    data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
    drmgr_set_tls_field(drcontext, tls_index, data);
    data->buf_base = dr_thread_alloc(drcontext, INSTR_BUF_SIZE);
    data->buf_ptr  = data->buf_base;
    /* set buf_end to be negative of address of buffer end for the lea later */
	data->buf_end  = -(ptr_int_t)(data->buf_base + INSTR_BUF_SIZE);
    data->num_refs = 0;

    /* We're going to dump our data to a per-thread file.
     * On Windows we need an absolute path so we place it in
     * the same directory as our library. We could also pass
     * in a path and retrieve with dr_get_options().
     */
    len = dr_snprintf(logname, sizeof(logname)/sizeof(logname[0]),
		"%s", client_arg->folder);
	dr_printf("%d\n",len);
    DR_ASSERT(len > 0);
    len += dr_snprintf(logname + len,
                      (sizeof(logname) - len)/sizeof(logname[0]),
                      "instrace.%d.log", dr_get_thread_id(drcontext));
    dr_printf("%d\n",len);
	DR_ASSERT(len > 0);
    logname[len] = '\0';
	dr_printf("name - %s\n",logname);
    data->log = dr_open_file(logname,
                             DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(data->log != INVALID_FILE);

}


void
instrace_thread_exit(void *drcontext)
{
    per_thread_t *data;

    print_trace(drcontext);
    data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mutex_lock(mutex);
    num_refs += data->num_refs;
    dr_mutex_unlock(mutex);
    dr_close_file(data->log);
    dr_thread_free(drcontext, data->buf_base, INSTR_BUF_SIZE);
    dr_thread_free(drcontext, data, sizeof(per_thread_t));
}


/* we transform string loops into regular loops so we can more easily
 * monitor every memory reference they make
 */
dr_emit_flags_t
instrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating)
{
    if (!drutil_expand_rep_string(drcontext, bb)) {
        DR_ASSERT(false);
        /* in release build, carry on: we'll just miss per-iter refs */
    }
    return DR_EMIT_DEFAULT;
}

/* our operations here only need to see a single-instruction window so
 * we do not need to do any whole-bb analysis
 */
dr_emit_flags_t
instrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data)
{
    return DR_EMIT_DEFAULT;
}

/* event_bb_insert calls necessary functions to fill up the static info about an instruction and 
   to instrument memory instructions to get its runtime address using dynamic instrumentation
 */
dr_emit_flags_t
instrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data)
{
   /* algorithm
	  1. First we need to filter the instructions to instrument - call should instrument
	  2. call the static info filler function to fill the instruction data and get the ptr to filled data
      3. send the data appropriately to instrumentation function
	*/

	static_info_t * instr_info;

	/* these are for the use of the caller */
	unsigned char * done;
	uint * phase;
	uint * dest_no;
	uint * src_no;

	done = (unsigned char *)dr_global_alloc(sizeof(unsigned char));
	phase = (uint *)dr_global_alloc(sizeof(uint));
	dest_no = (uint *)dr_global_alloc(sizeof(uint));
	src_no = (uint *)dr_global_alloc(sizeof(uint)*MAX_SRCS);

	DR_ASSERT(done != NULL);
	DR_ASSERT(phase != NULL);
	DR_ASSERT(dest_no != NULL);
	DR_ASSERT(src_no != NULL);

	*done = false;
	*phase = 0;

	if(filter_from_list(head,instrlist_first(bb),client_arg->filter_mode)){
		while(*done == false){
			instr_info = static_info_instrumentation(drcontext, instr, done, phase, dest_no, src_no);
			if(instr_info != NULL){ /* we may filter out the branch instructions */
				//dynamic_info_instrumentation(drcontext,bb,instr,instr_info, dest_no, src_no);		
			}
		}
	}

	dr_global_free(done,sizeof(unsigned char));
	dr_global_free(phase,sizeof(uint));
	dr_global_free(dest_no,sizeof(uint));
	dr_global_free(src_no,sizeof(uint)*MAX_SRCS);

	return DR_EMIT_DEFAULT;
			
}

static void debug_static_info(void * drcontext,instr_t * instr){

	char string_storage[MAX_STRING_LENGTH];
	opnd_t operand;
	per_thread_t * data;
	int i;
	
	data      = drmgr_get_tls_field(drcontext, tls_index);
	
	instr_disassemble_to_buffer(drcontext,instr,string_storage,MAX_STRING_LENGTH);
	dr_fprintf(data->log,"%s\n",string_storage);
	dr_fprintf(data->log,"srcs - %d\n",instr_num_srcs(instr));
	dr_fprintf(data->log,"dests - %d\n",instr_num_dsts(instr));

	for(i=0; i<instr_num_srcs(instr); i++){

		dr_fprintf(data->log,"srcs(%d)-",i);
		operand = instr_get_src(instr,i);
		if(opnd_is_immed_int(operand)){
			dr_fprintf(data->log,"imm-%d\n",opnd_get_immed_int(operand));
		}
		else if(opnd_is_immed_float(operand)){
			dr_fprintf(data->log,"imm-%f\n",opnd_get_immed_float(operand));
		}
		else if(opnd_is_memory_reference(operand)){
			dr_fprintf(data->log,"mem\n");
		}
		else if(opnd_is_reg(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(data->log,"reg-%s-width-%d\n",string_storage,opnd_size_in_bytes(reg_get_size(opnd_get_reg(operand))));
		}

	}

	for(i=0; i<instr_num_dsts(instr); i++){

		dr_fprintf(data->log,"dest(%d)-",i);
		operand = instr_get_dst(instr,i);
		if(opnd_is_immed_int(operand)){
			dr_fprintf(data->log,"imm-%d\n",opnd_get_immed_int(operand));
		}
		else if(opnd_is_immed_float(operand)){
			dr_fprintf(data->log,"imm-%f\n",opnd_get_immed_float(operand));
		}
		else if(opnd_is_memory_reference(operand)){
			dr_fprintf(data->log,"mem\n");
		}
		else if(opnd_is_reg(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(data->log,"reg-%s-width-%d\n",string_storage,opnd_size_in_bytes(reg_get_size(opnd_get_reg(operand))));
		}


	}

}

static static_info_t * static_info_instrumentation(void * drcontext, instr_t* instr, unsigned char * done, uint * phase,
								uint * dest_no, uint * src_no){
	/*
		for each src and dest add the information accordingly
		this should return canonicalized static info about an instruction; breaking down any complex instructions if necessary
	*/
	int i;
	static_info_t * static_info;
	opnd_t operand;
	reg_id_t reg;

	uint reg_size;

	/* acquire the global array ptr for storing static information for the given instruction */
	dr_mutex_lock(mutex);

	global_static_ptr++;
	DR_ASSERT(global_static_ptr < global_static_array_size);
	static_info = &global_static_info[global_static_ptr];

	dr_mutex_unlock(mutex);
	debug_static_info(drcontext,instr);

	*done = 1;
	return static_info;

	/* setup the srcs */ 
	//static_info->src_num = instr_num_srcs(instr);

	//DR_ASSERT(static_info->src_num <= 2);

	//dr_log(drcontext,LOG_ALL,1,"");

	/*for(i=0; i<instr_num_srcs(instr);i++){
		operand = instr_get_src(instr,i);
		if(opnd_is_reg(operand)){

	
			reg = opnd_get_reg(operand);
			reg_size = opnd_size_in_bytes(reg_get_size(reg));

			SET_TYPE(static_info->src[i].type,REG_TYPE);
			static_info->src[i].location = reg;
			switch(reg_size){
				case 1: 
					{
						if(reg <= DR_REG_BL){
							static_info->src[i].type |= WIDTH_8L;
						}
						else{
							static_info->src[i].type |= WIDTH_8H;
						}
					}
				case 2:
					static_info->src[i].type |= WIDTH_16;
				case 4:
					static_info->src[i].type |= WIDTH_32;
				case 8:
					static_info->src[i].type |= WIDTH_64;
				default:
			}

		}
		else if(opnd_is_immed(operand)){  //can be floating point operand or int operand
			SET_TYPE(static_info->src[i].type,IMM_TYPE);
			if(opnd_is_immed_int(operand)){
				static_info->src[i].location = opnd_get_immed_int(operand);
			}
			else if(opnd_is_immed_float(operand)){
				static_info->src[i].location = 0;
				//static_info->src[i].location = opnd_get_immed_float(operand);
			}
		}
		else if(opnd_is_memory_reference(operand)){
			SET_TYPE(static_info->src[i].type,MEM_TYPE);
			static_info->src[i].type |= (drutil_opnd_mem_size_in_bytes(operand,instr)) << 5;
		}
		else{
		}
	}*/

	/* setup the destinations - only a single destination should be processed 
	   we need to specialize based on the instructions for multi-destination instructions.
	   for single destination instruction we need not specialize */

	/* fill up assuming single destination or less */
	/*static_info->dest_num = 0;
	if(instr_num_dsts(instr) > 0){
		static_info->dest_num = 1;
		operand = instr_get_dst(instr,0);
	}
	static_info->dst.operation = instr_get_opcode(instr);*/ /* just use dr's enum encoding; we will keep using until it is ready to pretty print */


}

static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               static_info_t * static_info, uint * dest_no, uint * src_no)
{

	/*
		this function does the acutal instrumentation

		arguments - 

		we get a filled pointer here about the operand types for a given instruction (srcs and dests)
		1) increment the pointer to the instr_trace buffers
		2) add this pointer to instr_trace_t wrapper
		3) check whether any of the srcs and dests have memory operations; if so add a lea instruction and get the dynamic address
			Add this address to instr_trace_t structure
		4) if the buffer is full call a function to dump it to the file and restore the head ptr of the buffer 
			(lean function is used utilizing a code cache to limit code bloat needed for a clean call before every instruction.)
	*/

    instr_t *instr, *call, *restore, *first, *second;
    opnd_t   ref, opnd1, opnd2;
    reg_id_t reg1 = DR_REG_XBX; /* We can optimize it by picking dead reg */
    reg_id_t reg2 = DR_REG_XCX; /* reg2 must be ECX or RCX for jecxz */
    per_thread_t *data;
    app_pc pc;
	uint i;

    data = drmgr_get_tls_field(drcontext, tls_index);

    /* Steal the register for memory reference address *
     * We can optimize away the unnecessary register save and restore
     * by analyzing the code and finding the register is dead.
     */
    dr_save_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
    dr_save_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);

	/* check whether this instruction has a memory reference 
		In x86 an instruction can have only one memory reference.
		So, we can safely iterate and get the latest memory reference. (dst, src mem -> dst = src)
	*/
	ref = opnd_create_null();
	for(i=0; i < static_info->src_num; i++){
		if(opnd_is_memory_reference(instr_get_src(where,src_no[i]))){
			ref = instr_get_src(where,src_no[i]);
		}
	}
	for(i=0; i< static_info->dest_num; i++){
		if(opnd_is_memory_reference(instr_get_dst(where,dest_no[i]))){
			ref = instr_get_dst(where,src_no[i]);
		}
	}
	
	if(!opnd_is_null(ref)){ /* we have a memory reference */
		drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);
	}

    /* The following assembly performs the following instructions
     * buf_ptr->static_info_instr = static_info;
     * buf_ptr->mem_addr = addr;
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

	/* buf_ptr->static_info_instr = static_info; */
    /* Move static_info to static_info_instr field of buf (which is a instr_trace_t *) */
    opnd1 = OPND_CREATE_MEM32(reg2, offsetof(instr_trace_t, static_info_instr));
    opnd2 = OPND_CREATE_INT32((ptr_int_t)static_info);
    instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

	if(!opnd_is_null(ref)){ /* only bother to store address if there was a memory reference */
		
		/* buf_ptr->mem_addr = addr; */
		/* Store address in memory ref */
		opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, mem_addr));
		opnd2 = opnd_create_reg(reg1);
		instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

	}

	/* buf_ptr++; */
    /* Increment reg value by pointer size using lea instr */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = opnd_create_base_disp(reg2, DR_REG_NULL, 0,
                                  sizeof(instr_trace_t),
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

/* prints the trace and empties the instruction buffer */
static void print_trace(void *drcontext)
{
    /*per_thread_t *data;
    int num_refs;
	instr_trace_t *instr_trace;
#ifdef READABLE_TRACE
    int i;
#endif

    data      = drmgr_get_tls_field(drcontext, tls_index);
    instr_trace   = (instr_trace_t *)data->buf_base;
    num_refs  = (int)((instr_trace_t *)data->buf_ptr - instr_trace);

#ifdef READABLE_TRACE
	//TODO
    for (i = 0; i < num_refs; i++) {
        dr_fprintf(data->log, PFX",%c,%d,"PFX"\n",
                   instr_ref->pc, mem_ref->write ? 'w' : 'r', mem_ref->size, mem_ref->addr);
        ++instr_trace;
    }
#else
    dr_write_file(data->log, data->buf_base,
                  (size_t)(data->buf_ptr - data->buf_base));
#endif

    memset(data->buf_base, 0, INSTR_BUF_SIZE);
    data->num_refs += num_refs;
    data->buf_ptr   = data->buf_base;
	*/
}

/* clean_call dumps the memory reference info to the log file */
static void
clean_call(void)
{
    void *drcontext = dr_get_current_drcontext();
    print_trace(drcontext);
}

/* code cache to hold the call to "clean_call" and return to DR code cache */
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




