#ifndef _MEMDUMP_EXALGO_H
#define _MEMDUMP_EXALGO_H

#include "defines.h"
#include "dr_api.h"

/* typdefs */

/*instrumentation routines*/

/* for the entire process */
void memdump_init(client_id_t id, const char * name,
	const char * arguments);
void memdump_exit_event(void);

/* for basic blocks */
dr_emit_flags_t memdump_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace, bool translating,
	void *user_data);
dr_emit_flags_t
memdump_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data);
dr_emit_flags_t
memdump_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating);


/* for threads */
void memdump_thread_init(void *drcontext);
void memdump_thread_exit(void *drcontext);

void memdump_module_load(void * drcontext, module_data_t * module, bool loaded);


#endif