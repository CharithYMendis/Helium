#ifndef _FUNCREPLACE_EXALGO_H
#define _FUNCREPLACE_EXALGO_H

#include "defines.h"
#include "dr_api.h"

/* typdefs */

/*instrumentation routines*/

/* for the entire process */
void funcreplace_init(client_id_t id, const char * name,
	const char * arguments);
void funcreplace_exit_event(void);

/* for basic blocks */
dr_emit_flags_t funcreplace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace, bool translating,
	void *user_data);
dr_emit_flags_t
funcreplace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data);
dr_emit_flags_t
funcreplace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating);


/* for threads */
void funcreplace_thread_init(void *drcontext);
void funcreplace_thread_exit(void *drcontext);

/* module load */
void funcreplace_module_load(void * drcontext, module_data_t * module, bool loaded);


#endif