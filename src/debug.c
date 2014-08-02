#include "debug.h"
#include "dr_api.h"
#include "instrace.h"
#include "defines.h"

/*
this file contains functions for canonicalizing x86 instructions and producing only the needed static 
information for a given application.
*/


void debug_static_info(void * drcontext,instr_t * instr,file_t outfile){

	char string_storage[MAX_STRING_LENGTH];
	opnd_t operand;
	int i;
	
	
	instr_disassemble_to_buffer(drcontext,instr,string_storage,MAX_STRING_LENGTH);
	dr_fprintf(outfile,"%s\n",string_storage);
	dr_fprintf(outfile,"srcs - %d\n",instr_num_srcs(instr));
	dr_fprintf(outfile,"dests - %d\n",instr_num_dsts(instr));


	for(i=0; i<instr_num_srcs(instr); i++){

		dr_fprintf(outfile,"srcs(%d)-",i);
		operand = instr_get_src(instr,i);
		if(opnd_is_immed_int(operand)){
			dr_fprintf(outfile,"imm-%d\n",opnd_get_immed_int(operand));
		}
		else if(opnd_is_immed_float(operand)){
			dr_fprintf(outfile,"imm-%f\n",opnd_get_immed_float(operand));
		}
		else if(opnd_is_memory_reference(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(outfile,"mem-%s\n",string_storage);
		}
		else if(opnd_is_reg(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(outfile,"reg-%s-width-%d\n",string_storage,opnd_size_in_bytes(reg_get_size(opnd_get_reg(operand))));
		}

	}

	for(i=0; i<instr_num_dsts(instr); i++){

		dr_fprintf(outfile,"dest(%d)-",i);
		operand = instr_get_dst(instr,i);
		if(opnd_is_immed_int(operand)){
			dr_fprintf(outfile,"imm-%d\n",opnd_get_immed_int(operand));
		}
		else if(opnd_is_immed_float(operand)){
			dr_fprintf(outfile,"imm-%f\n",opnd_get_immed_float(operand));
		}
		else if(opnd_is_memory_reference(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(outfile,"mem-%s\n",string_storage);
		}
		else if(opnd_is_reg(operand)){
			opnd_disassemble_to_buffer(drcontext,operand,string_storage,MAX_STRING_LENGTH);
			dr_fprintf(outfile,"reg-%s-width-%d\n",string_storage,opnd_size_in_bytes(reg_get_size(opnd_get_reg(operand))));
		}


	}

}

//static_info->src_num = instr_num_srcs(instr);

	//DR_ASSERT(static_info->src_num <= 2);

	//dr_log(drcontext,LOG_ALL,1,"");

	/*for(i=0; i<instr_num_srcs(instr);i++){
		operand = instr_get_src(instr,i);
		if(opnd_is_reg(operand)){

	
			reg = opnd_get_reg(operand);
			reg_size = opnd_size_in_bytes(reg_get_size(reg));

			SET_TYPE(static_info->src[i].type,REG_TYPE);
			static_info->src[i].location = reg;
			switch(reg_size){
				case 1: 
					{
						if(reg <= DR_REG_BL){
							static_info->src[i].type |= WIDTH_8L;
						}
						else{
							static_info->src[i].type |= WIDTH_8H;
						}
					}
				case 2:
					static_info->src[i].type |= WIDTH_16;
				case 4:
					static_info->src[i].type |= WIDTH_32;
				case 8:
					static_info->src[i].type |= WIDTH_64;
				default:
			}

		}
		else if(opnd_is_immed(operand)){  //can be floating point operand or int operand
			SET_TYPE(static_info->src[i].type,IMM_TYPE);
			if(opnd_is_immed_int(operand)){
				static_info->src[i].location = opnd_get_immed_int(operand);
			}
			else if(opnd_is_immed_float(operand)){
				static_info->src[i].location = 0;
				//static_info->src[i].location = opnd_get_immed_float(operand);
			}
		}
		else if(opnd_is_memory_reference(operand)){
			SET_TYPE(static_info->src[i].type,MEM_TYPE);
			static_info->src[i].type |= (drutil_opnd_mem_size_in_bytes(operand,instr)) << 5;
		}
		else{
		}
	}*/

	/* setup the destinations - only a single destination should be processed 
	   we need to specialize based on the instructions for multi-destination instructions.
	   for single destination instruction we need not specialize */

	/* fill up assuming single destination or less */
	/*static_info->dest_num = 0;
	if(instr_num_dsts(instr) > 0){
		static_info->dest_num = 1;
		operand = instr_get_dst(instr,0);
	}
	static_info->dst.operation = instr_get_opcode(instr);*/ /* just use dr's enum encoding; we will keep using until it is ready to pretty print */