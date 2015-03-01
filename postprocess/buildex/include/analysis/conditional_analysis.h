#ifndef _CONDITIONAL_ANALYSIS_H
#define _CONDITIONAL_ANALYSIS_H


#include <stdint.h>
#include "analysis/staticinfo.h"
#include "memory/memregions.h"
#include "analysis/x86_analysis.h"

std::vector<uint32_t> find_dependant_statements(
					vec_cinstr &instrs, 
					mem_regions_t * mem, 
					std::vector<Static_Info *> static_info);

std::vector<Jump_Info *> find_dependant_conditionals(
			std::vector<uint32_t> dep_instrs, 
			vec_cinstr &instrs, 
			std::vector<Static_Info *> &static_info);

void populate_conditional_instructions(std::vector<Static_Info *> &static_info, std::vector<Jump_Info *> jumps);
Static_Info * get_instruction_info(std::vector<Static_Info*> &static_info, uint32_t pc);


#endif