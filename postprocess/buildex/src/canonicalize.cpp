#include <stdio.h>
#include "canonicalize.h"
#include "defines.h"
#include <stdlib.h>

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

	case OP_jmp:
	case OP_jmp_short:
	case OP_jnl:
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


