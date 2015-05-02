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
std::vector<uint32_t> get_instrace_startpoints(vec_cinstr &instrs, std::vector<uint32_t> pcs);

vec_cinstr filter_instr_trace(uint32_t start_pc, uint32_t end_pc, vec_cinstr &unfiltered_instrs);
vec_cinstr filter_instr_trace(std::vector<uint32_t> start_pcs, std::vector<uint32_t> end_pcs, vec_cinstr &unfiltered_instrs);

std::pair<uint32_t, uint32_t> get_start_end_pcs(std::vector<Static_Info *> &infos, Static_Info * first);



 #endif
 
