#include <stdio.h>
#include "canonicalize.h"
#include "defines.h"
#include "print_common.h"
#include <stdlib.h>


#define assign_value(start,opnd)  \
	opnd->value = (start) * MAX_SIZE_OF_REG - opnd->width; \
	 break

#define abcd_reg(v,start,opnd) \
	case DR_REG_R##v##X:  \
	case DR_REG_E##v##X:  \
	case DR_REG_##v##X:   \
	case DR_REG_##v##H:   \
	case DR_REG_##v##L:   \
						  \
	 opnd->value = (start) * MAX_SIZE_OF_REG - opnd->width; \
	 if( opnd->value == DR_REG_##v##H ){				  \
		opnd->value = (start) * MAX_SIZE_OF_REG - 2;      \
	 }													  \
	 break

#define sbsd_reg(v,start,opnd) \
	case DR_REG_R##v##:		   \
	case DR_REG_E##v##:		   \
	case DR_REG_##v##:         \
	case DR_REG_##v##L:        \
	assign_value(start,opnd)   

#define x64_reg(v,start,opnd) \
	case DR_REG_R##v##:       \
	case DR_REG_R##v##D:      \
	case DR_REG_R##v##W:      \
	case DR_REG_R##v##L:      \
	assign_value(start,opnd)  

#define mmx_reg(v,start,end)  \
	case DR_REG_MM##v##:      \
	case DR_REG_XMM##v##:	  \
	case DR_REG_YMM##v##:     \
	assign_value(start,opnd)

#define new_mmx_reg(v, start, end) \
	case DR_REG_XMM##v##:	  \
	case DR_REG_YMM##v##:     \
	assign_value(start,opnd)

#define fp_reg(v,start,end)  \
	case DR_REG_##v##:        \
	assign_value(start,opnd)

#define seg_reg(v,start,end)  \
	case DR_SEG_##v##:        \
	assign_value(start,opnd)

#define if_bounds(d,s)  if( (cinstr->num_dsts == d ) && (cinstr->num_srcs == s ) )
#define else_bounds else{ unhandled = true; } break


static void populate_rinstr(rinstr_t * rinstr,
					 operand_t dst,
					 int num_srcs,
					 operand_t src1,
				     operand_t src2,
					 int operation,
					 bool sign);


static void assert_opnds(int opcode,int needed_src, int needed_dst, int actual_src, int actual_dst){
	ASSERT_MSG((needed_src == actual_src) && (needed_dst == actual_src), ("ERROR: opcode %d - needed %d(src) %d(dst), actual %d(src) %d(dst)\n", opcode, needed_src, needed_dst, actual_src, actual_dst));
}


void reg_to_mem_range(operand_t * opnd){

	if (opnd->type == REG_TYPE){

		uint64 value = opnd->value;

#if DEBUG_LEVEL >= 3
		printf("%d\n", opnd->value);
		printf("before %s ", regname_to_string(opnd->value).c_str());
#endif


		switch (value){

			//abcd regs
			abcd_reg(A, 1, opnd);
			abcd_reg(B, 2, opnd);
			abcd_reg(C, 3, opnd);
			abcd_reg(D, 4, opnd);

			//sp,bp,si,di regs
			sbsd_reg(SP, 5, opnd);
			sbsd_reg(BP, 6, opnd);
			sbsd_reg(SI, 7, opnd);
			sbsd_reg(DI, 8, opnd);

			//x64 regs
			x64_reg(8, 9, opnd);
			x64_reg(9, 10, opnd);
			x64_reg(10, 11, opnd);
			x64_reg(11, 12, opnd);
			x64_reg(12, 13, opnd);
			x64_reg(13, 14, opnd);
			x64_reg(14, 15, opnd);
			x64_reg(15, 16, opnd);

			//mmx regs
			mmx_reg(0, 17, opnd);
			mmx_reg(1, 18, opnd);
			mmx_reg(2, 19, opnd);
			mmx_reg(3, 20, opnd);
			mmx_reg(4, 21, opnd);
			mmx_reg(5, 22, opnd);
			mmx_reg(6, 23, opnd);
			mmx_reg(7, 24, opnd);

			//new_mmx_regs
			new_mmx_reg(8, 25, opnd);
			new_mmx_reg(9, 26, opnd);
			new_mmx_reg(10, 27, opnd);
			new_mmx_reg(11, 28, opnd);
			new_mmx_reg(12, 29, opnd);
			new_mmx_reg(13, 30, opnd);
			new_mmx_reg(14, 31, opnd);
			new_mmx_reg(15, 32, opnd);

			//floating point regs
			fp_reg(ST0, 33, opnd);
			fp_reg(ST1, 34, opnd);
			fp_reg(ST2, 35, opnd);
			fp_reg(ST3, 36, opnd);
			fp_reg(ST4, 37, opnd);
			fp_reg(ST5, 38, opnd);
			fp_reg(ST6, 39, opnd);
			fp_reg(ST7, 40, opnd);

			//segments
			seg_reg(ES, 41, opnd);
			seg_reg(CS, 42, opnd);
			seg_reg(SS, 43, opnd);
			seg_reg(DS, 44, opnd);
			seg_reg(FS, 45, opnd);
			seg_reg(GS, 46, opnd);

			//virtual regs
			fp_reg(VIRTUAL_1, 47, opnd); 
			fp_reg(VIRTUAL_2, 48, opnd);

		default:
			ASSERT_MSG(false, ("ERROR: %d register not translated", value));

		}

#if DEBUG_LEVEL >= 3
		printf("after %s\n", opnd_to_string(opnd).c_str());
#endif

	}
	else if ((opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE)){
		ASSERT_MSG((opnd->value > MAX_SIZE_OF_REG * 48), ("ERROR: memory and register space overlap\n"));
	}



}

int mem_range_to_reg(operand_t * opnd){

	if (opnd->type == REG_TYPE){
		
		uint64 range = opnd->value / MAX_SIZE_OF_REG;
		range++;

		switch (range){

		case 1: return DR_REG_RAX;
		case 2: return DR_REG_RBX;
		case 3: return DR_REG_RCX;
		case 4: return DR_REG_RDX;
		case 5: return DR_REG_RSP;
		case 6: return DR_REG_RBP;
		case 7: return DR_REG_RSI;
		case 8: return DR_REG_RDI;
		
		case 9: return DR_REG_R8;
		case 10: return DR_REG_R9;
		case 11: return DR_REG_R10;
		case 12: return DR_REG_R11;
		case 13: return DR_REG_R12;
		case 14: return DR_REG_R13;
		case 15: return DR_REG_R14;
		case 16: return DR_REG_R15;

		case 17: return DR_REG_YMM0;
		case 18: return DR_REG_YMM1;
		case 19: return DR_REG_YMM2;
		case 20: return DR_REG_YMM3;
		case 21: return DR_REG_YMM4;
		case 22: return DR_REG_YMM5;
		case 23: return DR_REG_YMM6;
		case 24: return DR_REG_YMM7;
		case 25: return DR_REG_YMM8;
		case 26: return DR_REG_YMM9;
		case 27: return DR_REG_YMM10;
		case 28: return DR_REG_YMM11;
		case 29: return DR_REG_YMM12;
		case 30: return DR_REG_YMM13;
		case 31: return DR_REG_YMM14;
		case 32: return DR_REG_YMM15;

		case 33: return DR_REG_ST0;
		case 34: return DR_REG_ST1;
		case 35: return DR_REG_ST2;
		case 36: return DR_REG_ST3;
		case 37: return DR_REG_ST4;
		case 38: return DR_REG_ST5;
		case 39: return DR_REG_ST6;
		case 40: return DR_REG_ST7;

		case 41: return DR_SEG_ES;
		case 42: return DR_SEG_CS;
		case 43: return DR_SEG_SS;
		case 44: return DR_SEG_DS;
		case 45: return DR_SEG_FS;
		case 46: return DR_SEG_GS;

		case 47: return DR_REG_VIRTUAL_1;
		case 48: return DR_REG_VIRTUAL_2;

		default: return -1;


		}

	}
	else{
		return -1;
	}


}

rinstr_t * cinstr_to_rinstrs (cinstr_t * cinstr, int &amount){


	int operation;

	DEBUG_PRINT(("entering canonicalization\n"),2);

	rinstr_t * rinstr;
	rinstr = NULL;
	amount = 0;

	bool unhandled = false;

	switch (cinstr->opcode){

	case OP_push_imm:
	case OP_push:
		// [esp - 4] (dst[1]) <- src[0]
		if_bounds(2, 2){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_assign, cinstr->dsts[1], 1, { cinstr->srcs[0] }, false };
		}
		else_bounds;

	case OP_pop:
		// dst[0] <- [esp] (src[1])
		if_bounds(2, 2){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_assign, cinstr->dsts[0], 1, { cinstr->srcs[1] }, false };
		}
		else_bounds;

	case OP_mov_st:
	case OP_mov_ld:
	case OP_mov_imm:
	case OP_movzx:
		// dst[0] <- src[0]
		if_bounds(1, 1){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_assign, cinstr->dsts[0], 1, { cinstr->srcs[0] }, false };
		}
		else_bounds;


	case OP_imul:
		// 1st flavour -> 1 dst * 2 src
		//dst[0] <- src[0] * src[1]
		if_bounds(1, 2){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_mul, cinstr->dsts[0], 2, { cinstr->srcs[0], cinstr->srcs[1] }, true };
		}
		else_bounds;

	case OP_idiv:
		//dst - edx / dx , eax / ax , src - src[0], edx / dx , eax / ax
		if_bounds(2, 3){
			rinstr = new rinstr_t[3];
			amount = 3;
			/* create an operand for the virtual register */
			operand_t virtual_reg = { REG_TYPE, 2 * cinstr->srcs[1].width, DR_REG_VIRTUAL_1 };
			reg_to_mem_range(&virtual_reg);

			//virtual <- edx:eax
			rinstr[0] = { op_concat, virtual_reg, 2, { cinstr->srcs[1], cinstr->srcs[2] }, false };

			//edx <- virtual % src[0]
			rinstr[1] = { op_mod, cinstr->dsts[0], 2, { virtual_reg, cinstr->srcs[0] }, true };

			//eax <- virtual / src[0]
			rinstr[2] = { op_div, cinstr->dsts[1], 2, { virtual_reg, cinstr->srcs[0] }, true };
		}
		else_bounds;

	case OP_cdq:
		// edx <- eax
		if_bounds(1, 1){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_signex, cinstr->dsts[0], 1, { cinstr->srcs[0] }, true };

		}
		else_bounds;

	case OP_sub:
	case OP_xor:
	case OP_add:
		// dst[0] <- src[1] (op) src[0]
		if_bounds(1, 2){
			rinstr = new rinstr_t[1];
			amount = 1;
			switch (cinstr->opcode){
			case OP_sub: operation = op_sub; break;
			case OP_add: operation = op_add; break;
			case OP_xor: operation = op_xor; break;
			}
			rinstr[0] = { operation, cinstr->dsts[0], 2, { cinstr->srcs[0], cinstr->srcs[1] }, false };
		}
		else_bounds;

	case OP_sar:

		if_bounds(1, 2){
			rinstr = new rinstr_t[1];
			amount = 1;
			rinstr[0] = { op_rsh, cinstr->dsts[0], 2, { cinstr->srcs[1], cinstr->srcs[0] }, true };
		}
		else_bounds;

	case OP_jmp:
	case OP_jmp_short:
	case OP_jnl:
	case OP_jnl_short:
	case OP_jl:
	case OP_jnle:
	case OP_jnle_short:
	case OP_cmp:
	case OP_ret:
	case OP_call:
		break;

	default:
		unhandled = true;
	

	}

	ASSERT_MSG((!unhandled), ("ERROR: opcode %d with %d dests and %d srcs (app_pc - %d) not handled in canonicalization\n",cinstr->opcode,cinstr->num_dsts,cinstr->num_srcs,cinstr->pc));

	if(rinstr == NULL){
		DEBUG_PRINT(("opcode skipped\n"),2);
	}
	else{
		DEBUG_PRINT(("opcode reduced\n"),2);
	}

	return rinstr;
}



static void populate_rinstr(rinstr_t * rinstr,
					 operand_t dst,
					 int num_srcs,
					 operand_t src1,
				     operand_t src2,
					 int operation,
					 bool sign){

	rinstr->dst = dst; 
	rinstr->num_srcs = num_srcs;
	rinstr->srcs[0] = src1;
	if(num_srcs > 1){
		rinstr->srcs[1] = src2;
	}
	rinstr->operation = operation;
	rinstr->sign = sign;


}


