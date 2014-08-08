#ifndef _BBINFO_EXALGO_H
#define _BBINFO_EXALGO_H

#include "dr_api.h"


/*instrumentation routines*/
void bbinfo_init(client_id_t id, const char * name,
				const char * arguments);
void bbinfo_exit_event(void);
dr_emit_flags_t bbinfo_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr_current, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t
bbinfo_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
void bbinfo_thread_init(void *drcontext);
void bbinfo_thread_exit(void *drcontext);
dr_emit_flags_t
bbinfo_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating);


#endif