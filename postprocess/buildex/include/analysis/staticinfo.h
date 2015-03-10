#ifndef _STATICINFO_H
#define _STATICINFO_H

/* static information storage for dynamically executed instructions - this data structure will summarize 
all the static information found about dynamically executed instructions */

#include <string>
#include <stdint.h>
#include <vector>

struct Jump_Info {

	uint32_t jump_pc; // the pc of the conditional jump
	uint32_t cond_pc; // eflags set by this pc
	uint32_t target_pc; // the target pc of this jump
	uint32_t fall_pc;   // the fall-through pc 
	uint32_t merge_pc;  // the merge point for the taken and not taken paths

	// example lines in the instruction trace for taken and notTaken jump conditionals
	uint32_t taken;
	uint32_t not_taken;

};

class Static_Info {

public:

	enum Instr_type {

		NONE = 0,
		INPUT = 1 << 0,
		INPUT_DEPENDENT_DIRECT = 1 << 1,
		INPUT_DEPENDENT_INDIRECT = 1 << 2,
		CONDITIONAL = 1 << 3,
		LOOP = 1 << 4,
		OUTPUT = 1 << 5

	};

	uint32_t module_no; // module for this instruction (we encode as integers)
	uint32_t pc; // the program counter value for this instruction - for a jump instruction this is the jump pc
	std::string disassembly; // disassembly string

	Instr_type type; // type of the instruction
	std::vector< std::pair<Jump_Info *, bool> > conditionals; // are there any input dependent conditionals?
	
	Static_Info();
	~Static_Info();

};

Static_Info * get_static_info(std::vector<Static_Info *> instr, uint32_t pc);
Static_Info * get_static_info(std::vector<Static_Info *> instr, Jump_Info * jump);

#endif