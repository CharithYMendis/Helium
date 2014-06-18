#include <stdio.h>
#include "canonicalize.h"
#include "defines.h"

static void populate_rinstr(rinstr_t * rinstr,
					 operand_t dst,
					 int num_srcs,
					 operand_t src1,
				     operand_t src2,
					 int operation);


rinstr_t * cinstr_to_rinstrs (cinstr_t * cinstr, int &amount){


	int operation;

	printf("entering canonicalization\n");

	rinstr_t * rinstr;
	rinstr = NULL;
	amount = 0;
	

	switch(cinstr->opcode){
	
	case OP_push:
		// [esp - 4] (dst[1]) <- src[0]
		rinstr = new rinstr_t[1];
		amount = 1;
		populate_rinstr(&rinstr[0],
						cinstr->dsts[1],
						1,
						cinstr->srcs[0],
						cinstr->srcs[1],
						op_assign);
		break;

	case OP_pop:
		// dst[0] <- [esp] (src[1])
		rinstr = new rinstr_t[1];
		amount = 1;
		populate_rinstr(&rinstr[0],
						cinstr->dsts[0],
						1,
						cinstr->srcs[1],
						cinstr->srcs[0],
						op_assign);
		break;

	case OP_mov_st:
	case OP_mov_ld:
		// dst[0] <- src[0]
		rinstr = new rinstr_t[1];
		amount = 1;
		populate_rinstr(&rinstr[0],
						cinstr->dsts[0],
						1,
						cinstr->srcs[0],
						cinstr->srcs[1],
						op_assign);
		break;

	
	case OP_sub:
	case OP_xor:
	case OP_add:
		// dst[0] <- src[1] (op) src[0]
		rinstr = new rinstr_t[1];
		amount = 1;
		switch(cinstr->opcode){
			case OP_sub: operation = op_sub; break;
			case OP_add: operation = op_add; break;
			case OP_xor: operation = op_xor; break;
		}
		populate_rinstr(&rinstr[0],
						cinstr->dsts[0],
						2,
						cinstr->srcs[0],
						cinstr->srcs[1],
						operation);
		

	case OP_jmp:
	case OP_jnl:
	case OP_cmp:
	case OP_ret:
		break;

	}

	if(rinstr == NULL){
		printf("skipped\n");
	}
	else{
		printf("reduced\n");
	}

	return rinstr;
}



static void populate_rinstr(rinstr_t * rinstr,
					 operand_t dst,
					 int num_srcs,
					 operand_t src1,
				     operand_t src2,
					 int operation){

	rinstr->dst = dst; 
	rinstr->num_srcs = num_srcs;
	rinstr->srcs[0] = src1;
	if(num_srcs > 1){
		rinstr->srcs[1] = src2;
	}
	rinstr->operation = operation;


}


