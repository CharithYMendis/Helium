#include "dr_api.h"


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
	<fill_this_out>
} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static int tls_index;


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d", &client_arg->in_filename,
		&client_arg->filter_mode) != 2){
		return false;
	}

	return true;
}


/* callbacks for the entire process */
void <client_name>_init(client_id_t id, const char * arguments)
{

	file_t in_file;

	drmgr_init();

	DR_ASSERT(parse_commandline_args(arguments) == true);
	head = md_initialize();

	if (client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->in_filename, DR_FILE_READ);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
	}

}

void <client_name>_exit_event(void)
{

	md_delete_list(head, false);
	dr_global_free(client_arg, sizeof(client_arg_t));

	drmgr_exit();

}

/* callbacks for threads */
void <client_name>_thread_init(void *drcontext){
	per_thread_t * data;
	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_index, data);

}

void
<client_name>_thread_exit(void *drcontext){
	per_thread_t * data;
	data = drmgr_get_tls_field(drcontext, tls_index);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));

}


/* callbacks for basic blocks */
dr_emit_flags_t
<client_name>_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	return DR_EMIT_DEFAULT;

}

dr_emit_flags_t
<client_name>_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{
	return DR_EMIT_DEFAULT;
}


dr_emit_flags_t
<client_name>_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}




