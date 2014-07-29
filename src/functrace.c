#include "functrace.h"
#include "dr_api.h"
#include "dr_stack.h"
#include "utilities.h"


/* for each client following functions may be implemented

init_func_t init_func;
exit_func_t process_exit;

drmgr_analysis_cb_t analysis_bb;
drmgr_insertion_cb_t instrumentation_bb;
drmgr_xform_cb_t app2app_bb;

thread_func_t thread_init;
thread_func_t thread_exit;


*/

typedef struct _client_arg_t{
	char in_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

typedef struct {

	stack_t * stack;

	bool new_func;
	function_t * func;

} per_thread_t;

static client_arg_t * client_arg;
static module_t * all_modules;
static function_t * functions;
static int tls_index;


/* callbacks for the entire process */
void functrace_init(client_id_t id, const char * arguments)
{

	file_t in_file;
	drmgr_init();
	all_modules = md_initialize();


}

void functrace_exit_event(void)
{

	md_delete_list(all_modules, false);
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

	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	stack_init(&data->stack, INIT_CAPACITY, delete_function_t);
	data->new_func = false;
	data->func = NULL;

	drmgr_set_tls_field(drcontext, tls_index, data);

}

void
functrace_thread_exit(void *drcontext){

	per_thread_t * data;
	
	data = drmgr_get_tls_field(drcontext, tls_index);
	stack_delete(data->stack);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

}


/* callbacks for basic blocks */
dr_emit_flags_t
functrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	module_data_t * module_data;
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);

	/* if a new func add to the top of the stack */
	if (data->new_func){
		stack_push(data->stack, data->func);
		data->new_func = false;
	}

	/* enter the module data for this module */
	module_data = dr_lookup_module(instr_get_app_pc(instrlist_first(bb)));
	if (module_data != NULL){
		md_add_module(all_modules, module_data->full_path, 0);
		dr_free_module_data(module_data);
	}

	return DR_EMIT_DEFAULT;

}

static void
at_call(app_pc instr_addr, app_pc target_addr){

	void * drcontext = dr_get_current_drcontext(); 
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	
	module_data_t * module_data;
	module_t * mdinfo;
	app_pc offset;


	data->func = (function_t *)dr_global_alloc(sizeof(function_t));

	module_data = dr_lookup_module(instr_addr);

	if (module_data != NULL){

		data->func->module = md_lookup_module(all_modules, module_data->full_path);
		data->func->pc = target_addr - module_data->start;
		data->func->is_recursive = false;
		dr_free_module_data(module_data);
		data->new_func = true;
	}
	/* else is dynamically generated code - so we will disregard it for now */
	

}

static void 
at_return(app_pc instr_addr, app_pc target_address){

	void * drcontext = dr_get_current_drcontext();
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index); 

	function_t * func = stack_pop(data->stack);
	if (func != NULL){
		dr_global_free(func, sizeof(function_t));
	}
	else{
		dr_printf("unmatched ret to a call, may be a tail call please see\n");
		dr_abort();
	}
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

}


dr_emit_flags_t
functrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}


function_t * get_current_function(){

	void * drcontext = dr_get_current_drcontext();
	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);

	return stack_peek(data->stack);

}




