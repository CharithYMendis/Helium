#ifndef _UTILITIES_H
#define _UTILITIES_H

#include "dr_api.h"
#include "moduleinfo.h"

/* filtering modes shared by the files */
#define FILTER_BB		1
#define FILTER_MODULE	2
#define FILTER_RANGE	3
#define FILTER_NONE		4


/* provides various filtering functions - all the filtering is done through runtime */
bool filter_bb_level_from_list (module_t * head, instr_t * instr);
bool filter_module_level_from_list (module_t * head, instr_t * instr);
bool filter_range_from_list (module_t * head, instr_t * instr); /* can be used for function calls */
bool filter_from_list(module_t * head, instr_t * instr, uint mode); /* can be used for clients who do not need to do extra processing after filter for each differently */


#endif