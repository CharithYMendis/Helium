
#include <vector>
#include <stdint.h>
#include <string>

#include "analysis/staticinfo.h"

using namespace std;


Static_Info * get_static_info(vector<Static_Info *> instr, uint32_t pc){

	for (int i = 0; i < instr.size(); i++){
		if (pc == instr[i]->pc){
			return instr[i];
		}
	}
	return NULL;
}

Static_Info * get_static_info(vector<Static_Info *> instr, Jump_Info * jump){
	for (int i = 0; i < instr.size(); i++){
		if (jump->jump_pc == instr[i]->pc){
			return instr[i];
		}

	}
	return NULL;
}


