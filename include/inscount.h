 #ifndef INSCOUNT
 #define INSCOUNT

 /*instrumentation routines*/
void inscount_init(client_id_t id, const char * arguments);
void inscount_exit_event(void);
dr_emit_flags_t inscount_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t inscount_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
 

 
 #endif