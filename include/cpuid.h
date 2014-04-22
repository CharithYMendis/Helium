#ifndef CPUID_MASK
#define CPUID_MASK


void cpuid_init(client_id_t id, 
				const char * arguments);
void cpuid_exit_event(void);
dr_emit_flags_t cpuid_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t cpuid_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
void cpuid_thread_init(void *drcontext);
void cpuid_thread_exit(void *drcontext);
dr_emit_flags_t
cpuid_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating);







#endif