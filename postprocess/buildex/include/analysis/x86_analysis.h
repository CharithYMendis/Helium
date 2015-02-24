#ifndef _X86_ANALYSIS_H
#define _X86_ANALYSIS_H

 //this will be coded as a c file which can be both used inside 
 //therefore, will not use any C++ features 

#include  "..\..\..\dr_clients\include\output.h"
#include "analysis\staticinfo.h"
#include <string>
#include <iostream>
#include <stdint.h>
#include <vector>

using namespace std;

#define MAX_SIZE_OF_REG 32

#define FORWARD_ANALYSIS    1
#define BACKWARD_ANALYSIS   2

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

	/* logical operations */
	op_ge,
	op_gt,
	op_le,
	op_lt,
	op_eq,
	op_neq,


	op_unknown

};


 typedef output_t cinstr_t;

 typedef std::vector< std::pair<cinstr_t *, Static_Info * > >  vec_cinstr;
 
 typedef struct _rinstr_t{
 
	int operation;
	operand_t dst;
	uint num_srcs;
	operand_t srcs[2];
	bool sign;
	bool is_floating;
	
 
 } rinstr_t;

 
/* reducing cinstrs to rinstrs */
 rinstr_t * cinstr_to_rinstrs_eflags
			(cinstr_t * cinstr, 
			int &amount, 
			std::string disasm, 
			uint32_t line);
 rinstr_t * cinstr_to_rinstrs (
			cinstr_t * cinstr, 
			int &amount, 
			std::string disasm, 
			uint32_t line);
 void cinstr_convert_reg(cinstr_t * instr);
			
 void	reg_to_mem_range(operand_t * opnd);
 int	mem_range_to_reg(operand_t * opnd);

 void	update_floating_point_regs(
	 vec_cinstr  &instrs,
	 uint32_t direction,
	 std::vector<Static_Info * > static_info,
	 uint32_t pc);
 void	update_regs_to_mem_range(vec_cinstr  &instrs);

 /* x86 instruction analysis functions */
 bool		is_conditional_jump_ins(uint32_t opcode);
 uint32_t	is_eflags_affected(uint32_t opcode);
 bool		is_jmp_conditional_affected(uint32_t opcode, uint32_t flags);
 bool		is_branch_taken(uint32_t opcode, uint32_t flags);
 uint32_t	dr_logical_to_operation(uint32_t opcode);

 bool		is_floating_point_ins(uint32_t opcode);
 bool		is_floating_point_reg(operand_t * opnd);

 /* debug routines */
 void print_cinstr(cinstr_t * instr);
 void print_rinstrs(rinstr_t * rinstr, int amount);

 
 #endif
 
