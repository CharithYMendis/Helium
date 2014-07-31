#ifndef _TEMPLATE_EXALGO_H
#define _TEMPLATE_EXALGO_H

#include "defines.h"
#include "dr_api.h"

/* typdefs */

/*instrumentation routines*/

/* for the entire process */
void <client_name>_init(client_id_t id,
	const char * arguments);
void <client_name>_exit_event(void);

/* for basic blocks */
dr_emit_flags_t <client_name>_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace, bool translating,
	void *user_data);
dr_emit_flags_t
<client_name>_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data);
dr_emit_flags_t
<client_name>_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating);


/* for threads */
void <client_name>_thread_init(void *drcontext);
void <client_name>_thread_exit(void *drcontext);


#endif