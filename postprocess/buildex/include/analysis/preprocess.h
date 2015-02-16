#ifndef _PREPROCESS_H
#define _PREPROCESS_H

#include  "..\..\..\dr_clients\include\output.h"
#include <string>
#include <iostream>
#include <stdint.h>
#include <vector>

/* preprocess mutating routines */





void filter_disasm_vector(
					vec_cinstr &instrs, 
					Static_Info * static_info);

void cinstr_convert_reg(cinstr_t * instr);
					
/* information gathtering routines */

 vector<uint32_t> get_instrace_startpoints(
				vec_cinstr &instrs, 
				uint32_t pc);

 #endif
 
