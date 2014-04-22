#include "dr_api.h"
#include "drmgr.h"
#include "inscount.h"

#define SHOW_RESULTS

#ifdef WINDOWS
# define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
# define DISPLAY_STRING(msg) dr_printf("%s\n", msg);
#endif

#define NULL_TERMINATE(buf) buf[(sizeof(buf)/sizeof(buf[0])) - 1] = '\0'

/* constant values for module names */
static const char * main_modules[] = {"C:\\Program Files (x86)\\Adobe\\Adobe Photoshop CS6\\Required\\Plug-Ins\\Extensions\\MMXCore.8BX",
							"C:\\Program Files (x86)\\Adobe\\Adobe Photoshop CS6\\Required\\Plug-Ins\\Filters\\Standard MultiPlugin.8BF"};
static const int main_modules_length = 2;


/* we only have a global count */
static uint64 global_count;
/* A simple clean call that will be automatically inlined because it has only
 * one argument and contains no calls to other functions.
 */
static void inscount(uint num_instrs) { global_count += num_instrs; }

void inscount_init(client_id_t id, const char * arguments)
{
    drmgr_init();
	
	global_count = 0;

    /* make it easy to tell, by looking at log file, which client executed */
    dr_log(NULL, LOG_ALL, 1, "Client 'inscount' initializing\n");
#ifdef SHOW_RESULTS
    /* also give notification to stderr */
    if (dr_is_notify_on()) {
# ifdef WINDOWS
        /* ask for best-effort printing to cmd window.  must be called in dr_init(). */
        dr_enable_console_printing();
# endif
        dr_fprintf(STDERR, "Client inscount is running\n");
    }
#endif
}

void inscount_exit_event(void)
{
#ifdef SHOW_RESULTS
    char msg[512];
    int len;
    len = dr_snprintf(msg, sizeof(msg)/sizeof(msg[0]),
                      "process name - %s\nInstrumentation results: %llu instructions executed\n"
                      ,dr_get_application_name(),global_count);
    DR_ASSERT(len > 0);
    NULL_TERMINATE(msg);
    DISPLAY_STRING(msg);
#endif /* SHOW_RESULTS */
	
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
	
	module_data_t * module_data;
	instr_t *first = instrlist_first(bb);
	int i;
	uint num_instrs = 0;
	uint offset;
	
	if(instr == first){
		module_data = dr_lookup_module(instr_get_app_pc(first));
		
		//dynamically generated code - module information not available
		if(module_data == NULL){  
			return DR_EMIT_DEFAULT;
		}

		for(i=0;i<main_modules_length;i++){
			if(strcmp(module_data->full_path,main_modules[i]) == 0){
				offset = instr_get_app_pc(first) - module_data->start;
				if(offset >= 0xe85f && offset <= 0xeb61){
					for(instr = first ; instr!=NULL ; instr = instr_get_next(instr)){
						num_instrs++;
					}
				}
				break;
			}
		}

		if(num_instrs > 0){
			dr_insert_clean_call(drcontext, bb, instrlist_first(bb),
							 (void *)inscount, false /* save fpstate */, 1,
							 OPND_CREATE_INT32(num_instrs));
		}
		
		dr_free_module_data(module_data);
	}

	return DR_EMIT_DEFAULT;
}
