#ifndef _MEMTRACE_EXALGO_H
#define _MEMTRACE_EXALGO_H

#include "dr_api.h"
 
 /*instrumentation routines*/
void memtrace_init(client_id_t id, const char * name,
				const char * arguments);
void memtrace_exit_event(void);
dr_emit_flags_t memtrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t
memtrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
dr_emit_flags_t
memtrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating);
void memtrace_thread_init(void *drcontext);
void memtrace_thread_exit(void *drcontext);

 
 
 
 #endif