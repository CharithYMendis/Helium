#ifndef _CONDITIONAL_ANALYSIS_H
#define _CONDITIONAL_ANALYSIS_H

#include "canonicalize.h"
#include "memregions.h"
#include <stdint.h>




std::vector<uint32_t> find_dependant_statements(
					vec_cinstr &instrs, 
					mem_regions_t * mem, 
					std::vector<Static_Info *> static_info);

std::vector<jump_info_t *> find_depedant_conditionals(
			std::vector<uint32_t> dep_instrs, 
			vec_cinstr &instrs, 
			std::vector<Static_Info *> static_info);

std::vector<instr_info_t *> populate_conditional_instructions(std::vector<disasm_t *> disasm, std::vector<jump_info_t *> jumps);


std::vector<jump_info_t * > lookup_inverse_table(inverse_table_t jump_table, uint32_t pc);
inverse_table_t get_inverse_jumptable(std::vector<jump_info_t *> jumps);


instr_info_t * get_instruction_info(std::vector<instr_info_t *> instr, uint32_t pc);


#endif