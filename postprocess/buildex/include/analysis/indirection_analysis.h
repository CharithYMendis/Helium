#ifndef BUILDEX_INDIRECTION
#define BUILDEX_INDIRECTION

#include <vector>
#include <stdint.h>

#include "memory/memregions.h"
#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"

std::vector< std::vector<uint32_t> > find_dependant_statements_with_indirection
					(vec_cinstr &instrs, mem_regions_t * mem, std::vector<Static_Info *> static_info);
void update_dependant_indireciton(vector<uint32_t> dep, vector<Static_Info *> &static_info);



#endif