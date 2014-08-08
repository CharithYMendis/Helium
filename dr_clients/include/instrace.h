 #ifndef _INSTRACE_EXALGO_H
 #define _INSTRACE_EXALGO_H

#include "dr_api.h"
#include "defines.h"

 /*instrumentation routines*/
void instrace_init(client_id_t id, const char * name,
				const char * arguments);
void instrace_exit_event(void);
dr_emit_flags_t instrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t
instrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
dr_emit_flags_t
instrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating);
void instrace_thread_init(void *drcontext);
void instrace_thread_exit(void *drcontext);


 
 /* typdefs and defines */

#define OPCODE_COUNT	1098




 
 #endif

