#ifndef _PREPROCESS_H
#define _PREPROCESS_H

#include  "..\..\..\dr_clients\include\output.h"
#include <string>
#include <iostream>
#include <stdint.h>
#include <vector>

#include "analysis/x86_analysis.h"
#include "analysis/staticinfo.h"

/* preprocess mutating routines */
void filter_disasm_vector(vec_cinstr &instrs, std::vector<Static_Info *> &static_info);
					
/* information gathtering routines */
std::vector<uint32_t> get_instrace_startpoints(vec_cinstr &instrs, uint32_t pc);

 #endif
 
