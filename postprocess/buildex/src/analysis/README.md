Guidelines for developers
========================
x86_analysis.cpp
----------------

This file handles the canonicalization of complex x86 instructions with multiple destinations and multiple sources to a set of 1 destination and 2 source reduced architecture instructions.

Further, it has an ensemble of functions for checking which instructions affect the condition codes, which instructions are conditional jumps and what flags affect those jumps.

Before developing any code in this file, please download the Intel software developer’s manual, which can be found [here](http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf)

# Instructions for developers


## Development flow for introducing new instructions

1.	If the instruction affects any destination (it has one or more destination operands), then function cinstr_to_rinstrs should be updated.
2.	Else if the instruction affects the condition codes, then it should be updated in the cinstr_to_rinstrs_eflags function.
3.	If the instruction affects eflags in any way, then it should be updated in the is_eflags_affected function.
4.	If the instruction is a conditional jump instruction update ‘is_conditional_jump_ins’ and ‘is_jmp_conditional_affected’. Also update the function ‘is_branch_taken’.
10.	As a sanity check for any instruction you handle, please include that in ‘is_instr_handled’ function. This acts as a checklist/function to check which instructions are handled in the above-mentioned functions.

Please refer the source code for comments, which describe the arguments and return types of each function, data structure.

## Steps for reduction of x86 instructions

The following guidelines apply when adding new x86 instructions to the functions *cinstr_to_rinstrs* and *cinstr_to_rinstrs_eflags*. 

First we need to distinguish between the flavors of the instruction and this is accomplished by checking the number of destinations and sources in the complex x86 instruction. These sources and destinations include all explicit and implicit operands. Convenience macros, ‘if_bounds’, ‘elseif_bounds’ and ‘else_bounds’ can be used for this task.

For each of the flavors,
*	First sketch up how you are going break up the complex instruction into the reduced set of instructions.
*	Next, create a rinstr_t array to match the amount of reduced instructions you need to create and write this amount to the variable *amount* which is passed by reference to the function.
*	If you are introducing any new operand_t’s (e.g., virtual registers for temporaries) make sure to run them through reg_to_mem_range function before use.
*	Next populate the array of rinstr_t. Make sure to comment how exactly you broke up the x86 instruction.
Note the following as well.
The opcodes used in complex x86 instructions are not the same as the opcodes used in the reduced set of instructions. x86 instructions start with a capital ‘O’ and reduced set instructions start with a simple ‘o’.

## Canonicalization example 

Following example shows how some of the flavors of Intel’s imul instruction are canonicalized to reduced instructions in *cinstr_to_rinstrs* function. Similar procedure can be adopted for reducing instructions in *cinstr_to_rinstrs_eflags* function
 
	case OP_imul:
			// 1st flavour -> 1 dst * 2 src
			//dst[0] <- src[0] * src[1]
	if_bounds(1, 2){
		rinstr = new rinstr_t[1];
		amount = 1;
		rinstr[0] = { op_mul, cinstr->dsts[0], 2, { cinstr->srcs[0], cinstr->srcs[1] }, true };
	}
	elseif_bounds(2, 2){
		// edx [dst0] : eax [dst1] <- eax [src1] * [src0] 
		rinstr = new rinstr_t[3];
		amount = 3;
		// create an operand for the virtual register
		operand_t virtual_reg = { REG_TYPE, 2 * cinstr->srcs[1].width, DR_REG_VIRTUAL_1 };
		reg_to_mem_range(&virtual_reg);

		//virtual <= eax * src0
		rinstr[0] = { op_mul, virtual_reg, 2, { cinstr->srcs[1], cinstr->srcs[0] }, true };

		//edx <= split_h(virtual)
		rinstr[1] = { op_split_h, cinstr->dsts[0], 1, { virtual_reg }, true };
	}
	else_bounds;






