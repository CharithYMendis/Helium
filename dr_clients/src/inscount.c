#include "inscount.h"
#include "dr_api.h"
#include "drmgr.h"
#include "utilities.h"
#include "moduleinfo.h"
#include "defines.h"

#define SHOW_RESULTS

#ifdef WINDOWS
# define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
# define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#define NULL_TERMINATE(buf) buf[(sizeof(buf)/sizeof(buf[0])) - 1] = '\0'

static bool parse_commandline_args (const char * args);

typedef struct _client_arg_t{
	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

static client_arg_t * client_arg;
static module_t * head;

/* we only have a global count */
static uint64 global_count;
static uint bbcount = 0;
/* A simple clean call that will be automatically inlined because it has only
 * one argument and contains no calls to other functions.
 */

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];

static void inscount(uint num_instrs) { global_count += num_instrs; }

static bool parse_commandline_args (const char * args) {


	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));


	if(dr_sscanf(args,"%s %d",&client_arg->filter_filename,
						   &client_arg->filter_mode)!=2){
		return false;
	}


	
	return true;
}

void inscount_init(client_id_t id, const char * name,  const char * arguments)
{

	file_t in_file;
	char logfilename[MAX_STRING_LENGTH];

    drmgr_init();
	
	global_count = 0;

	DR_ASSERT(parse_commandline_args(arguments) == true);
	head = md_initialize();


	if(client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename,DR_FILE_READ);
		DR_ASSERT(in_file != INVALID_FILE);
		md_read_from_file(head,in_file,false);
		dr_close_file(in_file);
	}

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);
	


}

void inscount_exit_event(void)
{
#ifdef SHOW_RESULTS
    char msg[512];
    int len;
    len = dr_snprintf(msg, sizeof(msg)/sizeof(msg[0]),
                      "process name - %s\nInstrumentation results: %llu instructions executed - %d bbcount\n"
                      ,dr_get_application_name(),global_count,bbcount);
    DR_ASSERT(len > 0);
    NULL_TERMINATE(msg);
    DISPLAY_STRING(msg);
#endif /* SHOW_RESULTS */
	
	md_delete_list(head,false);
	dr_global_free(client_arg, sizeof(client_arg_t));

	if (log_mode){
		dr_close_file(logfile);
	}

	drmgr_exit();

}

dr_emit_flags_t 
	inscount_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data){
	
	return DR_EMIT_DEFAULT;
	
}

dr_emit_flags_t
inscount_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data)
{

	instr_t *first = instrlist_first(bb);
	uint num_instrs = 0;

	if(instr != first) 
		return DR_EMIT_DEFAULT;

	if(filter_from_list(head,instr,client_arg->filter_mode)){
		for(instr = first ; instr!=NULL ; instr = instr_get_next(instr)){
			num_instrs++;
		}
		bbcount++;
	}

		
	if(num_instrs > 0){
		dr_insert_clean_call(drcontext, bb, instrlist_first(bb),
							(void *)inscount, false /* save fpstate */, 1,
							OPND_CREATE_INT32(num_instrs));
	}


	



			
	return DR_EMIT_DEFAULT;
}



