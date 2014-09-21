#ifndef _TEMPLATE_EXALGO_H
#define _TEMPLATE_EXALGO_H

#include "defines.h"
#include "dr_api.h"

/* typdefs */

/*instrumentation routines*/

/* for the entire process */
void misc_init(client_id_t id, const char * name,
	const char * arguments);
void misc_exit_event(void);

/* for basic blocks */
dr_emit_flags_t misc_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace, bool translating,
	void *user_data);
dr_emit_flags_t
misc_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data);
dr_emit_flags_t
misc_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating);


/* for threads */
void misc_thread_init(void *drcontext);
void misc_thread_exit(void *drcontext);


#endif