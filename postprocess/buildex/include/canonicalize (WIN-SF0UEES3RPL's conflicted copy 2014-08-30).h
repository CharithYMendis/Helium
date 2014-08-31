#ifndef _CANONICALIZE_H
#define _CANONICALIZE_H

 //this will be coded as a c file which can be both used inside 
 //therefore, will not use any C++ features 

#include  "..\..\..\dr_clients\include\output.h"

#define MAX_SIZE_OF_REG 32

//canonicalized operations
enum {

	op_assign,
	op_add,
	op_sub,
	op_mul,
	op_div,
	op_mod,
	op_lsh,
	op_rsh,
	op_not,
	op_xor,
	op_and,
	op_or,

	/*support operations*/
	op_split_h,
	op_split_l,
	op_concat,
	op_signex,

	/* to cater to different widths */
	op_partial_overlap,
	op_full_overlap,

};


 typedef output_t cinstr_t;
 
 
 typedef struct _rinstr_t{
 
	int operation;
	operand_t dst;
	uint num_srcs;
	operand_t srcs[2];
	bool sign;
	
 
 } rinstr_t;
 
 //functions
 rinstr_t * cinstr_to_rinstrs (cinstr_t * cinstr, int &amount);
 void reg_to_mem_range(operand_t * opnd);
 int mem_range_to_reg(operand_t * opnd);
 void print_rinstrs(rinstr_t * rinstr, int amount);

 
 #endif
 
