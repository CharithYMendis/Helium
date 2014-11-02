#ifndef _BUILDEX_FORWARD_ANALYSIS
#define _BUILDEX_FORWARD_ANALYSIS

#include "canonicalize.h"
#include "memregions.h"
#include <stdint.h>


typedef struct _jump_info_t {

	uint32_t jump_pc;
	uint32_t target_pc;
	uint32_t fall_pc;
	uint32_t cond_pc;
	uint32_t merge_pc;
	uint32_t trueFalse[2];

} jump_info_t;


typedef struct instr_info_t {

	uint32_t pc;
	string   disasm;
	vector< pair<jump_info_t *, bool> > conditions; /* if a condition is attached to this instruction */

};

typedef vector< pair<uint32_t, vector<jump_info_t *> > > inverse_table_t;

std::vector<uint32_t> find_dependant_statements(vec_cinstr &instrs, mem_regions_t * mem, std::vector<disasm_t *> disasm_vec);
std::vector<jump_info_t *> find_depedant_conditionals
			(std::vector<uint32_t> dep_instrs, vec_cinstr &instrs, std::vector<disasm_t *> disasm);
std::vector<instr_info_t *> populate_conditional_instructions(std::vector<disasm_t *> disasm, std::vector<jump_info_t *> jumps);


std::vector<jump_info_t * > lookup_inverse_table(inverse_table_t jump_table, uint32_t pc);
inverse_table_t get_inverse_jumptable(std::vector<jump_info_t *> jumps);


instr_info_t * get_instruction_info(std::vector<instr_info_t *> instr, uint32_t pc);


#endif