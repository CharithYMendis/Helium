#ifndef _UTILITIES_EXALGO_H
#define _UTILITIES_EXALGO_H

#include "dr_api.h"
#include "defines.h"
#include "moduleinfo.h"

/* these are externs which are defined in main.c and can be used in any instrumentation pass */
extern char logdir[MAX_STRING_LENGTH];
extern bool debug_mode;
extern bool log_mode;
extern file_t global_logfile;

/* provides various filtering functions - all the filtering is done through runtime */
bool filter_bb_level_from_list (module_t * head, instr_t * instr);
bool filter_module_level_from_list (module_t * head, instr_t * instr);
bool filter_range_from_list (module_t * head, instr_t * instr); /* can be used for function calls */
bool filter_from_list(module_t * head, instr_t * instr, uint mode); /* can be used for clients who do not need to do extra processing after filter for each differently */
bool filter_from_module_name(module_t * head, char * name, uint mode);

/* other utility functions */
bool get_offset_from_module(app_pc instr_addr, uint * offset);
uint populate_conv_filename(char * dest,const char * folder,const char * name, const char * other_details);

#endif