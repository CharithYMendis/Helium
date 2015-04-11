
#include <vector>
#include <stdint.h>
#include <string>

#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"
#include "utility/defines.h"

using namespace std;

Static_Info::Static_Info(){
	type = NONE;
	example_line = -1;
}

Static_Info::~Static_Info(){

}


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

void populate_standard_funcs(vector<Func_Info_t *> &funcs){

	Func_Info_t * func = new Func_Info_t;
	func->module_name = "";
	func->start = 15792;
	func->end = 15938;

	/* float passed in the floating point stack */
	operand_t * para = new operand_t;
	para->type = REG_TYPE;
	para->value = DR_REG_ST9;
	para->width = 10;
	reg_to_mem_range(para);
	func->parameters.push_back(para);

	/* float returned at the top of the stack */
	operand_t * ret = new operand_t;
	ret->type = REG_TYPE;
	ret->value = DR_REG_ST9;
	ret->width = 10;
	reg_to_mem_range(ret);
	func->ret = ret;

	func->func_name = "Halide::floor";

	funcs.push_back(func);

}


