#include <stdio.h>
#include "dr_api.h"
#include "drmgr.h"

/* includes from by instrumentation passes */
#include "bbinfo.h"
#include "cpuid.h"
#include "memtrace.h"
#include "inscount.h"
#include "instrace.h"
#include "defines.h"

//#define DEBUG_MAIN
#define NO_OF_INS_PASSES 10

typedef void (*thread_func_t) (void * drcontext);
typedef void (*init_func_t) (client_id_t id, const char * arguments);
typedef void (*exit_func_t) (void);

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

} instrumentation_pass_t;

typedef struct _cmdarguments_t {
	
	char name[MAX_STRING_LENGTH];
	char arguments[MAX_STRING_LENGTH];

} cmdarguments_t;

//prototypes
static void setupInsPasses();
static void doCommandLineArgProcessing(client_id_t id);
static void process_exit_routine_call(void);

//allocate statically enough space
static instrumentation_pass_t ins_pass[NO_OF_INS_PASSES];
static cmdarguments_t arguments[NO_OF_INS_PASSES];
static int pass_length = 0;
static int argument_length = 0;


DR_EXPORT void 
dr_init(client_id_t id)
{
	
	int i = 0;
	int j = 0;

	drmgr_init();
	dr_enable_console_printing();

	setupInsPasses();
	doCommandLineArgProcessing(id);

#ifdef DEBUG_MAIN
	dr_printf("argument length - %d\n",argument_length);
	for( i=0; i<argument_length; i++ ){
		dr_printf("%s - %s\n",arguments[i].name,arguments[i].arguments);
	}
#endif


	for( i=0 ; i<argument_length; i++){
		for( j=0; j<pass_length; j++){
			if(strcmp(arguments[i].name,ins_pass[j].name) == 0){
				//we can now register the call backs
#ifdef DEBUG_MAIN
	dr_printf("%s - registered\n",arguments[i].name);
#endif
				ins_pass[j].init_func(id,arguments[i].arguments);
				
				//register thread events
				if(ins_pass[j].thread_init != NULL)
					drmgr_register_thread_init_event(ins_pass[j].thread_init);
				if(ins_pass[j].thread_exit != NULL)
					drmgr_register_thread_exit_event(ins_pass[j].thread_exit);
				
				//register bb events
				if(ins_pass[j].app2app_bb != NULL)
					drmgr_register_bb_app2app_event(ins_pass[j].app2app_bb,
									&ins_pass[j].priority);
				if(ins_pass[j].instrumentation_bb != NULL && ins_pass[j].analysis_bb != NULL)
					drmgr_register_bb_instrumentation_event(ins_pass[j].analysis_bb,
										ins_pass[j].instrumentation_bb,
										&ins_pass[j].priority);
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
				ins_pass[j].process_exit();
			}
		}
	}

	drmgr_exit();

	
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
				arguments[index].arguments[string_index] = '\0';
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
	
	
	
}

//this function is responsible for setting up priorities and the order of the instrumentation passes
static void setupInsPasses(){

	//priority structure template
	drmgr_priority_t priority = {
        sizeof(priority), /* size of struct */
        "default",       /* name of our operation */
        NULL,             /* optional name of operation we should precede */
        NULL,             /* optional name of operation we should follow */
        0};

	//ins pass 1 - bbinfo
	ins_pass[0].name = "bbinfo";	
	ins_pass[0].priority = priority;
	ins_pass[0].priority.name = ins_pass[0].name;
	ins_pass[0].priority.priority = 0;
	ins_pass[0].init_func = bbinfo_init;
	ins_pass[0].app2app_bb = bbinfo_bb_app2app;
	ins_pass[0].analysis_bb = bbinfo_bb_analysis;
	ins_pass[0].instrumentation_bb = bbinfo_bb_instrumentation;
	ins_pass[0].thread_init = bbinfo_thread_init;
	ins_pass[0].thread_exit = bbinfo_thread_exit;
	ins_pass[0].process_exit = bbinfo_exit_event;


	//ins pass 2 - cpuid
	ins_pass[1].name = "cpuid";	
	ins_pass[1].priority = priority;
	ins_pass[1].priority.name = ins_pass[1].name;
	ins_pass[1].priority.priority = 0;
	ins_pass[1].init_func = cpuid_init;
	ins_pass[1].app2app_bb = cpuid_bb_app2app;
	ins_pass[1].analysis_bb = cpuid_bb_analysis;
	ins_pass[1].instrumentation_bb = cpuid_bb_instrumentation;
	ins_pass[1].thread_init = cpuid_thread_init;
	ins_pass[1].thread_exit = cpuid_thread_exit;
	ins_pass[1].process_exit = cpuid_exit_event;

	//ins pass 3 - memtrace
	ins_pass[2].name = "memtrace";	
	ins_pass[2].priority = priority;
	ins_pass[2].priority.name = ins_pass[2].name;
	ins_pass[2].priority.priority = 0;
	ins_pass[2].init_func = memtrace_init;
	ins_pass[2].app2app_bb = memtrace_bb_app2app;
	ins_pass[2].analysis_bb = memtrace_bb_analysis;
	ins_pass[2].instrumentation_bb = memtrace_bb_instrumentation;
	ins_pass[2].thread_init = memtrace_thread_init;
	ins_pass[2].thread_exit = memtrace_thread_exit;
	ins_pass[2].process_exit = memtrace_exit_event;


	//ins pass 4 - inscount
	ins_pass[3].name = "inscount";	
	ins_pass[3].priority = priority;
	ins_pass[3].priority.name = ins_pass[3].name;
	ins_pass[3].priority.priority = 0;
	ins_pass[3].init_func = inscount_init;
	ins_pass[3].app2app_bb = NULL;
	ins_pass[3].analysis_bb = inscount_bb_analysis;
	ins_pass[3].instrumentation_bb = inscount_bb_instrumentation;
	ins_pass[3].thread_init = NULL;
	ins_pass[3].thread_exit = NULL;
	ins_pass[3].process_exit = inscount_exit_event;

	//ins pass 5 - instrace
	ins_pass[4].name = "instrace";	
	ins_pass[4].priority = priority;
	ins_pass[4].priority.name = ins_pass[4].name;
	ins_pass[4].priority.priority = 0;
	ins_pass[4].init_func = instrace_init;
	ins_pass[4].app2app_bb = instrace_bb_app2app;
	ins_pass[4].analysis_bb = instrace_bb_analysis;
	ins_pass[4].instrumentation_bb = instrace_bb_instrumentation;
	ins_pass[4].thread_init = instrace_thread_init;
	ins_pass[4].thread_exit = instrace_thread_exit;
	ins_pass[4].process_exit = instrace_exit_event;

	pass_length = 5;

	 
	

}

