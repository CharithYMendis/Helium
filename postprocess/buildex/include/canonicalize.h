#ifndef _CANONICALIZE_H
#define _CANONICALIZE_H

 //this will be coded as a c file which can be both used inside 
 //therefore, will not use any C++ features 

#include  "..\..\..\include\output.h"

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
	op_split,
	op_concat,
	op_signex,

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
 
 
 #endif
 
