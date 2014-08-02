#include "cpuid.h"
#include <stdio.h>
#include "dr_api.h"
#include "drmgr.h"
#include "defines.h"


//ECX
#define SSE_42 1<<(20)
#define SSE_41 1<<(19)
#define SSSE_3 1<<(9)
#define SSE_3  1<<(0)

//EDX
#define SSE    1<<(26)
#define SSE_2  1<<(25)
#define MMX	   1<<(23)

#define ECX_MASK ~(SSE_42 | SSE_41 | SSSE_3 | SSE_3)
#define EDX_MASK ~(SSE | SSE_2 | MMX)

void cpuid_init(client_id_t id, 
				const char * arguments){

	drmgr_init();	


}
void cpuid_exit_event(void){

	drmgr_exit();
	
}

void cpuid_thread_init(void *drcontext){}
void cpuid_thread_exit(void *drcontext){}


dr_emit_flags_t
cpuid_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating){
	return DR_EMIT_DEFAULT;
}


dr_emit_flags_t cpuid_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data){
	
	return DR_EMIT_DEFAULT;

}

dr_emit_flags_t cpuid_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data){
		
		instr_t * and_ecx;
		instr_t * and_edx;
		instr_t * label1;
		instr_t * jne;
		instr_t * cmp;
		instr_t * next;

		/*
		<store eax in spill 1>
		cpuid
		<store eax in spill 2>
		<save arithmetic>
		cmp spill1,1
		jne label1
		and ecx,0
		and edx,0
		label1 : <restore eax spill2 and arithmetic flags>
		<original>
		*/

		//cpuid
		if(instr_get_opcode(instr) == OP_cpuid){  
			//we will change what is returned by the instruction

			//instrlist_disassemble(drcontext,instr_get_app_pc(instrlist_first(bb)),bb,summary_file);

			next = instr_get_next(instr);
			label1 = INSTR_CREATE_label(drcontext);
			cmp = INSTR_CREATE_cmp(drcontext,dr_reg_spill_slot_opnd(drcontext,SPILL_SLOT_1),opnd_create_immed_int(1,OPSZ_1));
			jne = INSTR_CREATE_jcc_short(drcontext,OP_jne,opnd_create_instr(label1));
			and_edx = INSTR_CREATE_and(drcontext,opnd_create_reg(DR_REG_EDX),
											opnd_create_immed_int(EDX_MASK,OPSZ_4));
			and_ecx = INSTR_CREATE_and(drcontext,opnd_create_reg(DR_REG_ECX),
											opnd_create_immed_int(ECX_MASK,OPSZ_4));


			//sequence of insertions
			dr_save_reg(drcontext,bb,instr,DR_REG_XAX,SPILL_SLOT_1);

			instrlist_meta_postinsert(bb,instr,cmp);

			dr_save_reg(drcontext, bb, cmp, DR_REG_XAX, SPILL_SLOT_2);
			dr_save_arith_flags_to_xax(drcontext, bb, cmp);

			instrlist_meta_postinsert(bb,cmp,jne);
			instrlist_meta_postinsert(bb,jne,and_ecx);
			instrlist_meta_postinsert(bb,and_ecx,and_edx);
			instrlist_meta_postinsert(bb,and_edx,label1);

			dr_restore_arith_flags_from_xax(drcontext, bb,next);
			dr_restore_reg(drcontext, bb,next, DR_REG_XAX, SPILL_SLOT_2);

			//instrlist_disassemble(drcontext,instr_get_app_pc(instrlist_first(bb)),bb,summary_file);
			
		}

		return DR_EMIT_DEFAULT;


}





