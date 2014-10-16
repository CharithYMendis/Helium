#ifndef _FUNCWRAP_EXALGO_H
#define _FUNCWRAP_EXALGO_H

#include "defines.h"
#include "dr_api.h"

extern uint is_within_func;
extern uint thread_id_func;

/* typdefs */

bool should_filter_func();
bool should_filter_thread(uint thread_id);

/*instrumentation routines*/

/* for the entire process */
void funcwrap_init(client_id_t id, const char * name,
	const char * arguments);
void funcwrap_exit_event(void);

/* for threads */
void funcwrap_thread_init(void *drcontext);
void funcwrap_thread_exit(void *drcontext);

/* bb analysis */
dr_emit_flags_t
funcwrap_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data);

/* for module */
void funcwrap_module_load(void * drcontext, module_data_t * module, bool loaded);


#endif