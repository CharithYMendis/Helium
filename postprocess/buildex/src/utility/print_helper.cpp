#include <fstream>
#include <string>

#include "utility/defines.h"
#include "common_defines.h"

#include "utility/print_helper.h"
#include "analysis/x86_analysis.h" /* all the x86 and Helium related risc defines */


using namespace std;

/************************************************************************/
/*       generic print strings of x86 related opnds, operations etc.	*/
/************************************************************************/

/* node - opnd to string */
string opnd_to_string(operand_t * opnd){

	string label = "";

	if (opnd->type == REG_TYPE){
		uint32_t offset = opnd->value - (opnd->value / MAX_SIZE_OF_REG) * MAX_SIZE_OF_REG;
		label += to_string(opnd->value) + ":r[" + regname_to_string(mem_range_to_reg(opnd)) + ":" + to_string(offset) + "]";
	}
	else if (opnd->type == IMM_FLOAT_TYPE){
		label += "imm[" + to_string(opnd->float_value) + "]";
	}
	else if (opnd->type == IMM_INT_TYPE){
		label += "imm[" + to_string((int)opnd->value) + "]";
	}
	else if (opnd->type == MEM_STACK_TYPE){
		label += "ms[" + to_string(opnd->value) + "]";
	}
	else if (opnd->type == MEM_HEAP_TYPE){
		label += "mh[" + to_string(opnd->value) + "]";
	}

	label += "{" + to_string(opnd->width) + "}";

	return label;
}

/* get the operations to string */
string operation_to_string(uint operation){

	switch (operation){
	case op_assign: return "=";
	case op_add: return "+";
	case op_sub: return "-";
	case op_mul: return "*";
	case op_div: return "/";
	case op_mod: return "%";
	case op_lsh: return "<<";
	case op_rsh: return ">>";
	case op_not: return "~";
	case op_xor: return "^";
	case op_and: return "&";
	case op_or: return "|";
	case op_concat: return ",";
	case op_signex: return "SE";
	case op_full_overlap: return "FO";
	case op_partial_overlap: return "PO";
	case op_split_l: return "SL";
	case op_split_h: return "SH";

	case op_lt: return "<";
	case op_le: return "<=";
	case op_gt: return ">";
	case op_ge: return ">=";
	case op_eq: return "==";
	case op_neq: return "!=";

	case op_indirect: return "indirect";

	default: return "__";
	}

}

/* get the regname to string */
string regname_to_string(uint reg){

#include "utility/print_regs.h"
	return "ERROR";
}

/* DR operations to string */
string dr_operation_to_string(uint operation){

#include "utility/print_ops.h"
	return "ERROR";

}

/************************************************************************/
/*                  dot file related prints                             */
/************************************************************************/


std::string dot_get_edge_string(uint32_t from, uint32_t to){
	return to_string(from) + " -> " + to_string(to) + ";";
}


std::string dot_get_node_string(uint32_t node_num, std::string node_string){
	return to_string(node_num) + " [label=\"" + node_string +  "\"];";
}

