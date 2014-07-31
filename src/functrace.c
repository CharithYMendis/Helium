#include "functrace.h"
#include "moduleinfo.h"
#include "defines.h"
#include "utilities.h"
#include "drmgr.h"
#include "dr_stack.h"



/* for each client following functions may be implemented

init_func_t init_func;
exit_func_t process_exit;

drmgr_analysis_cb_t analysis_bb;
drmgr_insertion_cb_t instrumentation_bb;
drmgr_xform_cb_t app2app_bb;

thread_func_t thread_init;
thread_func_t thread_exit;


*/

/*
tail recursion not handled
*/

typedef struct _client_arg_t{
	char in_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

typedef struct {

	stack_t * stack;
	file_t file; 

	//bool new_func;
	//function_t * func;

	//drvector_t * allocated_funcs;  /* vector for collecting the deleted functions */

	bool jmp_to_outside;

} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static function_t * functions;
static int tls_index;

static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d", 
		&client_arg->in_filename,
		&client_arg->filter_mode) != 2){
		return false;
	}

	return true;
}



/* callbacks for the entire process */
void functrace_init(client_id_t id, const char * arguments)
{

	file_t in_file;
	file_t out_file;

	DEBUG_PRINT("functrace - initializing functrace\n");
	drmgr_init();

	DR_ASSERT(parse_commandline_args(arguments) == true);
	tls_index = drmgr_register_tls_field();
	head = md_initialize();
	out_file = dr_open_file("C:\\Charith\\Dropbox\\Research\\development\\exalgo\\log\\out_file.txt", DR_FILE_WRITE_OVERWRITE);


	if (client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->in_filename, DR_FILE_READ);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
		md_print_to_file(head,out_file);
		dr_close_file(out_file);
	}


}

void functrace_exit_event(void)
{


	DEBUG_PRINT("functrace - exiting\n");
	md_delete_list(head, false);
	dr_global_free(client_arg, sizeof(client_arg_t));

	drmgr_exit();

}

function_t * get_current_function(void * drcontext){

	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	return stack_peek(data->stack);

}

void delete_function_t(void * func){

	dr_global_free(func, sizeof(function_t));

}

/* callbacks for threads */
void functrace_thread_init(void *drcontext){

#define INIT_CAPACITY	100

	per_thread_t * data;
	char filename[MAX_STRING_LENGTH];

	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));

	
	dr_snprintf(filename, MAX_STRING_LENGTH, "C:\\Charith\\Dropbox\\Research\\development\\exalgo\\log\\functrace_%d.txt", dr_get_thread_id(drcontext));
	data->file = dr_open_file(filename, DR_FILE_WRITE_OVERWRITE);
	data->jmp_to_outside = false;

	dr_fprintf(data->file, "functrace - initializing thread %d\n", dr_get_thread_id(drcontext));
	
	DR_ASSERT_MSG(stack_init(&(data->stack), INIT_CAPACITY, delete_function_t),"vector allocation failure\n");


	drmgr_set_tls_field(drcontext, tls_index, data);

	dr_fprintf(data->file,"functrace - initializing thread done %d\n",dr_get_thread_id(drcontext));

}



void
functrace_thread_exit(void *drcontext){

	per_thread_t * data;
	
	data = drmgr_get_tls_field(drcontext, tls_index);

	stack_delete(data->stack);

	dr_fprintf(data->file, "functrace - exiting thread done %d\n", dr_get_thread_id(drcontext));
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	

}

void print_all(stack_t * stack){


	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);

	int i = 0;
	function_t * func;

	for (i = 0; i <= stack->head; i++){
		func = drvector_get_entry(stack->vector, i);
		dr_fprintf(data->file,"%x,", func->start_addr);
	}
	dr_fprintf(data->file,"\n");

}


/* clean calls */

static void
at_call(app_pc instr_addr,app_pc target_addr){

	void * drcontext = dr_get_current_drcontext(); 
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	
	module_data_t * target_module_data;
	module_data_t * module_data;
	function_t * func;

	app_pc offset;


	module_data = dr_lookup_module(instr_addr);
	target_module_data = dr_lookup_module(target_addr);

	if (target_module_data != NULL){

		if (filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)){

			func = (function_t *)dr_global_alloc(sizeof(function_t));
			func->start_addr = (int64)target_addr - (int64)target_module_data->start;
			func->end_addr = 0; /* will be updated when the function returns */
			func->is_recursive = false;

			stack_push(data->stack, func);

			dr_fprintf(data->file,"push:%d\n", data->stack->head);
			dr_fprintf(data->file, "at_call_target:%s,%x\n", target_module_data->full_path, func->start_addr);

			if (module_data != NULL){
				offset = instr_addr - module_data->start;
				dr_fprintf(data->file, "at_call_instr:%s,%x\n", module_data->full_path, offset);
			}

		}

	}
	/* else is dynamically generated code - so we will disregard it for now */

	dr_free_module_data(module_data);
	dr_free_module_data(target_module_data);
	

}

static void 
at_return(app_pc instr_addr, app_pc target_addr){

	void * drcontext = dr_get_current_drcontext();
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index); 
	function_t * func;
	module_data_t * module_data;
	module_data_t * target_module_data;

	app_pc offset;

	module_data = dr_lookup_module(instr_addr);
	target_module_data = dr_lookup_module(target_addr);

	if (module_data != NULL){

		if (

			( (target_module_data != NULL)
			&& !filter_from_module_name(head, module_data->full_path, client_arg->filter_mode)
			&& filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)
			&& data->jmp_to_outside) 

			|| 
			
			(filter_from_module_name(head, module_data->full_path, client_arg->filter_mode))
			
			){

			func = stack_pop(data->stack);


			dr_fprintf(data->file,"pop:%d\n", data->stack->head);
			if (target_module_data != NULL){
				offset = target_addr - target_module_data->start;
				dr_fprintf(data->file, "at_ret_target:%s,%x\n", target_module_data->full_path, offset);
			}

			DR_ASSERT_MSG(func != NULL, "unmatched ret to a call, may be a tail call please see\n");

			offset = instr_addr - module_data->start;
			dr_fprintf(data->file, "at_ret_instr:%s,%x,%x\n", module_data->full_path, offset, func->start_addr);

			

			//dr_global_free(func, sizeof(function_t));
			data->jmp_to_outside = false;
		}
	}

	dr_free_module_data(module_data);
	dr_free_module_data(target_module_data);

}

static void
at_cti(app_pc instr_addr, app_pc target_addr){

	void * drcontext = dr_get_current_drcontext();
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	module_data_t * module_data;
	module_data_t * target_module_data;

	app_pc offset;

	module_data = dr_lookup_module(instr_addr);
	target_module_data = dr_lookup_module(target_addr);

	if (module_data != NULL && target_module_data != NULL){

		/* the jump address is outside the module */
		if (filter_from_module_name(head, module_data->full_path, client_arg->filter_mode) 
			&& !filter_from_module_name(head,target_module_data->full_path,client_arg->filter_mode)){

			data->jmp_to_outside = true;
			
		}

	}

	dr_free_module_data(module_data);
	dr_free_module_data(target_module_data);

}


/* callbacks for basic blocks */
dr_emit_flags_t
functrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	return DR_EMIT_DEFAULT;

}

dr_emit_flags_t
functrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{

	
	if (instr_is_call_direct(instr)) {
		dr_insert_call_instrumentation(drcontext, bb, instr, (app_pc)at_call);
	}
	else if (instr_is_call_indirect(instr)) {
		dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)at_call,
			SPILL_SLOT_1);
	}

	else if (instr_is_return(instr)) {
		dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)at_return,
			SPILL_SLOT_1);
	}
	else if (instr_is_mbr(instr)){
		dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)at_cti,
			SPILL_SLOT_1);
	}
	else if (instr_is_ubr(instr)){
		dr_insert_ubr_instrumentation(drcontext, bb, instr, (app_pc)at_cti);
	}
	


	return DR_EMIT_DEFAULT;

}


dr_emit_flags_t
functrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}







