#include <stdio.h>
#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"

#include "defines.h"
#include "stack.h"

/* includes from by instrumentation passes */
#include "profile_global.h"
#include "cpuid.h"
#include "memtrace.h"
#include "inscount.h"
#include "instrace.h"
#include  "functrace.h"
#include "funcwrap.h"
#include "utilities.h"
#include "memdump.h"
#include "funcreplace.h"
#include "misc.h"

#define ARGUMENT_LENGTH 20

typedef void(*thread_func_t) (void * drcontext);
typedef void(*init_func_t) (client_id_t id, const char * name, const char * arguments);
typedef void(*exit_func_t) (void);
typedef void(*module_load_t) (void * drcontext, const module_data_t * info, bool loaded);
typedef void(*module_unload_t) (void * drcontext, const module_data_t * info);

typedef struct _instrumentation_pass_t {

	char * name;
	init_func_t init_func;
	drmgr_analysis_cb_t analysis_bb;
	drmgr_insertion_cb_t instrumentation_bb;
	drmgr_xform_cb_t app2app_bb;
	drmgr_priority_t priority;
	thread_func_t thread_init;
	thread_func_t thread_exit;
	exit_func_t process_exit;
	module_load_t module_load;
	module_unload_t module_unload;


} instrumentation_pass_t;

typedef struct _cmdarguments_t {

	char name[MAX_STRING_LENGTH];
	char arguments[10 * MAX_STRING_LENGTH];

} cmdarguments_t;

//prototypes
static void setupInsPasses();
static void doCommandLineArgProcessing(client_id_t id);
static void process_exit_routine_call(void);

//allocate statically enough space
static instrumentation_pass_t ins_pass[ARGUMENT_LENGTH];
static cmdarguments_t arguments[ARGUMENT_LENGTH];
static int pass_length = 0;
static int argument_length = 0;

char logdir[MAX_STRING_LENGTH];
bool debug_mode = false;
bool log_mode = false;
file_t global_logfile;

static char global_logfilename[MAX_STRING_LENGTH];
static char exec[MAX_STRING_LENGTH];

bool nudge_instrument = false;


void nudge_event(void * drcontext, uint64 argument){

	nudge_instrument = argument;
	//dr_messagebox("nudged - %d\n", argument);
	//dr_unlink_flush_region(0, ~((ptr_uint_t)0));

}

DR_EXPORT void
dr_init(client_id_t id)
{

	int i = 0;
	int j = 0;

	drmgr_init();
	dr_enable_console_printing();

	setupInsPasses();
	doCommandLineArgProcessing(id);


	DEBUG_PRINT("argument length - %d\n", argument_length);
	for (i = 0; i < argument_length; i++){
		DEBUG_PRINT("\"%s - %s\"\n", arguments[i].name, arguments[i].arguments);
	}

	dr_register_nudge_event(nudge_event, id);

	/*if (log_mode){
		populate_conv_filename(global_logfilename, logdir, "global", NULL);
		global_logfile = dr_open_file(global_logfilename, DR_FILE_WRITE_OVERWRITE);
		DR_ASSERT(global_logfile != INVALID_FILE);
	}*/

	//dr_messagebox("client id - %d\n", id);
	DEBUG_PRINT("%s is starting\n", dr_get_application_name());
	/* if you are using it only for photoshop do no instrument other exes */
	if (strcmp(exec, "photoshop") == 0){
		DEBUG_PRINT("photoshop detected only instrumenting Photoshop.exe\n");
		if(strcmp(dr_get_application_name(), "Photoshop.exe") != 0){
			return;
		}
		DEBUG_PRINT("starting to instrument Photoshop.exe\n");
	}

	

	for( i=0 ; i<argument_length; i++){
		for( j=0; j<pass_length; j++){
			if(strcmp(arguments[i].name,ins_pass[j].name) == 0){
				//we can now register the call backs
				DEBUG_PRINT("%s - registered\n", arguments[i].name);

				DEBUG_PRINT("%s - initializing...\n", arguments[i].name);
				ins_pass[j].init_func(id,arguments[i].name, arguments[i].arguments);
				DEBUG_PRINT("%s - initialized\n", arguments[i].name);

				//register thread events
				if(ins_pass[j].thread_init != NULL)
					drmgr_register_thread_init_event(ins_pass[j].thread_init);
				if(ins_pass[j].thread_exit != NULL)
					drmgr_register_thread_exit_event(ins_pass[j].thread_exit);
				
				//register bb events
				if(ins_pass[j].app2app_bb != NULL)
					drmgr_register_bb_app2app_event(ins_pass[j].app2app_bb,
									&ins_pass[j].priority);
				if(ins_pass[j].instrumentation_bb != NULL || ins_pass[j].analysis_bb != NULL)
					drmgr_register_bb_instrumentation_event(ins_pass[j].analysis_bb,
										ins_pass[j].instrumentation_bb,
										&ins_pass[j].priority);
				if (ins_pass[j].module_load != NULL)
					drmgr_register_module_load_event_ex(ins_pass[j].module_load, &ins_pass[j].priority);
				if (ins_pass[j].module_unload != NULL)
					drmgr_register_module_unload_event_ex(ins_pass[j].module_unload, &ins_pass[j].priority);
			}
		}
	}

	

	dr_register_exit_event(process_exit_routine_call);
		
}


static void process_exit_routine_call(void){

	int i = 0;
	int j = 0;


	for( i=0 ; i<argument_length; i++){
		for( j=0; j<pass_length; j++){
			if(strcmp(arguments[i].name,ins_pass[j].name) == 0){
				DEBUG_PRINT("%s - exiting....\n", ins_pass[j].name);
				ins_pass[j].process_exit();
				DEBUG_PRINT("%s - exited\n", ins_pass[j].name);
			}
		}
	}
	/*if (log_mode){
		dr_close_file(global_logfile);
	}*/
	drmgr_exit();

	
}




void process_global_arguments(){

	int i = 0;

	for (i = 0; i < argument_length; i++){
		if (strcmp(arguments[i].name, "logdir") == 0){
			dr_printf("global logdir - %s\n", arguments[i].arguments);
			strncpy(logdir, arguments[i].arguments, MAX_STRING_LENGTH);
		}
		else if (strcmp(arguments[i].name, "debug") == 0){
			dr_printf("global debug - %s\n", arguments[i].arguments);
			debug_mode = arguments[i].arguments[0] - '0';
		}
		else if (strcmp(arguments[i].name, "log") == 0){
			dr_printf("global log - %s\n", arguments[i].arguments);
			log_mode = arguments[i].arguments[0] - '0';
		}
		else if (strcmp(arguments[i].name, "exec") == 0){
			dr_printf("exec - %s\n", arguments[i].arguments);
			strncpy(exec, arguments[i].arguments, MAX_STRING_LENGTH);
		}
	}
}

static void doCommandLineArgProcessing(client_id_t id){
	
	const char * args = dr_get_options(id);

	int i = 0;

	int string_index = 0;
	int index = -1;
	int name_collect_state = 0;
	int arguments_collect_state = 0;

	while(args[i] != '\0'){
		if(args[i] == '-'){
			name_collect_state = 1;
			arguments_collect_state = 0;
			if(index>=0){
				arguments[index].arguments[string_index - 1] = '\0';
			}
			index++;
			string_index = 0;
			i++;
			continue;
		}

		if(args[i] == ' ' && name_collect_state){
			arguments[index].name[string_index++] = '\0';
			name_collect_state = 0;
			arguments_collect_state = 1;
			string_index = 0;
			i++;
			continue;
		}


		if(name_collect_state){
			arguments[index].name[string_index++] = args[i];
		}
		if(arguments_collect_state){
			arguments[index].arguments[string_index++] = args[i];
		}
		
		i++;
	}

	//epilog
	arguments[index].arguments[string_index++] = '\0';
	argument_length = index + 1;
	
	
	process_global_arguments();
	
}



//this function is responsible for setting up priorities and the order of the instrumentation passes
static void setupInsPasses(){

	//priority structure template
	drmgr_priority_t priority = {
        sizeof(priority), /* size of struct */
        "default",       /* name of our operation */
		DRMGR_PRIORITY_NAME_DRWRAP,             /* optional name of operation we should precede */
        NULL,             /* optional name of operation we should follow */
        0};


	//ins pass 1 - bbinfo - most of the time should be executed first 
	ins_pass[0].name = "profile";	
	ins_pass[0].priority = priority;
	ins_pass[0].priority.name = ins_pass[0].name;
	ins_pass[0].priority.priority = 1;
	ins_pass[0].init_func = bbinfo_init;
	ins_pass[0].app2app_bb = bbinfo_bb_app2app;
	ins_pass[0].analysis_bb = bbinfo_bb_analysis;
	ins_pass[0].instrumentation_bb = bbinfo_bb_instrumentation;
	ins_pass[0].thread_init = bbinfo_thread_init;
	ins_pass[0].thread_exit = bbinfo_thread_exit;
	ins_pass[0].process_exit = bbinfo_exit_event;
	ins_pass[0].module_load = NULL;
	ins_pass[0].module_unload = NULL;


	//ins pass 2 - cpuid
	ins_pass[1].name = "cpuid";	
	ins_pass[1].priority = priority;
	ins_pass[1].priority.name = ins_pass[1].name;
	ins_pass[1].priority.priority = 3;
	ins_pass[1].init_func = cpuid_init;
	ins_pass[1].app2app_bb = cpuid_bb_app2app;
	ins_pass[1].analysis_bb = cpuid_bb_analysis;
	ins_pass[1].instrumentation_bb = cpuid_bb_instrumentation;
	ins_pass[1].thread_init = cpuid_thread_init;
	ins_pass[1].thread_exit = cpuid_thread_exit;
	ins_pass[1].process_exit = cpuid_exit_event;
	ins_pass[1].module_load = NULL;
	ins_pass[1].module_unload = NULL;

	//ins pass 3 - memtrace
	ins_pass[2].name = "memtrace";	
	ins_pass[2].priority = priority;
	ins_pass[2].priority.name = ins_pass[2].name;
	ins_pass[2].priority.priority = 3;
	ins_pass[2].init_func = memtrace_init;
	ins_pass[2].app2app_bb = memtrace_bb_app2app;
	ins_pass[2].analysis_bb = memtrace_bb_analysis;
	ins_pass[2].instrumentation_bb = memtrace_bb_instrumentation;
	ins_pass[2].thread_init = memtrace_thread_init;
	ins_pass[2].thread_exit = memtrace_thread_exit;
	ins_pass[2].process_exit = memtrace_exit_event;
	ins_pass[2].module_load = NULL;
	ins_pass[2].module_unload = NULL;


	//ins pass 4 - inscount
	ins_pass[3].name = "inscount";	
	ins_pass[3].priority = priority;
	ins_pass[3].priority.name = ins_pass[3].name;
	ins_pass[3].priority.priority = 3;
	ins_pass[3].init_func = inscount_init;
	ins_pass[3].app2app_bb = NULL;
	ins_pass[3].analysis_bb = inscount_bb_analysis;
	ins_pass[3].instrumentation_bb = inscount_bb_instrumentation;
	ins_pass[3].thread_init = NULL;
	ins_pass[3].thread_exit = NULL;
	ins_pass[3].process_exit = inscount_exit_event;
	ins_pass[3].module_load = NULL;
	ins_pass[3].module_unload = NULL;

	//ins pass 5 - instrace
	ins_pass[4].name = "instrace";	
	ins_pass[4].priority = priority;
	ins_pass[4].priority.name = ins_pass[4].name;
	ins_pass[4].priority.priority = 3;
	ins_pass[4].init_func = instrace_init;
	ins_pass[4].app2app_bb = instrace_bb_app2app;
	ins_pass[4].analysis_bb = instrace_bb_analysis;
	ins_pass[4].instrumentation_bb = instrace_bb_instrumentation;
	ins_pass[4].thread_init = instrace_thread_init;
	ins_pass[4].thread_exit = instrace_thread_exit;
	ins_pass[4].process_exit = instrace_exit_event;
	ins_pass[4].module_load = NULL;
	ins_pass[4].module_unload = NULL;


	//ins pass 6 - functrace - this is a low priority update (should be the last)
	ins_pass[5].name = "functrace";
	ins_pass[5].priority = priority;
	ins_pass[5].priority.name = ins_pass[5].name;
	ins_pass[5].priority.priority = 4;
	ins_pass[5].init_func = functrace_init;
	ins_pass[5].app2app_bb = functrace_bb_app2app;
	ins_pass[5].analysis_bb = functrace_bb_analysis;
	ins_pass[5].instrumentation_bb = functrace_bb_instrumentation;
	ins_pass[5].thread_init = functrace_thread_init;
	ins_pass[5].thread_exit = functrace_thread_exit;
	ins_pass[5].process_exit = functrace_exit_event;
	ins_pass[5].module_load = NULL;
	ins_pass[5].module_unload = NULL;

	//ins pass 7 - funcwrapping - given a high priority  
	ins_pass[6].name = "funcwrap";
	ins_pass[6].priority = priority;
	ins_pass[6].priority.name = ins_pass[6].name;
	ins_pass[6].priority.priority = 0;
	ins_pass[6].init_func = funcwrap_init;
	ins_pass[6].app2app_bb = NULL;
	ins_pass[6].analysis_bb = NULL;
	ins_pass[6].instrumentation_bb = funcwrap_bb_instrumentation;
	ins_pass[6].thread_init = funcwrap_thread_init;
	ins_pass[6].thread_exit = funcwrap_thread_exit;
	ins_pass[6].process_exit = funcwrap_exit_event;
	ins_pass[6].module_load = funcwrap_module_load;
	ins_pass[6].module_unload = NULL;


	//ins pass 8 - memdump
	ins_pass[7].name = "memdump";
	ins_pass[7].priority = priority;
	ins_pass[7].priority.name = ins_pass[7].name;
	ins_pass[7].priority.priority = 0;
	ins_pass[7].init_func = memdump_init;
	ins_pass[7].app2app_bb = NULL;
	ins_pass[7].analysis_bb = NULL;
	ins_pass[7].instrumentation_bb = memdump_bb_instrumentation;
	ins_pass[7].thread_init = memdump_thread_init;
	ins_pass[7].thread_exit = memdump_thread_exit;
	ins_pass[7].process_exit = memdump_exit_event;
	ins_pass[7].module_load = memdump_module_load;
	ins_pass[7].module_unload = NULL;

	//ins pass 9 - funcreplace
	ins_pass[8].name = "funcreplace";
	ins_pass[8].priority = priority;
	ins_pass[8].priority.name = ins_pass[8].name;
	ins_pass[8].priority.priority = 0;
	ins_pass[8].init_func = funcreplace_init;
	ins_pass[8].app2app_bb = NULL;
	ins_pass[8].analysis_bb = NULL;
	ins_pass[8].instrumentation_bb = funcreplace_bb_instrumentation;
	ins_pass[8].thread_init = funcreplace_thread_init;
	ins_pass[8].thread_exit = funcreplace_thread_exit;
	ins_pass[8].process_exit = funcreplace_exit_event;
	ins_pass[8].module_load = funcreplace_module_load;
	ins_pass[8].module_unload = NULL;

	//ins pass 10 - misc
	ins_pass[9].name = "misc";
	ins_pass[9].priority = priority;
	ins_pass[9].priority.name = ins_pass[9].name;
	ins_pass[9].priority.priority = 0;
	ins_pass[9].init_func = misc_init;
	ins_pass[9].app2app_bb = NULL;
	ins_pass[9].analysis_bb = NULL;
	ins_pass[9].instrumentation_bb = misc_bb_instrumentation;
	ins_pass[9].thread_init = misc_thread_init;
	ins_pass[9].thread_exit = misc_thread_exit;
	ins_pass[9].process_exit = misc_exit_event;
	ins_pass[9].module_load = NULL;
	ins_pass[9].module_unload = NULL;


	pass_length = 10;

}

