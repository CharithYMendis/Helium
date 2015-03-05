#include "dr_api.h"
#include "defines.h"
#include "moduleinfo.h"
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
	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

typedef struct {
	file_t  logfile;
	file_t  outfile;
} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static int tls_index;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d", &client_arg->filter_filename,
		&client_arg->filter_mode) != 2){
		return false;
	}

	return true;
}


/* callbacks for the entire process */
void misc_init(client_id_t id, const char * name, const char * arguments)
{

	char logfilename[MAX_STRING_LENGTH];
	file_t in_file;

	drmgr_init();
	tls_index = drmgr_register_tls_field();
	DR_ASSERT(parse_commandline_args(arguments) == true);
	head = md_initialize();

	if (client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename, DR_FILE_READ);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
	}

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);

}

void misc_exit_event(void)
{

	md_delete_list(head, false);
	dr_global_free(client_arg, sizeof(client_arg_t));
	drmgr_unregister_tls_field(tls_index);
	if (log_mode){
		dr_close_file(logfile);
	}
	drmgr_exit();

}

/* callbacks for threads */
void misc_thread_init(void *drcontext){
	per_thread_t * data;

	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));
	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_index, data);

}

void
misc_thread_exit(void *drcontext){
	per_thread_t * data;
	data = drmgr_get_tls_field(drcontext, tls_index);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));
	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}


/* callbacks for basic blocks */
dr_emit_flags_t
misc_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	return DR_EMIT_DEFAULT;

}

dr_emit_flags_t
misc_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{
	/*if (instr == instrlist_first(bb)){
		instrlist_disassemble(drcontext, instr_get_app_pc(instrlist_first(bb)), bb, logfile);
	}*/

	/*int i = 0;
	opnd_t opnd;

	for (i = 0; i < instr_num_srcs(instr); i++){
		opnd = instr_get_src(instr, i);
		if (opnd_is_memory_reference(opnd)){
			if (opnd_is_abs_addr(opnd)){
				//LOG_PRINT(logfile, "abs_addr,%d,%d,%d,%d,%d\n", opnd_get_addr(opnd), opnd_get_base(opnd), opnd_get_index(opnd), opnd_get_scale(opnd), opnd_get_disp(opnd));
			}
			else if (opnd_is_base_disp(opnd)){
				//LOG_PRINT(logfile, "base_disp,%d,%d,%d,%d\n", opnd_get_base(opnd), opnd_get_index(opnd), opnd_get_scale(opnd), opnd_get_disp(opnd));
			}
			else{
				DR_ASSERT(false);
				//LOG_PRINT(logfile, "may be relative addr, %d,%d,%d,%d\n", opnd_get_base(opnd), opnd_get_index(opnd), opnd_get_scale(opnd), opnd_get_disp(opnd));
			}
		}
		
	}*/

	
	return DR_EMIT_DEFAULT;
}


dr_emit_flags_t
misc_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}




