#ifndef _FUNCTRACE_EXALGO_H
#define _FUNCTRACE_EXALGO_H

#include "dr_api.h"

/* typdefs */
typedef struct _function_t {

	uint start_addr;
	uint end_addr;
	bool is_recursive;

} function_t;

/* other public functions */
function_t * get_current_function(void * drcontext);
uint get_current_function_all(void * drcontext);

/*instrumentation routines*/

/* for the entire process */
void functrace_init(client_id_t id, const char * name,
	const char * arguments);
void functrace_exit_event(void);

/* for basic blocks */
dr_emit_flags_t functrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace, bool translating,
	void *user_data);
dr_emit_flags_t
functrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data);
dr_emit_flags_t
functrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating);


/* for threads */
void functrace_thread_init(void *drcontext);
void functrace_thread_exit(void *drcontext);




#endif