#include "functrace.h"
#include "moduleinfo.h"
#include "defines.h"
#include "utilities.h"
#include "drmgr.h"
#include "stack.h"



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
	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

typedef struct {

	stack_t * stack;
	file_t logfile; 
	bool jmp_to_outside;
	int jmp_cnt;
	bool call_to_outside;


} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static function_t * functions;
static int tls_index;
static bool init = false;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d", 
		&client_arg->filter_filename,
		&client_arg->filter_mode) != 2){
		return false;
	}

	return true;
}



/* callbacks for the entire process */
void functrace_init(client_id_t id, const char * name, const char * arguments)
{

	file_t in_file;
	char logfilename[MAX_STRING_LENGTH];

	drmgr_init();

	DR_ASSERT(parse_commandline_args(arguments) == true);
	tls_index = drmgr_register_tls_field();
	head = md_initialize();

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);
	

	if (client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename, DR_FILE_READ);
		DR_ASSERT(in_file != INVALID_FILE);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
	}

	init = true;


}

void functrace_exit_event(void)
{

	if (log_mode){
		dr_close_file(logfile);
	}

	md_delete_list(head, false);
	dr_global_free(client_arg, sizeof(client_arg_t));
	drmgr_unregister_tls_field(tls_index);
	drmgr_exit();


}

function_t * get_current_function(void * drcontext){

	per_thread_t * data = drmgr_get_tls_field(drcontext, tls_index);
	if (init){
		return stack_peek(data->stack);
	}
	else{
		return NULL;
	}
}

void delete_function_t(void * func){

	dr_global_free(func, sizeof(function_t));

}

/* callbacks for threads */
void functrace_thread_init(void *drcontext){

#define INIT_CAPACITY	100

	per_thread_t * data;
	char logfilename[MAX_STRING_LENGTH];
	char thread_id[MAX_STRING_LENGTH];

	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));

	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));

	if (log_mode){
		dr_snprintf(thread_id, MAX_STRING_LENGTH, "%d", dr_get_thread_id(drcontext));
		populate_conv_filename(logfilename, logdir, ins_pass_name, thread_id);
		data->logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	data->jmp_to_outside = false;
	data->call_to_outside = false;
	data->jmp_cnt = 0;
	
	DR_ASSERT(stack_init(&(data->stack), INIT_CAPACITY, delete_function_t));

	drmgr_set_tls_field(drcontext, tls_index, data);

	DEBUG_PRINT("%s - initializing thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}



void
functrace_thread_exit(void *drcontext){

	per_thread_t * data;
	
	data = drmgr_get_tls_field(drcontext, tls_index);

	//stack_delete(data->stack);

	if (log_mode){
		dr_close_file(data->logfile);
	}

	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

	

}

void print_all(stack_t * stack){


	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);

	int i = 0;
	function_t * func;

	for (i = 0; i <= stack->head; i++){
		func = drvector_get_entry(stack->vector, i);
		LOG_PRINT(data->logfile,"%x,", func->start_addr);
	}
	LOG_PRINT(data->logfile,"\n");

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

	if (module_data == NULL) dr_printf("instr null\n");
	if (target_module_data == NULL) dr_printf("target null\n");

	if (target_module_data != NULL && module_data != NULL){

		/* call in after a jmp go back to normal tracking */
		if (!filter_from_module_name(head, module_data->full_path, client_arg->filter_mode)
			&& filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode) 
			&& data->jmp_cnt){

			LOG_PRINT(data->logfile,"call in after jmp\n");
			data->jmp_to_outside = false;
		}

		

		if (filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)){

			func = (function_t *)dr_global_alloc(sizeof(function_t));
			func->start_addr = (int64)target_addr - (int64)target_module_data->start;
			func->end_addr = 0; /* will be updated when the function returns */
			func->is_recursive = false;

			stack_push(data->stack, func);

			LOG_PRINT(data->logfile, "push:%d\n", data->stack->head);
			LOG_PRINT(data->logfile, "at_call_target:%s,%x\n", target_module_data->full_path, func->start_addr);

			if (module_data != NULL){
				offset = instr_addr - module_data->start;
				LOG_PRINT(data->logfile, "at_call_instr:%s,%x\n", module_data->full_path, offset);
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
	bool handle_jmp;

	module_data = dr_lookup_module(instr_addr);
	target_module_data = dr_lookup_module(target_addr);

	if (module_data == NULL) dr_printf("instr null\n");
	if (target_module_data == NULL) dr_printf("target null\n");

	if (module_data != NULL && target_module_data != NULL){

		/* return back to non filtered go back to jump mode */
		
		if (filter_from_module_name(head, module_data->full_path, client_arg->filter_mode)
			&& !filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)
			&& data->jmp_cnt){
			LOG_PRINT(data->logfile, "ret out after jmp\n");
			data->jmp_to_outside = true;

		}

		handle_jmp = !filter_from_module_name(head, module_data->full_path, client_arg->filter_mode)
			&& filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)
			&& data->jmp_to_outside;

		if (handle_jmp || (filter_from_module_name(head, module_data->full_path, client_arg->filter_mode))){

			func = stack_pop(data->stack);


			LOG_PRINT(data->logfile, "pop:%d\n", data->stack->head);
			if (target_module_data != NULL){
				offset = target_addr - target_module_data->start;
				LOG_PRINT(data->logfile, "at_ret_target:%s,%x\n", target_module_data->full_path, offset);
			}

			DR_ASSERT(func != NULL);

			offset = instr_addr - module_data->start;
			LOG_PRINT(data->logfile, "at_ret_instr:%s,%x,%x\n", module_data->full_path, offset, func->start_addr);

			//dr_global_free(func, sizeof(function_t));
			if (handle_jmp){
				data->jmp_to_outside = false;
				data->jmp_cnt--;
			}
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

	if (module_data == NULL) dr_printf("instr null\n");
	if (target_module_data == NULL) dr_printf("target null\n");

	if (module_data != NULL && target_module_data != NULL){

		/* the jump address is outside the module */
		if (filter_from_module_name(head, module_data->full_path, client_arg->filter_mode) 
			&& !filter_from_module_name(head,target_module_data->full_path,client_arg->filter_mode)){

			data->jmp_to_outside = true;
			data->jmp_cnt++;

			offset = instr_addr - module_data->start;

			//DEBUG_PRINT("out jmp\n");
			LOG_PRINT(data->logfile,"out jmp\n");
			LOG_PRINT(data->logfile, "at_cti_instr:%s,%x\n", module_data->full_path, offset);
			offset = target_addr - target_module_data->start;
			LOG_PRINT(data->logfile, "at_cti_target:%s,%x\n", target_module_data->full_path, offset);
			
		}

		else if (!filter_from_module_name(head, module_data->full_path, client_arg->filter_mode)
			&& filter_from_module_name(head, target_module_data->full_path, client_arg->filter_mode)){
			
			DEBUG_PRINT("in jmp %d\n",data->jmp_cnt);

			data->jmp_cnt--;
			if (data->jmp_cnt){
				data->jmp_to_outside = false;
			}
			
			DR_ASSERT(data->jmp_cnt >= 0);

			
			LOG_PRINT(data->logfile,"in jmp\n");
			offset = instr_addr - module_data->start;
			LOG_PRINT(data->logfile, "at_cti_instr:%s,%x\n", module_data->full_path, offset);
			offset = target_addr - target_module_data->start;
			LOG_PRINT(data->logfile, "at_cti_target:%s,%x\n", target_module_data->full_path, offset);

		}

	}

	dr_free_module_data(module_data);
	dr_free_module_data(target_module_data);

}


dr_emit_flags_t
functrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
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

	module_data_t * data;
	char disassembly[250];
	file_t out_file;

	DR_ASSERT(instr_ok_to_mangle(instr));

	if (instr_ok_to_mangle(instr)){


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
		else if (instr_is_near_ubr(instr)){
			dr_insert_ubr_instrumentation(drcontext, bb, instr, (app_pc)at_cti);
		}
		else if (instr_is_ubr(instr)){
			dr_printf("WARNING : far cti detected\n");
		}
		else if (instr_is_far_cti(instr)){
			dr_printf("WARNING : far cti detected\n");
		}


	}


	return DR_EMIT_DEFAULT;

}










