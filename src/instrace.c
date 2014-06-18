#include <string.h> /* for memset */
#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "utilities.h"
#include "instrace.h"
#include "debug.h"
#include "inops.h"
#include "output.h"

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

#define MAX_MEM_OPNDS 10

#define DST_TYPE 1
#define SRC_TYPE 2

#define VERBOSE_INSTRACE
//#define OPCODE_TRACE
//#define DISASSEMBLY_TRACE
#define OUTPUT_TO_FILE_TRACE
//#define OPERAND_TRACE



/* 
	instrace main structure  
*/

typedef struct _instr_trace_t {

	instr_t * static_info_instr;
	uint num_mem;
	uint pos[MAX_MEM_OPNDS];
	uint dst_or_src[MAX_MEM_OPNDS];
	uint mem_opnds[MAX_MEM_OPNDS];
	uint eflags;

} instr_trace_t;


/* Control the format of memory trace: readable or hexl */
#define READABLE_TRACE
/* Max number of mem_ref a buffer can have */
#define MAX_NUM_INSTR_TRACES 8192
/* The size of memory buffer for holding mem_refs. When it fills up,
 * we dump data from the buffer to the file.
 */
#define INSTR_BUF_SIZE (sizeof(instr_trace_t) * MAX_NUM_INSTR_TRACES)
#define OUTPUT_BUF_SIZE (sizeof(output_t) * MAX_NUM_INSTR_TRACES)


/* thread private log file and counter */
typedef struct {
    char   *buf_ptr;
    char   *buf_base;
    /* buf_end holds the negative value of real address of buffer end. */
    ptr_int_t buf_end;
    void   *cache;
	output_t * output_array;
	
	/* array to keep static instructions */
	instr_t ** static_array;
	uint static_ptr;
	uint static_array_size;

    file_t  log;
    uint64  num_refs;
} per_thread_t;


static client_id_t client_id;
static app_pc opnd_code_cache;
static app_pc print_code_cache;
static void  *mutex;    /* for multithread support */
static uint64 num_refs; /* keep a global memory reference count */
static int tls_index;

static bool opcode_missed[OPCODE_COUNT];

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
static void print_code_cache_init(void);
static void print_code_cache_exit(void);
static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               instr_t * static_info);
static instr_t * static_info_instrumentation(void * drcontext, instr_t* instr);
bool parse_commandline_args (const char * args);
void operand_trace(instr_t * instr,void * drcontext);

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

	return true;
}

void instrace_init(client_id_t id,const char * arguments)
{

	file_t in_file;
	file_t out_file;
	int i;

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


	/*out_file = dr_open_file("C:\\Charith\\Dropbox\\Research\\development\\exalgo\\tests\\c_tests\\loop_2_filter.txt",DR_FILE_WRITE_OVERWRITE);
	md_print_to_file(head,out_file);
	dr_close_file(out_file);

	dr_printf("done - writing output\n");*/


	mutex = dr_mutex_create();
    tls_index = drmgr_register_tls_field();
    DR_ASSERT(tls_index != -1);
    print_code_cache_init();

	for(i=OP_FIRST;i<=OP_LAST; i++){
		opcode_missed[i] = false;
	}
	

}

void instrace_exit_event()
{

	int i;

#ifdef VERBOSE_INSTRACE
	dr_printf("total amount of instructions - %d\n",num_refs);
	dr_printf("opcodes you missed - \n");
	for(i=OP_FIRST; i<=OP_LAST; i++){
		if(opcode_missed[i]){
			dr_printf("%s - ",decode_opcode_name(i));
		}
	}
	dr_printf("\n");
#endif

	md_delete_list(head,false);
	dr_global_free(client_arg,sizeof(client_arg_t));
    print_code_cache_exit();
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
    DR_ASSERT(len > 0);
    len += dr_snprintf(logname + len,
                      (sizeof(logname) - len)/sizeof(logname[0]),
                      "%s.instrace.%d.log", dr_get_application_name(),dr_get_thread_id(drcontext));
	DR_ASSERT(len > 0);
    logname[len] = '\0';
#ifdef VERBOSE_INSTRACE
	dr_printf("thread id : %d, new thread logging at - %s\n",dr_get_thread_id(drcontext),logname);
#endif
    data->log = dr_open_file(logname,
                             DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(data->log != INVALID_FILE);


	data->static_array = (instr_t **)dr_thread_alloc(drcontext,sizeof(instr_t *)*client_arg->static_info_size);
	data->static_array_size = client_arg->static_info_size;
	data->static_ptr = 0;

	data->output_array = (output_t *)dr_thread_alloc(drcontext,OUTPUT_BUF_SIZE);

}


void
instrace_thread_exit(void *drcontext)
{
    per_thread_t *data;
	int i;
#ifndef OPERAND_TRACE
    print_trace(drcontext);
#endif
    data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mutex_lock(mutex);
    num_refs += data->num_refs;
    dr_mutex_unlock(mutex);
    dr_close_file(data->log);
    dr_thread_free(drcontext, data->buf_base, INSTR_BUF_SIZE);
	dr_thread_free(drcontext, data->output_array, OUTPUT_BUF_SIZE);

#ifdef VERBOSE_INSTRACE
	dr_printf("thread id : %d, cloned instructions freeing now - %d\n",dr_get_thread_id(drcontext),data->static_ptr);
#endif
	for(i=0 ; i<data->static_ptr; i++){
		instr_destroy(dr_get_current_drcontext(),data->static_array[i]);
	}

	dr_thread_free(drcontext, data->static_array, sizeof(instr_t *)*client_arg->static_info_size);
    dr_thread_free(drcontext, data, sizeof(per_thread_t));
}


/* we transform string loops into regular loops so we can more easily
 * monitor every memory reference they make
 */
dr_emit_flags_t
instrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating)
{
	/*bool expanded;
	char stringop[MAX_STRING_LENGTH];
	instr_t * instr;
	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);
	instr_t * first, * test, * last, * label, * jz, * main;
	opnd_t opnd;
	int i;

	

	instrlist_disassemble(drcontext,instr_get_app_pc(instrlist_first(bb)),bb,data->log);
	
	if (!drutil_expand_rep_string_ex(drcontext, bb,&expanded,&instr)) {
        DR_ASSERT(false);
    }

	if(expanded){

		dr_fprintf(data->log,"expanded\n");
		instrlist_disassemble(drcontext,instr_get_app_pc(instrlist_first(bb)),bb,data->log);

		first = instrlist_first(bb);
		last = instrlist_last(bb);

		//need to get the main instruction
		main = first;
		for(i=0; i<5;i++){
			main = instr_get_next(main);
		}
		instr_disassemble_to_buffer(drcontext,main,stringop,MAX_STRING_LENGTH);
		dr_printf("%s\n",stringop);


		label = INSTR_CREATE_label(drcontext);
		instrlist_meta_preinsert(bb,first,label);

		dr_save_reg(drcontext,bb,first,DR_REG_XAX,SPILL_SLOT_2);
		dr_save_arith_flags_to_xax(drcontext,bb,first);

		test = INSTR_CREATE_test(drcontext,opnd_create_reg(DR_REG_XCX),opnd_create_reg(DR_REG_XCX));
		instrlist_meta_preinsert(bb,first,test);
		
		opnd = instr_get_target(first);
		opnd_disassemble_to_buffer(drcontext,opnd,stringop,MAX_STRING_LENGTH);
		dr_printf("%s\n",stringop);

		jz = INSTR_CREATE_jcc(drcontext,OP_jz,opnd);
		instrlist_preinsert(bb,first,jz);
		instr_set_next(jz,instr_get_next(first));

		dr_restore_arith_flags_from_xax(drcontext,bb,main);
		dr_restore_reg(drcontext,bb,main,DR_REG_XAX,SPILL_SLOT_2);

		//instr_set_target(last,opnd_create_instr(label));

		dr_fprintf(data->log,"after change\n");
		instrlist_disassemble(drcontext,instr_get_app_pc(instrlist_first(bb)),bb,data->log);
		
		dr_printf("expanded string\n");
		instr_disassemble_to_buffer(drcontext,instr,stringop,MAX_STRING_LENGTH);
		dr_printf("%s\n",stringop);
		//instr_destroy(drcontext,instr);
	}*/
	

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
	  1. First we need to filter the instructions to instrument 
	  2. call the static info filler function to get a slot at the global instruction array
      3. send the data appropriately to instrumentation function
	*/
	instr_t * instr_info;


	/* these are for the use of the caller - instrlist_first(bb) */
	if(filter_from_list(head,instr,client_arg->filter_mode)){
			//dr_printf("entering static instrumentation\n");
			instr_info = static_info_instrumentation(drcontext, instr);
			if(instr_info != NULL){ /* we may filter out the branch instructions */
#ifndef OPCODE_TRACE
				dynamic_info_instrumentation(drcontext,bb,instr,instr_info);		
#endif
			}
	}


	return DR_EMIT_DEFAULT;
			
}


static instr_t * static_info_instrumentation(void * drcontext, instr_t* instr){
	/*
		for each src and dest add the information accordingly
		this should return canonicalized static info about an instruction; breaking down any complex instructions if necessary

		1) check whether this instruction needs to be instrumented
		2) if yes, then get a location and then proceed to instrument -> return the struct
		3) if no, return null

	*/

	/* main variables */
	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);

	/* helper variables */
	int opcode;
	instr_t * ret;
	
	/* loop variables */
	int i;

	/* 1) */

	opcode = instr_get_opcode(instr);

#ifdef OPCODE_TRACE
	opcode_missed[opcode] = true;
	return NULL;
#endif

#ifdef OPERAND_TRACE
	operand_trace(instr,drcontext);
	return NULL;
#endif

	/* check whether this instr needs instrumentation - check for ones to skip and skip if */
	switch(opcode){
	case OP_jecxz:
		return NULL;
	}
	
	/* 2) */

	data->static_array[data->static_ptr++] = instr_clone(drcontext,instr);
	DR_ASSERT(data->static_ptr < data->static_array_size);

	return data->static_array[data->static_ptr - 1];

}

void disassembly_trace(){

	char disassembly[SHORT_STRING_LENGTH];

	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(),tls_index);
	instr_trace_t * trace = (instr_trace_t *)data->buf_ptr;
	instr_disassemble_to_buffer(dr_get_current_drcontext(),trace->static_info_instr,disassembly,SHORT_STRING_LENGTH);
	dr_printf("%s\n",disassembly);
}


void clean_call_populate_mem(reg_id_t reg, uint pos, uint dest_or_src){

	uint addr;
	per_thread_t * data;
	dr_mcontext_t mc = {sizeof(mc),DR_MC_ALL};
	void * drcontext = dr_get_current_drcontext();
	instr_trace_t * trace;

	dr_get_mcontext(drcontext,&mc);
	addr = reg_get_value(reg,&mc);

	data = drmgr_get_tls_field(drcontext,tls_index);
	trace = (instr_trace_t *)data->buf_ptr;
	trace->mem_opnds[trace->num_mem] = addr;
	trace->pos[trace->num_mem] = pos;
	trace->dst_or_src[trace->num_mem++] = dest_or_src;
	

}

static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               instr_t * static_info)
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
	reg_id_t reg3 = DR_REG_XAX;
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
	dr_save_reg(drcontext, ilist, where, reg3, SPILL_SLOT_4);
	

	/*  check whether this instruction has a memory reference 
		In x86 an instruction can have only one memory reference.
		So, we can safely iterate and get the latest memory reference. (dst, src mem -> dst = src)
	*/

	/* The following assembly performs the following instructions
	 * for(all memory operands){
	 *    buf_ptr->pos[num_mem] = i;
	 *    buf_ptr->dest_or_src[num_mem] = ?
	 *    buf_ptr->mem_addr[num_mem] = ?
	 *	  buf_ptr->num_mem++;
	 * }
     * buf_ptr++;
     * if (buf_ptr >= buf_end_ptr)
     *    clean_call();
     */

	/*
	assembly to get the memory addresses
	reg 1 <- lea mem_addr
	reg_2 <- buf_ptr
	reg_3 <- [reg_2 + offset(num_mem)]
	reg_4 <- reg_2 + offset(dest_or_src)
	[reg_4 + uint(reg_3)] <- dest or src
	reg_4 <- reg_2 + offset(pos)
	[reg_4 + uint(reg_3)] <- pos
	reg_4 <- reg_2 + offset(addr)
	[reg_4 + uint(reg_3)] <- reg_1
	[reg_2 + offset(num_mem)]++;

	then the rest

	*/
	dr_save_arith_flags_to_xax(drcontext,ilist,where);


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

	/* buf_ptr->num_mem = 0; */
    opnd1 = OPND_CREATE_MEM32(reg2, offsetof(instr_trace_t, num_mem));
    opnd2 = OPND_CREATE_INT32(0);
    instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);


	for(i=0; i<instr_num_dsts(where); i++){
		if(opnd_is_memory_reference(instr_get_dst(where,i))){
			ref =  instr_get_dst(where,i);

			DR_ASSERT(opnd_is_null(ref) == false);
		
			drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);
			dr_insert_clean_call(drcontext,ilist,where,clean_call_populate_mem,false,3,OPND_CREATE_INT32(reg1),OPND_CREATE_INT32(i),OPND_CREATE_INT32(DST_TYPE));
		
		}
	}

	for(i=0; i<instr_num_srcs(where); i++){
		if(opnd_is_memory_reference(instr_get_src(where,i))){
			ref =  instr_get_src(where,i);
		
			drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);
			dr_insert_clean_call(drcontext,ilist,where,clean_call_populate_mem,false,3,OPND_CREATE_INT32(reg1),OPND_CREATE_INT32(i),OPND_CREATE_INT32(SRC_TYPE));
		
		}
	}

#ifdef DISASSEMBLY_TRACE
	dr_insert_clean_call(drcontext,ilist,where,disassembly_trace,false,0);
#endif

	drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);
    /* Load data->buf_ptr into reg2 */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
    instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

	/* load the eflags */
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, eflags));
	opnd2 = opnd_create_reg(reg3);
    instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);



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
    opnd1 = opnd_create_pc(print_code_cache);
    instr = INSTR_CREATE_jmp(drcontext, opnd1);
    instrlist_meta_preinsert(ilist, where, instr);

    /* restore %reg */
    instrlist_meta_preinsert(ilist, where, restore);
    dr_restore_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
    dr_restore_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);
	dr_restore_reg(drcontext, ilist, where, reg3, SPILL_SLOT_4);

}

void output_populator_printer(void * drcontext, opnd_t opnd, instr_t * instr, uint addr, operand_t * output){


	uint value;
	float float_value;
	uint width;
	int i;

	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);


	if(opnd_is_reg(opnd)){
		
		value = opnd_get_reg(opnd);
		width = opnd_size_in_bytes(reg_get_size(value));
#ifdef READABLE_TRACE
		dr_fprintf(data->log,",%d,%d,%d",REG_TYPE, width, value);
#else	
		output->type = REG_TYPE; 
		output->width = width;
		output->value = value;

#endif
	
	}
	else if(opnd_is_immed(opnd)){
		
		//DR_ASSERT(opnd_is_immed_float(opnd) == false);

		if(opnd_is_immed_float(opnd)){

			width = opnd_size_in_bytes(opnd_get_size(opnd));
			float_value = opnd_get_immed_float(opnd);

#ifdef READABLE_TRACE
			dr_fprintf(data->log,",%d,%d,%.4f",IMM_FLOAT_TYPE,width,float_value);
#else
			output->type = IMM_FLOAT_TYPE;
			output->width = width;
			output->float_value = float_value;
#endif

		}
		
		if(opnd_is_immed_int(opnd)){

			width = opnd_size_in_bytes(opnd_get_size(opnd));
			value = opnd_get_immed_int(opnd);
#ifdef READABLE_TRACE
			dr_fprintf(data->log,",%d,%d,%d",IMM_INT_TYPE,width,value);
#else
			output->type = IMM_INT_TYPE;
			output->width = width;
			output->value = value;
#endif
		}

	}
	else if(opnd_is_memory_reference(opnd)){

		width = drutil_opnd_mem_size_in_bytes(opnd,instr);
		value = addr;
#ifdef READABLE_TRACE
		dr_fprintf(data->log, ",%d,%d,%d",MEM_STACK_TYPE,width,value);
#else
		output->type = MEM_STACK_TYPE;
		output->width = width;
		output->float_value = value;
#endif

	}
	

}

void operand_trace(instr_t * instr,void * drcontext){

	int i;
	char stringop[MAX_STRING_LENGTH];
	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);
	
	instr_disassemble_to_buffer(drcontext,instr,stringop,MAX_STRING_LENGTH);
	dr_fprintf(data->log,"%s\n",stringop);

	for(i=0; i<instr_num_dsts(instr); i++){
		opnd_disassemble_to_buffer(drcontext,instr_get_dst(instr,i),stringop,MAX_STRING_LENGTH);
		dr_fprintf(data->log,"dst-%d-%s\n",i,stringop);
	}

	for(i=0; i<instr_num_srcs(instr); i++){
		opnd_disassemble_to_buffer(drcontext,instr_get_src(instr,i),stringop,MAX_STRING_LENGTH);
		dr_fprintf(data->log,"src-%d-%s\n",i,stringop);
	}

}

uint get_address(instr_trace_t * trace, uint pos, uint dst_or_src){

	uint i;

	for(i=0; i<trace->num_mem; i++){
		if(trace->dst_or_src[i] == dst_or_src && trace->pos[i] == pos){
			return trace->mem_opnds[i];
		}
	}

	return 0;
	
}

static uint calculate_operands(instr_t * instr,uint dst_or_src){

	opnd_t op;
	int i;
	int ret = 0;

	if(dst_or_src == DST_TYPE){
		for(i=0; i<instr_num_dsts(instr); i++){
			op = instr_get_dst(instr,i);
			if(opnd_is_immed(op) ||
				opnd_is_memory_reference(op) ||
				opnd_is_reg(op)){
				ret++;
			}

		}
	}
	else if(dst_or_src == SRC_TYPE){
		for(i=0; i<instr_num_srcs(instr); i++){
			op = instr_get_src(instr,i);
			if(opnd_is_immed(op) ||
				opnd_is_memory_reference(op) ||
				opnd_is_reg(op)){
				ret++;
			}

		}
	}

	return ret;

}


/* prints the trace and empties the instruction buffer */
static void print_trace(void *drcontext)
{
    per_thread_t *data;
    int num_refs;
	instr_trace_t *instr_trace;
	instr_t * instr;
	int i;
	int j;
#ifdef READABLE_TRACE
	char disassembly[SHORT_STRING_LENGTH];
#else
	output_t * output;
#endif

    data      = drmgr_get_tls_field(drcontext, tls_index);
    instr_trace   = (instr_trace_t *)data->buf_base;
    num_refs  = (int)((instr_trace_t *)data->buf_ptr - instr_trace);


#ifdef OUTPUT_TO_FILE_TRACE
	
#ifdef READABLE_TRACE
	//TODO
    for (i = 0; i < num_refs; i++) {
		
		instr = instr_trace->static_info_instr;
		instr_disassemble_to_buffer(drcontext,instr,disassembly,SHORT_STRING_LENGTH);
		//dr_fprintf(data->log,"%s\n",disassembly);
		dr_fprintf(data->log,"%d",instr_get_opcode(instr));

		dr_fprintf(data->log,",%d",calculate_operands(instr,DST_TYPE));
		for(j=0; j<instr_num_dsts(instr); j++){
			output_populator_printer(drcontext,instr_get_dst(instr,j),instr,get_address(instr_trace,j,DST_TYPE),NULL);
		}

		dr_fprintf(data->log,",%d",calculate_operands(instr,SRC_TYPE));
		for(j=0; j<instr_num_srcs(instr); j++){
			output_populator_printer(drcontext,instr_get_src(instr,j),instr,get_address(instr_trace,j,SRC_TYPE),NULL);
		}
		dr_fprintf(data->log,",%d\n",instr_trace->eflags);
        ++instr_trace;
    }
#else

	/* we need to fill up the output array here */

	for(i = 0; i< num_refs; i++){
		instr = instr_trace->static_info_instr;
		output = &data->output_array[i];
		
		//opcode 
		output->opcode = instr_get_opcode(instr);
		output->num_dsts = 0;
		output->num_srcs = 0;

		for(j=0; j<instr_num_dsts(instr); j++){

			output_populator_printer(drcontext,instr_get_dst(instr,j),instr,get_address(instr_trace,j,DST_TYPE),&output->dsts[output->num_dsts]);
			output->num_dsts++;
		}

		dr_fprintf(data->log,"src,");
		for(j=0; j<instr_num_srcs(instr); j++){
			output_populator_printer(drcontext,instr_get_src(instr,j),instr,get_address(instr_trace,j,SRC_TYPE),&output->srcs[output->num_srcs]);
			output->num_srcs++;
		}
		
		output->eflags = instr_trace->eflags;

		++instr_trace;

	}


	dr_write_file(data->log,data->output_array,num_refs * sizeof(output_t));
#endif

	
#endif

    memset(data->buf_base, 0, INSTR_BUF_SIZE);
    data->num_refs += num_refs;
    data->buf_ptr   = data->buf_base;
	
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
print_code_cache_init(void)
{
    void         *drcontext;
    instrlist_t  *ilist;
    instr_t      *where;
    byte         *end;

    drcontext  = dr_get_current_drcontext();
    print_code_cache = dr_nonheap_alloc(PAGE_SIZE,
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
    end = instrlist_encode(drcontext, ilist, print_code_cache, false);
    DR_ASSERT((end - print_code_cache) < PAGE_SIZE);
    instrlist_clear_and_destroy(drcontext, ilist);
    /* set the memory as just +rx now */
    dr_memory_protect(print_code_cache, PAGE_SIZE, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
}

static void
print_code_cache_exit(void)
{
    dr_nonheap_free(print_code_cache, PAGE_SIZE);
}






