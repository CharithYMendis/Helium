#include "dr_api.h"
#include "funcwrap.h"
#include "moduleinfo.h"
#include "drwrap.h"
#include "drmgr.h"
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

uint is_within_func = false;
uint thread_id_func = 0;

typedef struct _client_arg_t{
	char filter_filename[MAX_STRING_LENGTH];
} client_arg_t;

typedef struct {
	bool filter_func;
	int nesting;
	file_t logfile;
} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static int tls_index;
static bool file_registered = false;
static uint wrap_thread_id = 0;
static bool dumped = 0;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s", &client_arg->filter_filename) != 1){
		return false;
	}

	return true;
}


/* callbacks for the entire process */
void funcwrap_init(client_id_t id, const char * name, const char * arguments)
{

	file_t in_file;
	char logfilename[MAX_STRING_LENGTH];

	drmgr_init();
	drwrap_init();
	tls_index = drmgr_register_tls_field();

	DR_ASSERT(parse_commandline_args(arguments) == true);
	head = md_initialize();
	if (!dr_file_exists(client_arg->filter_filename)){
		file_registered = false;
	}
	/* we expect the filter file to be of the form for function filtering */
	else{
		file_registered = true;
		in_file = dr_open_file(client_arg->filter_filename, DR_FILE_READ);
		DR_ASSERT(in_file != INVALID_FILE);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
	}

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);
	

}

void funcwrap_exit_event(void)
{

	md_delete_list(head, false);
	dr_global_free(client_arg, sizeof(client_arg_t));
	drmgr_unregister_tls_field(tls_index);
	if (log_mode){
		dr_close_file(logfile);
	}
	drwrap_exit();
	drmgr_exit();

}

/* callbacks for threads */
void funcwrap_thread_init(void *drcontext){
	
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

	

	data->filter_func = false;
	data->nesting = 0;
	drmgr_set_tls_field(drcontext, tls_index, data);

	DEBUG_PRINT("%s - initializing thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}

void
funcwrap_thread_exit(void *drcontext){
	per_thread_t * data;
	data = drmgr_get_tls_field(drcontext, tls_index);
	if (log_mode){
		dr_close_file(data->logfile);
	}
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}

bool should_filter_func(){
	
	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);

	if (!file_registered){
		return true;
	}
	else{
		return data->filter_func;
	}
}

bool should_filter_thread(uint thread_id){
	return (wrap_thread_id == thread_id);
}

static void clean_call(uint pc){

	DEBUG_PRINT("funcwrap - entered the pre-call clean call\n");
	
	if (dumped == 0){
		dr_unlink_flush_region(0, ~((ptr_uint_t)0));
		dumped = 1;
	}
	
	DEBUG_PRINT("funcwrap - entered the pre-call done\n");

	
	

}

dr_emit_flags_t
funcwrap_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{

	instr_t * first = instrlist_first(bb);
	app_pc pc = instr_get_app_pc(first);
	module_data_t * module_data;
	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	module_t * md;
	app_pc offset;
	
	
	if (instr != first || data->nesting != 0){
		return DR_EMIT_DEFAULT;
	}

	module_data = dr_lookup_module(pc);
	data = drmgr_get_tls_field(drcontext, tls_index);

	
	if (module_data != NULL){
		md = md_lookup_module(head, module_data->full_path);
		if (md != NULL){
			offset = pc - module_data->start;
			
			for (int i = 1; i <= md->bbs[0].start_addr; i++){
				if (offset == md->bbs[i].start_addr){
					DEBUG_PRINT("bb instrumenting function\n");
					data->filter_func = true;
					//dr_insert_clean_call(drcontext, bb, instr, clean_call, false, 1, OPND_CREATE_INTPTR(instr_get_app_pc(instr)));
					wrap_thread_id = dr_get_thread_id(drcontext);

					is_within_func = true;
					thread_id_func = dr_get_thread_id(drcontext);


					DEBUG_PRINT("done bb instrumenting function\n");
				}
			}
		}
	}

	

	/*if (data->filter_func){
		instrlist_disassemble(drcontext, instr_get_app_pc(instrlist_first(bb)), bb, logfile);
	}*/

	dr_free_module_data(module_data);


	return DR_EMIT_DEFAULT;

}

static void pre_func_cb(void * wrapcxt, OUT void ** user_data){
	DEBUG_PRINT("funcwrap - pre_func_cb\n");
	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	data->filter_func = true;

	is_within_func = true;

	data->nesting++;
}

static void post_func_cb(void * wrapcxt, void ** user_data){
	DEBUG_PRINT("funcwrap - post_func_cb\n");
	per_thread_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	data->nesting--;
	//dr_unlink_flush_region(0, ~((ptr_uint_t)0));
	DR_ASSERT(data->nesting >= 0); 
	if (data->nesting == 0){
		data->filter_func = false;
		is_within_func = false;
	}
	DEBUG_PRINT("funcwrap - post_func_cb done \n");
	

}


void funcwrap_module_load(void * drcontext, module_data_t * module, bool loaded){


	module_t * md = md_lookup_module(head, module->full_path);
	int i = 0;
	app_pc address;	
	
	if (md != NULL){
		for (int i = 1; i <= md->bbs[0].start_addr; i++){
			address = md->bbs[i].start_addr + module->start;
			DEBUG_PRINT("funcwrap: %s module %x function wrapping\n", md->module, address);
			drwrap_wrap(address, pre_func_cb, post_func_cb);
		}
	}
	

}

void funcwrap_module_unload(void * drcontext, module_data_t * module){

}



