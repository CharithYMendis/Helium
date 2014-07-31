#ifndef _UTILITIES_EXALGO_H
#define _UTILITIES_EXALGO_H

#include "dr_api.h"
#include "defines.h"
#include "moduleinfo.h"


/* provides various filtering functions - all the filtering is done through runtime */
bool filter_bb_level_from_list (module_t * head, instr_t * instr);
bool filter_module_level_from_list (module_t * head, instr_t * instr);
bool filter_range_from_list (module_t * head, instr_t * instr); /* can be used for function calls */
bool filter_from_list(module_t * head, instr_t * instr, uint mode); /* can be used for clients who do not need to do extra processing after filter for each differently */
bool filter_from_module_name(module_t * head, char * name, uint mode);

/* other utility functions */
bool get_offset_from_module(app_pc instr_addr, uint * offset);

#endif