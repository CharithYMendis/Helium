#include <string.h> /* for memset */
#include <stddef.h> /* for offsetof */
#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "utilities.h"
#include "instrace.h"
#include "debug.h"
#include "inops.h"

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

#define SRC_OPND 1
#define DST_OPND 2
#define NO_OPND  3

/* 
	instrace main structure  
*/

typedef struct _instr_trace_t {

	static_info_t * static_info_instr;
	uint mem_addr_src;						//src memory address
	uint mem_addr_dst;						//dst memory address

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
static void code_cache_init(void);
static void code_cache_exit(void);
static static_info_t * static_info_instrumentation(void * drcontext, instr_t* instr, uchar * done, uchar * phase,
								char * position);
static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               static_info_t * static_info, char * position);

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

	dr_printf("%d %s %s %d\n",client_arg->filter_mode,client_arg->folder,client_arg->in_filename,client_arg->static_info_size);

	return true;
}

void instrace_init(client_id_t id, char * arguments)
{

	file_t in_file;
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

	global_static_info = (static_info_t * )dr_global_alloc(sizeof(static_info_t)*client_arg->static_info_size);
	global_static_array_size = client_arg->static_info_size;
	global_static_ptr = 0;

	mutex = dr_mutex_create();
    tls_index = drmgr_register_tls_field();
    DR_ASSERT(tls_index != -1);
    code_cache_init();

	for(i=OP_FIRST;i<=OP_LAST; i++){
		opcode_missed[i] = false;
	}
	

}

void instrace_exit_event()
{

	int i;

	for(i=OP_FIRST; i<=OP_LAST; i++){
		if(opcode_missed[i]){
			dr_printf("%s\n",decode_opcode_name(i));
		}
	}

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
    DR_ASSERT(len > 0);
    len += dr_snprintf(logname + len,
                      (sizeof(logname) - len)/sizeof(logname[0]),
                      "%s.instrace.%d.log", dr_get_application_name(),dr_get_thread_id(drcontext));
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
	uchar * done;
	uchar * phase;
	uchar * dest_src;
	char * pos;

	done = (uchar *)dr_global_alloc(sizeof(unsigned char));
	phase = (uchar *)dr_global_alloc(sizeof(uchar));
	//dest_src = (uchar *)dr_global_alloc(sizeof(uchar)*2);
	pos = (char *)dr_global_alloc(sizeof(char)*2);

	DR_ASSERT(done != NULL);
	DR_ASSERT(phase != NULL);
	//DR_ASSERT(dest_src != NULL);
	DR_ASSERT(pos != NULL);

	//dest_src[0] = NO_OPND;
	//dest_src[1] = NO_OPND;
	pos[0] = -1;
	pos[1] = -1;


	*done = false;
	*phase = 0;

	if(filter_from_list(head,instrlist_first(bb),client_arg->filter_mode)){
		while(*done == false){
			//dr_printf("entering static instrumentation\n");
			instr_info = static_info_instrumentation(drcontext, instr, done, phase, pos);
			if(instr_info != NULL){ /* we may filter out the branch instructions */
#ifndef DEBUG_STATIC
				dynamic_info_instrumentation(drcontext,bb,instr,instr_info, pos);		
#endif
			}
		}
	}


	dr_global_free(done,sizeof(uchar));
	dr_global_free(phase,sizeof(uchar));
	//dr_global_free(dest_src,sizeof(uchar)*2);
	dr_global_free(pos,sizeof(char)*2);


	return DR_EMIT_DEFAULT;
			
}

void opnd_populator(void * drcontext, opnd_t opnd,operand_t * static_info, instr_t * instr, uchar * is_memory){


	reg_id_t reg;
	uint reg_size;

	*is_memory = false;

	if(opnd_is_reg(opnd)){
		
		reg = opnd_get_reg(opnd);
		reg_size = opnd_size_in_bytes(reg_get_size(reg));
		static_info->width = reg_size;

		SET_TYPE(static_info->type,REG_TYPE);
		static_info->location = reg;
		
	
	}
	else if(opnd_is_immed(opnd)){
		
		DR_ASSERT(opnd_is_immed_float(opnd) == false);

		/* floating point immediates can be supported by having a byte array with memcpy - not currently supported */
		SET_TYPE(static_info->type,IMM_TYPE);
		static_info->location = opnd_get_immed_int(opnd);
		static_info->width = opnd_size_in_bytes(opnd_get_size(opnd));

	}
	else if(opnd_is_memory_reference(opnd)){

		SET_TYPE(static_info->type,MEM_TYPE);
		static_info->width = drutil_opnd_mem_size_in_bytes(opnd,instr);
		*is_memory = true;

	}

}


static static_info_t * static_info_instrumentation(void * drcontext, instr_t* instr, uchar * done, uchar * phase,
								 char * position){
	/*
		for each src and dest add the information accordingly
		this should return canonicalized static info about an instruction; breaking down any complex instructions if necessary

		1) check whether this instruction needs to be instrumented
		2) if yes, then get a location and then proceed to instrument -> return the struct
		3) if no, return null

	*/

	/* main variables */
	static_info_t * static_info = NULL;
	per_thread_t * data = drmgr_get_tls_field(drcontext,tls_index);

	/* working variables - need for intermediate computations */
	opnd_t operand;
	reg_id_t reg;
	uint reg_size;
	int opcode;
	uchar is_memory;
	
	/* loop variables */
	int i;

	//dr_printf("static instrumentation\n");

	/* 1) */



	opcode = instr_get_opcode(instr);

	/* check whether this instr needs instrumentation - check for ones to skip and skip if */
	/*switch(opcode){
	case OP_jmp:
		*done = true;
		return static_info;
	default:
	}*/

	/* reverse filter - check for once to instrument proceed if */
	switch(opcode){
#ifndef DEBUG_STATIC
	case OP_push:
	case OP_pop:
	case OP_add:
	case OP_or:
	case OP_and:
	case OP_xor:
#else
	//case OP_inc:
#endif
	//	break;
	default:
		opcode_missed[opcode] = true;
		*done = true;
		return static_info;
	}


#ifdef DEBUG_STATIC
	//debug_static_info(drcontext,instr,data->log);
	*done = true;
	return static_info;
#endif
	
	


	/* 2) */

	/* acquire the global array ptr for storing static information for the given instruction */
	dr_mutex_lock(mutex);

	global_static_ptr++;
	DR_ASSERT(global_static_ptr < global_static_array_size);
	static_info = &global_static_info[global_static_ptr];

	dr_mutex_unlock(mutex);

#ifdef DEBUG_INSTRACE
	/* we will disassemble the instruction here */
	instr_disassemble_to_buffer(drcontext,instr,static_info->disassembly,SHORT_STRING_LENGTH);
	static_info->phase = *phase;
#endif

	/* we will only be instrumentating certain instructions */
	opcode = instr_get_opcode(instr);

	switch(opcode){
	
	case OP_push:
		{
			/* this is like a mov - we only need to store the memory update that is happening */
			/* here the 2nd destination holds the memory location; and the 1st source holds the value to be stored which can also be a memory reference */
			DR_ASSERT(instr_num_dsts(instr) == 2);
			DR_ASSERT(instr_num_srcs(instr) == 2);
			DR_ASSERT(opnd_is_memory_reference(instr_get_dst(instr,1)) == true);

			static_info->dest_num = 1;
			static_info->src_num = 1;
			static_info->operation = OP_MOV;
		
			/* fill up the destination */
			opnd_populator(drcontext,instr_get_dst(instr,1),&(static_info->dst),instr,&is_memory);
			DR_ASSERT(is_memory == true);
			position[0] = 1;
			
			/* fill up the src */
			opnd_populator(drcontext,instr_get_src(instr,0),&(static_info->src[0]),instr,&is_memory);
			if(is_memory)   position[1] = 0;
			*done = true;
			break;
		
		}
	case OP_pop:
		{
			DR_ASSERT(instr_num_dsts(instr) == 2);
			DR_ASSERT(instr_num_srcs(instr) == 2);
			DR_ASSERT(opnd_is_memory_reference(instr_get_src(instr,1)) == true);

			static_info->dest_num = 1;
			static_info->src_num = 1;
			static_info->operation = OP_MOV;

			/* fill up the destination */
			opnd_populator(drcontext,instr_get_dst(instr,0),&(static_info->dst),instr,&is_memory);
			if(is_memory) position[0] = 0;

			/* fill up srcs */
			opnd_populator(drcontext,instr_get_src(instr,1),&(static_info->src[0]),instr,&is_memory);
			DR_ASSERT(is_memory == true);
			position[1] = 1;
			*done = true;
			break;
		}
	case OP_add:
	case OP_or:
	case OP_and:
	case OP_xor:
	case OP_inc:
	case OP_dec:
		{
			DR_ASSERT(instr_num_dsts(instr) == 1);
			DR_ASSERT(instr_num_srcs(instr) == 2);
		
			static_info->dest_num = instr_num_dsts(instr);
			static_info->src_num = instr_num_srcs(instr);
			static_info->operation = OP_ADD;  /* FIXME */

			/* dst */
			opnd_populator(drcontext,instr_get_dst(instr,0),&(static_info->dst),instr,&is_memory);
			if(is_memory) position[0] = 0;

			/* src */
			for(i=0; i<instr_num_srcs(instr); i++){
				opnd_populator(drcontext,instr_get_src(instr,i),&(static_info->src[i]),instr,&is_memory);
				if(is_memory){
					DR_ASSERT(position[1] < 0);
					position[1] = i;
				}
			}
			*done = true;
			break;

		} 
	

	
	}

		return static_info;

	

}

void debug_dymem_addr(){

	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(),tls_index);
	instr_trace_t * trace = (instr_trace_t *)data->buf_base;
	dr_printf("%u %u\n",trace->mem_addr_dst,trace->mem_addr_src);

}

static void dynamic_info_instrumentation(void *drcontext, instrlist_t *ilist, instr_t *where,
               static_info_t * static_info, char * pos)
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

	/* get the stack address */
	ref = opnd_create_null();

	if(pos[0] >= 0){
		ref = instr_get_dst(where,pos[0]);
		DR_ASSERT(opnd_is_memory_reference(ref) == true);
	}
	
	
	if(!opnd_is_null(ref)){ /* we have a memory reference */
		drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);
	}

    /* The following assembly performs the following instructions
     * buf_ptr->static_info_instr = static_info;
     * buf_ptr->mem_addr_stack = ?;
	 * buf_ptr->mem_addr_value = ?;
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
		
		/* buf_ptr->mem_addr_stack = addr; */
		opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, mem_addr_dst));
		opnd2 = opnd_create_reg(reg1);
		instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

	}
	else{
	
		opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, mem_addr_dst));
		opnd2 = OPND_CREATE_INT32(0);
		instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

	}

	/* actual memory operand */
	ref = opnd_create_null();

	if(pos[1] >= 0){	
		ref = instr_get_src(where,pos[1]);
		DR_ASSERT(opnd_is_memory_reference(ref) == true);
	}

	if(!opnd_is_null(ref)){
		drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg1, reg2);

		/* again get the buf ptr */
		drmgr_insert_read_tls_field(drcontext, tls_index, ilist, where, reg2);
		/* Load data->buf_ptr into reg2 */
		opnd1 = opnd_create_reg(reg2);
		opnd2 = OPND_CREATE_MEMPTR(reg2, offsetof(per_thread_t, buf_ptr));
		instr = INSTR_CREATE_mov_ld(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

		/* buf_ptr->mem_addr_value = addr; */
		opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, mem_addr_src));
		opnd2 = opnd_create_reg(reg1);
		instr = INSTR_CREATE_mov_st(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

	}
	else{

		/* buf_ptr->mem_addr_value = addr; */
		opnd1 = OPND_CREATE_MEMPTR(reg2, offsetof(instr_trace_t, mem_addr_src));
		opnd2 = OPND_CREATE_INT32(0);
		instr = INSTR_CREATE_mov_imm(drcontext, opnd1, opnd2);
		instrlist_meta_preinsert(ilist, where, instr);

	}

	//dr_insert_clean_call(drcontext,ilist,where,debug_dymem_addr,false,0);

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
    per_thread_t *data;
    int num_refs;
	instr_trace_t *instr_trace;
	static_info_t * static_info;
#ifdef READABLE_TRACE
    int i;
	int j;
#endif

    data      = drmgr_get_tls_field(drcontext, tls_index);
    instr_trace   = (instr_trace_t *)data->buf_base;
    num_refs  = (int)((instr_trace_t *)data->buf_ptr - instr_trace);

#ifdef READABLE_TRACE
	//TODO
    for (i = 0; i < num_refs; i++) {
		/* print it */
		/* print destination */
		static_info = instr_trace->static_info_instr;
		dr_fprintf(data->log,"%s\n",static_info->disassembly);
		dr_fprintf(data->log,"<%u,",static_info->operation);
		//dr_fprintf(data->log,"%u,%u,",static_info->dst.opnd.width,instr_trace->mem_addr_stack);
		if(static_info->dest_num > 0){
			if(GET_TYPE(static_info->dst.type) == MEM_TYPE){
				dr_fprintf(data->log,"%u,%u,",static_info->dst.width,instr_trace->mem_addr_dst);
			}
			else{
				dr_fprintf(data->log,"%u,%u,",static_info->dst.width,static_info->dst.location);
			}
		}

		for(j = 0; j<static_info->src_num; j++){
			if(GET_TYPE(static_info->src[j].type) == MEM_TYPE){
				dr_fprintf(data->log,"%u,%u,",static_info->src[j].width,instr_trace->mem_addr_src);
			}
			else{
				dr_fprintf(data->log,"%u,%u,",static_info->src[j].width,static_info->src[j].location);
			}
		}
		dr_fprintf(data->log,">\n");

        ++instr_trace;
    }
#else
    dr_write_file(data->log, data->buf_base,
                  (size_t)(data->buf_ptr - data->buf_base));
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




