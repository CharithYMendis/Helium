#include "instrace.h"
#include <string.h> /* for memset */
#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "utilities.h"
#include "debug.h"
#include "output.h"
#include "funcwrap.h"

/****************************defines*********************************/

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

#define OPERAND_TRACE		1  /* this prints to the out files*/
#define OPCODE_TRACE		2  /* this prints to the console */
#define DISASSEMBLY_TRACE	3  /* this prints to the out files*/
#define INS_TRACE			4  /* this prints to the out file */
#define INS_DISASM_TRACE	5  /* this prints to the out file */

//debug prints
//#define DEBUG_MEM_REGS   /* prints out the memory regs before dr util mem address calculation */
//#define DEBUG_MEM_STATS  /* prints out the memory values - stats at that given pc */

/* Control the format of memory trace: readable or hexl */
#define READABLE_TRACE
/* Max number of mem_ref a buffer can have */
#define MAX_NUM_INSTR_TRACES 8192
/* The size of memory buffer for holding mem_refs. When it fills up,
 * we dump data from the buffer to the file.
 */
#define INSTR_BUF_SIZE (sizeof(instr_trace_t) * MAX_NUM_INSTR_TRACES)
#define OUTPUT_BUF_SIZE (sizeof(output_t) * MAX_NUM_INSTR_TRACES)

/*************************** typedefs ******************************/
/*instrace main structure*/
typedef struct _instr_trace_t {

	instr_t * static_info_instr;
	uint num_mem;
	uint pos[MAX_MEM_OPNDS];
	uint dst_or_src[MAX_MEM_OPNDS];
	uint mem_type[MAX_MEM_OPNDS];
	uint64 mem_opnds[MAX_MEM_OPNDS];
	uint eflags;
	uint pc;

} instr_trace_t;

/* thread private log file and counter */
typedef struct {

    char  * buf_ptr;
    char  * buf_base;
    /* buf_end holds the negative value of real address of buffer end. */
    ptr_int_t buf_end;
    void  * cache;
	output_t * output_array;
	
	/* array to keep static instructions */
	instr_t ** static_array;
	uint static_ptr;
	uint static_array_size;

    file_t outfile;
	file_t logfile;

	uint64  num_refs;

	/* thread stack limits */
	uint stack_base;
	uint deallocation_stack;
	uint thread_id;


} per_thread_t;

/* client arguments processing */
typedef struct _client_arg_t {

	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
	char output_folder[MAX_STRING_LENGTH];
	uint static_info_size;
	uint instrace_mode;
	char extra_info[MAX_STRING_LENGTH];


} client_arg_t;


/************************* global variables *************************/
static client_id_t client_id;
static app_pc code_cache;
static void  *mutex;    /* for multithread support */
static uint64 num_refs; /* total number of dynamic instructions */
static int tls_index;

static client_arg_t * client_arg;
static module_t * head;
static bool opcodes_visited[OPCODE_COUNT];
static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];

static module_t * instrace_head;

/*********************** function prototypes *************************/

/* instrumentation functions */
static instr_t * static_info_instrumentation(void * drcontext, instr_t* instr);
static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
	instr_t * static_info);
static void code_cache_init(void);
static void code_cache_exit(void);

/* utility functions */
static bool parse_commandline_args(const char * args);

/* clean calls */
//needed instrumentation
static void clean_call_ins_trace(void);
static void clean_call_disassembly_trace();
static void clean_call_populate_mem(reg_t regvalue, uint pos, uint dest_or_src);
//debug
static void clean_call_print_regvalues();
static void clean_call_mem_stats(reg_t memvalue);
static void clean_call_teb_info(uint * stack_base, uint * stack_limit, uint * deallocation_stack);

/* printing functions */
static void ins_trace(void *drcontext);
void operand_trace(instr_t * instr, void * drcontext);


/******************* function implementation ************************/

/****************** main instrumentation functions ******************/

static bool parse_commandline_args (const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));
	if(dr_sscanf(args,"%s %d %s %d %d %s",
									   &client_arg->filter_filename,
									   &client_arg->filter_mode,
									   &client_arg->output_folder,
									   &client_arg->static_info_size,
									   &client_arg->instrace_mode,
									   &client_arg->extra_info)!=6){
		return false;
	}

	return true;
}

void instrace_init(client_id_t id, const char * name, const char * arguments)
{

	file_t in_file;
	file_t out_file;
	int i;
	char logfilename[MAX_STRING_LENGTH];

    drmgr_init();
    drutil_init();
    client_id = id;

	DR_ASSERT(parse_commandline_args(arguments)==true);

	head = md_initialize();
	instrace_head = md_initialize();

	if(client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename,DR_FILE_READ);
		DR_ASSERT(in_file != INVALID_FILE);
		md_read_from_file(head,in_file,false);
		dr_close_file(in_file);
	}

	mutex = dr_mutex_create();
    tls_index = drmgr_register_tls_field();
    DR_ASSERT(tls_index != -1);
    code_cache_init();

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);

	for(i=OP_FIRST;i<=OP_LAST; i++){
		opcodes_visited[i] = false;
	}

	
	

}

void instrace_exit_event()
{

	int i;

	DEBUG_PRINT("%s - total amount of instructions - %d\n",ins_pass_name, num_refs);

	if (client_arg->instrace_mode == OPCODE_TRACE){
		dr_printf("opcodes that were covered in this part of the code - \n");
		for (i = OP_FIRST; i <= OP_LAST; i++){
			if (opcodes_visited[i]){
				dr_printf(logfile,"%s - ", decode_opcode_name(i));
			}
		}
		dr_printf("\n");
	}

	md_delete_list(head, false);
	md_delete_list(instrace_head, false);
	dr_global_free(client_arg,sizeof(client_arg_t));
    code_cache_exit();
    drmgr_unregister_tls_field(tls_index);
    dr_mutex_destroy(mutex);
	if (log_mode){
		dr_close_file(logfile);
	}
    drutil_exit();
    drmgr_exit();
}

void instrace_thread_init(void *drcontext)
{
    char outfilename[MAX_STRING_LENGTH];
	char logfilename[MAX_STRING_LENGTH];
	char thread_id[MAX_STRING_LENGTH];
	char extra_info[MAX_STRING_LENGTH];

    char *dirsep;
    int len;
    per_thread_t *data;
	char * mode;

	uint * stack_base;
	uint * deallocation_stack;

	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));

    /* allocate thread private data */
    data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
    drmgr_set_tls_field(drcontext, tls_index, data);
    data->buf_base = dr_thread_alloc(drcontext, INSTR_BUF_SIZE);
    data->buf_ptr  = data->buf_base;
    /* set buf_end to be negative of address of buffer end for the lea later */
	data->buf_end  = -(ptr_int_t)(data->buf_base + INSTR_BUF_SIZE);
    data->num_refs = 0;
	data->thread_id = dr_get_thread_id(drcontext);

    /* We're going to dump our data to a per-thread file.
     * On Windows we need an absolute path so we place it in
     * the same directory as our library. We could also pass
     * in a path and retrieve with dr_get_options().
     */
	dr_snprintf(thread_id, MAX_STRING_LENGTH, "%d", dr_get_thread_id(drcontext));
	if (log_mode){
		populate_conv_filename(logfilename, logdir, ins_pass_name, thread_id);
		data->logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
	}

	/* instrace types */
	if (client_arg->instrace_mode == OPERAND_TRACE){
		mode = "opnd";
	}
	else if (client_arg->instrace_mode == OPCODE_TRACE){
		mode = "opcode";
	}
	else if (client_arg->instrace_mode == DISASSEMBLY_TRACE){
		mode = "disasm";
	}
	else if (client_arg->instrace_mode == INS_DISASM_TRACE){
		mode = "asm_instr";
	}
	else{
		mode = "instr";
	}
	
	

	dr_snprintf(extra_info, MAX_STRING_LENGTH, "%s_%s_%s", client_arg->extra_info, mode, thread_id);

	populate_conv_filename(outfilename, client_arg->output_folder, ins_pass_name, extra_info);
	data->outfile = dr_open_file(outfilename, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
	DR_ASSERT(data->outfile != INVALID_FILE);

	DEBUG_PRINT("%s - thread id : %d, new thread logging at - %s\n",ins_pass_name, dr_get_thread_id(drcontext),logfilename);

	data->static_array = (instr_t **)dr_thread_alloc(drcontext,sizeof(instr_t *)*client_arg->static_info_size);
	data->static_array_size = client_arg->static_info_size;
	data->static_ptr = 0;

	data->output_array = (output_t *)dr_thread_alloc(drcontext,OUTPUT_BUF_SIZE);

	deallocation_stack = &data->deallocation_stack;
	stack_base = &data->stack_base;

	__asm{
		mov EAX, FS : [0x04]
		mov EBX, stack_base
		mov [EBX], EAX
		mov EAX, FS : [0xE0C]
		mov EBX, deallocation_stack
		mov [EBX], EAX
	}

	DEBUG_PRINT("%s - thread %d stack information - stack_base %x stack_reserve %x\n", ins_pass_name,
		dr_get_thread_id(drcontext), data->stack_base, data->deallocation_stack);

	DEBUG_PRINT("%s - initializing thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));


}

void
instrace_thread_exit(void *drcontext)
{
    per_thread_t *data;
	int i;

	if (client_arg->instrace_mode == INS_TRACE){
		ins_trace(drcontext);
	}

    data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mutex_lock(mutex);
    num_refs += data->num_refs;
    dr_mutex_unlock(mutex);

    dr_close_file(data->outfile);
	if (log_mode){
		dr_close_file(data->logfile);
	}

	dr_thread_free(drcontext, data->buf_base, INSTR_BUF_SIZE);
	dr_thread_free(drcontext, data->output_array, OUTPUT_BUF_SIZE);

	DEBUG_PRINT("%s - thread id : %d, cloned instructions freeing now - %d\n",ins_pass_name, dr_get_thread_id(drcontext),data->static_ptr);

	for(i=0 ; i<data->static_ptr; i++){
		instr_destroy(dr_get_current_drcontext(),data->static_array[i]);
	}

	dr_thread_free(drcontext, data->static_array, sizeof(instr_t *)*client_arg->static_info_size);
    dr_thread_free(drcontext, data, sizeof(per_thread_t));

	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}


/* we transform string loops into regular loops so we can more easily
 * monitor every memory reference they make
 */
dr_emit_flags_t
instrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating)
{
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
	//if(filter_from_list(head,instr,client_arg->filter_mode) && should_filter_thread(dr_get_thread_id(drcontext))){
			//dr_printf("entering static instrumentation\n");
			instr_info = static_info_instrumentation(drcontext, instr);
			if(instr_info != NULL){ /* we may filter out the branch instructions */
				/* can only be entered in the DISASSEMBLY_TRACE or INS_TRACE*/
				DR_ASSERT(client_arg->instrace_mode == INS_TRACE || client_arg->instrace_mode == DISASSEMBLY_TRACE);
				dynamic_info_instrumentation(drcontext, bb, instr, instr_info);
			}
	//}


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

	if (client_arg->instrace_mode == OPCODE_TRACE){
		opcodes_visited[opcode] = true;
		return NULL;
	}

	if ( (client_arg->instrace_mode == OPERAND_TRACE) || (client_arg->instrace_mode == INS_DISASM_TRACE) ){
		operand_trace(instr, drcontext);
		return NULL;
	}

	/* check whether this instr needs instrumentation - check for ones to skip and skip if */
	/*switch(opcode){
	case OP_jecxz:
		return NULL;
	}*/
	
	/* 2) */

	data->static_array[data->static_ptr++] = instr_clone(drcontext,instr);
	DR_ASSERT(data->static_ptr < data->static_array_size);

	return data->static_array[data->static_ptr - 1];

}

/* dynamic information generation */

static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               instr_t * static_info)
{


	/* 
		issues that may arise
		1. pc and eflags is uint but in 64 bit mode 8 byte transfers are done -> so far no problem (need to see this)
			need to see whether there is a better way
		2. double check all the printing
	*/

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
    uint pc;
	uint i;

	instr_t * label_func;
	instr_t * label_thread;

	module_data_t * module_data;

	if (client_arg->instrace_mode == DISASSEMBLY_TRACE){
		dr_insert_clean_call(drcontext, ilist, where, clean_call_disassembly_trace, false, 0);
		return;
	}

    data = drmgr_get_tls_field(drcontext, tls_index);
	label_func = INSTR_CREATE_label(drcontext);
	label_thread = INSTR_CREATE_label(drcontext);


	/* just print out the before ilist */
	//instrlist_disassemble(drcontext, instr_get_app_pc(instrlist_first(ilist)), ilist, logfile);

    /* Steal the register for memory reference address *
     * We can optimize away the unnecessary register save and restore
     * by analyzing the code and finding the register is dead.
     */

	/* pre check code */

	/*
	reg1 = EBX
	reg2 = ECX
	reg3 = EAX
	*/

	
	dr_save_reg(drcontext, ilist, where, reg3, SPILL_SLOT_4);
	dr_save_arith_flags_to_xax(drcontext, ilist, where);

	//cmp [is_within_func],0
	opnd1 = OPND_CREATE_ABSMEM(&is_within_func, OPSZ_4);
	opnd2 = OPND_CREATE_INTPTR(0);
	instr = INSTR_CREATE_cmp(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	//je label_func
	opnd1 = opnd_create_instr(label_func);
	instr = INSTR_CREATE_jcc(drcontext, OP_je, opnd1);
	instrlist_meta_preinsert(ilist, where, instr);

	dr_save_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
	dr_save_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);
	
	drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);

	/* Load data->thread_id into reg2 */
	opnd1 = opnd_create_reg(reg2);
	opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, thread_id));
	instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	//mov ebx, [thread_id_func]
	opnd1 = opnd_create_reg(reg1);
	opnd2 = OPND_CREATE_ABSMEM(&thread_id_func, OPSZ_4);
	instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist, where, instr);

	//cmp ebx,ecx
	opnd1 = opnd_create_reg(reg2);
	opnd2 = opnd_create_reg(reg1);
	instr = INSTR_CREATE_cmp(drcontext, opnd1, opnd2);
	instrlist_meta_preinsert(ilist,where,instr);

	//jne label_thread
	opnd1 = opnd_create_instr(label_thread);
	instr = INSTR_CREATE_jcc(drcontext, OP_jne, opnd1);
	instrlist_meta_preinsert(ilist,where,instr); 

	/* end of precheck code */


	drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);

    /* Load data->buf_ptr into reg2 */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
    instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);


	/* buf_ptr->static_info_instr = static_info; */
    /* Move static_info to static_info_instr field of buf (which is a instr_trace_t *) */
    opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, static_info_instr));
	instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)static_info, opnd1, ilist, where, &first, &second);

	/* buf_ptr->num_mem = 0; */
    opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, num_mem));
    
	instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)0, opnd1, ilist, where, &first, &second);

	for (i = 0; i<instr_num_dsts(where); i++){
		if (opnd_is_memory_reference(instr_get_dst(where, i))){
			ref = instr_get_dst(where, i);

			DR_ASSERT(opnd_is_null(ref) == false);


			dr_restore_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
			dr_restore_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);

#ifdef DEBUG_MEM_REGS
			dr_insert_clean_call(drcontext, ilist, where, clean_call_disassembly_trace, false, 0);
			dr_insert_clean_call(drcontext, ilist, where, clean_call_print_regvalues, false, 0);
#endif
			
			drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);

#ifdef DEBUG_MEM_REGS
			dr_insert_clean_call(drcontext, ilist, where, clean_call_print_regvalues, false, 0);
#endif

#ifdef DEBUG_MEM_STATS
			dr_insert_clean_call(drcontext, ilist, where, clean_call_disassembly_trace, false, 0);
			dr_insert_clean_call(drcontext, ilist, where, clean_call_mem_stats, false, 1, opnd_create_reg(reg1));
#endif

			dr_insert_clean_call(drcontext, ilist, where, clean_call_populate_mem, false, 3, opnd_create_reg(reg1), OPND_CREATE_INT32(i), OPND_CREATE_INT32(DST_TYPE));

		}
	}

	for (i = 0; i<instr_num_srcs(where); i++){
		if (opnd_is_memory_reference(instr_get_src(where, i))){
			ref = instr_get_src(where, i);

			DR_ASSERT(opnd_is_null(ref) == false);			
			
			dr_restore_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
			dr_restore_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);

#ifdef DEBUG_MEM_REGS
			dr_insert_clean_call(drcontext, ilist, where, clean_call_disassembly_trace, false, 0);
			dr_insert_clean_call(drcontext, ilist, where, clean_call_print_regvalues, false, 0);
#endif

			drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);

#ifdef DEBUG_MEM_REGS

			dr_insert_clean_call(drcontext, ilist, where, clean_call_print_regvalues, false, 0);
#endif

#ifdef DEBUG_MEM_STATS
			dr_insert_clean_call(drcontext, ilist, where, clean_call_disassembly_trace, false, 0);
			dr_insert_clean_call(drcontext, ilist, where, clean_call_mem_stats, false, 1, opnd_create_reg(reg1));
#endif

			dr_insert_clean_call(drcontext, ilist, where, clean_call_populate_mem, false, 3, opnd_create_reg(reg1), OPND_CREATE_INT32(i), OPND_CREATE_INT32(SRC_TYPE));

		}
	}
	

	

	drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);
    /* Load data->buf_ptr into reg2 */
    opnd1 = opnd_create_reg(reg2);
    opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
    instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);

	/* arithmetic flags are saved here for buf_ptr->eflags filling */
	//dr_save_arith_flags_to_xax(drcontext, ilist, where);

	/* load the eflags */
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, eflags));
	opnd2 = opnd_create_reg(reg3);
    instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
    instrlist_meta_preinsert(ilist, where, instr);


	/* load the app_pc */
	opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, pc));
	module_data = dr_lookup_module(instr_get_app_pc(where));

	//dynamically generated code - module information not available - then just store 0 at the pc slot of the instr_trace data
	if (module_data != NULL){
		pc = instr_get_app_pc(where) - module_data->start;
		dr_free_module_data(module_data);
	}
	else{
		pc = 0;
	}

	instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc, opnd1, ilist, where, &first, &second);



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

	instrlist_meta_preinsert(ilist, where, label_thread);

	
    dr_restore_reg(drcontext, ilist, where, reg1, SPILL_SLOT_2);
	dr_restore_reg(drcontext, ilist, where, reg2, SPILL_SLOT_3);


	instrlist_meta_preinsert(ilist, where, label_func);
	
	
	dr_restore_arith_flags_from_xax(drcontext, ilist, where);
	dr_restore_reg(drcontext, ilist, where, reg3, SPILL_SLOT_4);


	/* just print out the before ilist */
	//instrlist_disassemble(drcontext, instr_get_app_pc(instrlist_first(ilist)), ilist, logfile); 
	//dr_abort();


}

/*****************************end instrumentation functions************************/

static print_base_disp_for_lea(file_t file, opnd_t opnd){

	/* [base + index * scale + disp] */
	dr_fprintf(file, " base - %d %s\n", opnd_get_base(opnd), get_register_name(opnd_get_base(opnd)));
	dr_fprintf(file, " index - %d %s\n", opnd_get_index(opnd), get_register_name(opnd_get_index(opnd)));
	dr_fprintf(file, "reg - %d\n", opnd_is_reg(opnd_create_reg(opnd_get_index(opnd))));
	dr_fprintf(file, " scale - %d\n", opnd_get_scale(opnd));
	dr_fprintf(file, " disp - %d\n", opnd_get_disp(opnd));

}

/* this is only called when the instrace mode is operand trace (this happens at the instrumentation time) */
static void operand_trace(instr_t * instr, void * drcontext){

	int i;
	char stringop[MAX_STRING_LENGTH];
	int pc = 0;
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	module_data_t * module_data = dr_lookup_module(instr_get_app_pc(instr));

	if (module_data != NULL){
		pc = instr_get_app_pc(instr) - module_data->start;
	}
	instr_disassemble_to_buffer(drcontext, instr, stringop, MAX_STRING_LENGTH);
	
	if (client_arg->instrace_mode == OPERAND_TRACE){

		dr_fprintf(data->outfile, "%s\n", stringop);

		for (i = 0; i < instr_num_dsts(instr); i++){
			opnd_disassemble_to_buffer(drcontext, instr_get_dst(instr, i), stringop, MAX_STRING_LENGTH);
			if ((instr_get_opcode(instr) == OP_lea) && opnd_is_base_disp(instr_get_dst(instr,i))){
				dr_fprintf(data->outfile, "dst-\n");
				print_base_disp_for_lea(data->outfile, instr_get_dst(instr, i));
			}
			else{
				dr_fprintf(data->outfile, "dst-%d-%s\n", i, stringop);
			}
		}

		for (i = 0; i < instr_num_srcs(instr); i++){
			opnd_disassemble_to_buffer(drcontext, instr_get_src(instr, i), stringop, MAX_STRING_LENGTH);
			if ((instr_get_opcode(instr) == OP_lea) && opnd_is_base_disp(instr_get_src(instr, i))){
				dr_fprintf(data->outfile, "src-\n");
				print_base_disp_for_lea(data->outfile, instr_get_src(instr, i));
			}
			else{
				dr_fprintf(data->outfile, "src-%d-%s\n", i, stringop);
			}
		}

		if (module_data != NULL){
			dr_fprintf(data->outfile, "app_pc-%d\n", pc);
		}
	}
	else if (client_arg->instrace_mode == INS_DISASM_TRACE){
		if (module_data != NULL){
			if (md_get_module_position(instrace_head, module_data->full_path) == -1){
				md_add_module(instrace_head, module_data->full_path, MAX_BBS_PER_MODULE);
			}
			dr_fprintf(data->outfile, "%d,%d,%s\n", md_get_module_position(instrace_head, module_data->full_path), pc, stringop);
		}
		else{
			dr_fprintf(data->outfile, "%d,%d,%s\n",0, 0, stringop);
		}
		
	}

	dr_free_module_data(module_data);

}

/* this is only called when the instrace mode is disassembly trace (this happens at the analysis time)*/
static void clean_call_disassembly_trace(){

	char disassembly[SHORT_STRING_LENGTH];

	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	instr_trace_t * trace = (instr_trace_t *)data->buf_ptr;
	module_data_t * md;

	md = dr_lookup_module(instr_get_app_pc(trace->static_info_instr));

	instr_disassemble_to_buffer(dr_get_current_drcontext(), trace->static_info_instr, disassembly, SHORT_STRING_LENGTH);

	dr_fprintf(data->outfile, "%s ", disassembly);

	if (md != NULL){
		dr_fprintf(data->outfile, "%x", instr_get_app_pc(trace->static_info_instr) - md->start);
		dr_free_module_data(md);
	}
	dr_fprintf(data->outfile, "\n");
}

/* called when the mode is ins trace to populate the memory operands */
static void clean_call_populate_mem(reg_t regvalue, uint pos, uint dest_or_src){


	char string_ins[MAX_STRING_LENGTH];
	per_thread_t * data;
	void * drcontext = dr_get_current_drcontext();
	instr_trace_t * trace;

	data = drmgr_get_tls_field(drcontext, tls_index);
	trace = (instr_trace_t *)data->buf_ptr;
	trace->mem_opnds[trace->num_mem] = regvalue;
	trace->pos[trace->num_mem] = pos;
	/* assuming the thread init gives out stack bounds properly we can select the memory type as follows */
	if (regvalue <= data->stack_base && regvalue >= data->deallocation_stack){
		trace->mem_type[trace->num_mem] = MEM_STACK_TYPE;
	}
	else{
		trace->mem_type[trace->num_mem] = MEM_HEAP_TYPE;
	}

	trace->dst_or_src[trace->num_mem++] = dest_or_src;



}

/* prints out the operands / populates the operands in the instrace mode */
static void output_populator_printer(void * drcontext, opnd_t opnd, instr_t * instr, uint64 addr, uint mem_type, operand_t * output){


	int value;
	float float_value;
	uint width;
	int i;

	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);


	if(opnd_is_reg(opnd)){
		
		value = opnd_get_reg(opnd);
		if (value != DR_REG_NULL){
			width = opnd_size_in_bytes(reg_get_size(value));
		}
		else{
			width = 0;
		}
		
#ifdef READABLE_TRACE
		dr_fprintf(data->outfile,",%u,%u,%u",REG_TYPE, width, value);
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
			dr_fprintf(data->outfile,",%u,%u,%.4f",IMM_FLOAT_TYPE,width,float_value);
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
			dr_fprintf(data->outfile,",%u,%u,%d",IMM_INT_TYPE,width,value);
#else
			output->type = IMM_INT_TYPE;
			output->width = width;
			output->value = value;
#endif
		}

	}
	else if(opnd_is_memory_reference(opnd)){

		width = drutil_opnd_mem_size_in_bytes(opnd,instr);
#ifdef READABLE_TRACE
		dr_fprintf(data->outfile, ",%u,%u,%llu",mem_type,width,addr);
#else
		output->type = mem_type;
		output->width = width;
		output->float_value = addr;
#endif

	}
	

}

/* helper functions for the print trace */
static void get_address(instr_trace_t *trace, uint pos, uint dst_or_src, uint *type, uint64  *addr){

	uint i;
	*type = 0;
	*addr = 0;

	for(i=0; i<trace->num_mem; i++){
		if(trace->dst_or_src[i] == dst_or_src && trace->pos[i] == pos){
			*addr = trace->mem_opnds[i];
			*type = trace->mem_type[i];
		}
	}

	return;
	
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

			if (instr_get_opcode(instr) == OP_lea && opnd_is_base_disp(op)){
				ret += 4;

			}
			else if (opnd_is_immed(op) ||
				opnd_is_memory_reference(op) ||
				opnd_is_reg(op)){
				ret++;
			}

		}
	}

	return ret;

}

/* prints the trace and empties the instruction buffer */
static void ins_trace(void *drcontext)
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

	uint mem_type;
	uint64 mem_addr;

	opnd_t opnd;

	
#ifdef READABLE_TRACE
	//TODO
    for (i = 0; i < num_refs; i++) {
		
		instr = instr_trace->static_info_instr;
		instr_disassemble_to_buffer(drcontext,instr,disassembly,SHORT_STRING_LENGTH);

		dr_fprintf(data->outfile,"%u",instr_get_opcode(instr));

		dr_fprintf(data->outfile,",%u",calculate_operands(instr,DST_TYPE));
		for(j=0; j<instr_num_dsts(instr); j++){
			get_address(instr_trace, j, DST_TYPE, &mem_type, &mem_addr);
			output_populator_printer(drcontext,instr_get_dst(instr,j),instr,mem_addr,mem_type,NULL);
		}

		dr_fprintf(data->outfile,",%u",calculate_operands(instr,SRC_TYPE));
		for(j=0; j<instr_num_srcs(instr); j++){
			get_address(instr_trace, j, SRC_TYPE, &mem_type, &mem_addr);
			opnd = instr_get_src(instr, j);
			if (instr_get_opcode(instr) == OP_lea && opnd_is_base_disp(opnd)){
				/* four operands here for [base + index * scale + disp] */
				output_populator_printer(drcontext, opnd_create_reg(opnd_get_base(opnd)), instr, mem_addr, mem_type, NULL);
				output_populator_printer(drcontext, opnd_create_reg(opnd_get_index(opnd)), instr, mem_addr, mem_type, NULL);
				output_populator_printer(drcontext, opnd_create_immed_int(opnd_get_scale(opnd),OPSZ_PTR), instr, mem_addr, mem_type, NULL);
				output_populator_printer(drcontext, opnd_create_immed_int(opnd_get_disp(opnd), OPSZ_PTR), instr, mem_addr, mem_type, NULL);
			}
			else{
				output_populator_printer(drcontext, opnd, instr, mem_addr, mem_type, NULL);
			}
		}
		//dr_printf("%u,%u\n", instr_trace->eflags, instr_trace->pc);
		dr_fprintf(data->outfile,",%u,%u\n",instr_trace->eflags,instr_trace->pc);
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

		for(j=0; j<instr_num_srcs(instr); j++){
			output_populator_printer(drcontext,instr_get_src(instr,j),instr,get_address(instr_trace,j,SRC_TYPE),&output->srcs[output->num_srcs]);
			output->num_srcs++;
		}
		
		output->eflags = instr_trace->eflags;

		++instr_trace;

	}

	dr_write_file(data->outfile,data->output_array,num_refs * sizeof(output_t));
#endif

	
    memset(data->buf_base, 0, INSTR_BUF_SIZE);
    data->num_refs += num_refs;
    data->buf_ptr   = data->buf_base;
	
}

/* clean_call dumps the memory reference info to the log file */
static void
clean_call_ins_trace(void)
{
    void *drcontext = dr_get_current_drcontext();
    ins_trace(drcontext);
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
    dr_insert_clean_call(drcontext, ilist, where, (void *)clean_call_ins_trace, false, 0);
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


/******************* debug clean calls **********************/

/* these are for informational purposes only; do not worry too much */

static void clean_call_print_regvalues(){

	uint regvalue;
	dr_mcontext_t mc = { sizeof(mc), DR_MC_ALL };
	void * drcontext = dr_get_current_drcontext();

	dr_get_mcontext(drcontext, &mc);
	dr_printf("---------\n");
	regvalue = reg_get_value(DR_REG_XAX, &mc);
	dr_printf("xax - %x\n", regvalue);
	regvalue = reg_get_value(DR_REG_XBX, &mc);
	dr_printf("xbx - %x\n", regvalue);
	regvalue = reg_get_value(DR_REG_XCX, &mc);
	dr_printf("xcx - %x\n", regvalue);
	regvalue = reg_get_value(DR_REG_XDX, &mc);
	dr_printf("xdx - %x\n", regvalue);

}

static void clean_call_mem_stats(reg_t memvalue){

	dr_mem_info_t info;
	uint base;
	uint limit;
	uint reserve;

	void * drcontext = dr_get_current_drcontext();

	dr_switch_to_app_state(drcontext);

	dr_query_memory_ex(memvalue, &info);

	dr_printf("mem - %d, base_pc - %d , size - %d, prot - %d, type - %d\n", memvalue, info.base_pc, info.size, info.prot, info.type);

	__asm{
		mov EAX, FS : [0x04]
			mov[base], EAX
			mov EAX, FS : [0x08]
			mov[limit], EAX
			mov EAX, FS : [0xE0C]
			mov[reserve], EAX
	}

	dr_printf("stack information - %d %d %d\n", base, limit, reserve);

	dr_switch_to_dr_state(drcontext);

}

static void clean_call_teb_info(uint * stack_base, uint * stack_limit, uint * deallocation_stack){

	__asm{
		mov EAX, FS : [0x04]
			mov[stack_base], EAX
			mov EAX, FS : [0x08]
			mov[stack_limit], EAX
			mov EAX, FS : [0xE0C]
			mov[deallocation_stack], EAX
	}
	return;
}






