#ifndef BUILDEX_INDIRECTION
#define BUILDEX_INDIRECTION

#include <vector>
#include <stdint.h>

#include "memory/memregions.h"
#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"

std::vector< std::vector<uint32_t> > find_dependant_statements_with_indirection
					(vec_cinstr &instrs, std::vector<mem_regions_t *> mem, std::vector<Static_Info *> static_info, std::vector<uint32_t> start_points);
void populate_dependant_indireciton(std::vector<Static_Info *> &static_info, std::vector<uint32_t> dep);



#endif