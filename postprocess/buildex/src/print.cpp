#include "print.h"
#include "defines.h"
#include <string>

#define MAX_STRING_LENGTH 250

/* get strings for opnds and operations etc */

void print_tree(Node * node, std::ostream &file);

static uint num = 0;

uint number_tree(Node * node){

	if (node->order_num == -1){
		node->order_num = num;
		num++;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		number_tree(node->srcs[i]);
	}

	return num;

}


string opnd_to_string(operand_t * opnd){

	string label = "";

	if (opnd->type == REG_TYPE){
		uint offset = opnd->value - (opnd->value / MAX_SIZE_OF_REG) * MAX_SIZE_OF_REG;
		label += to_string(opnd->value) + ":r[" +  regname_to_string(mem_range_to_reg(opnd)) + ":" + to_string(offset) +  "]";
	}
	else if (opnd->type == IMM_FLOAT_TYPE){
		label += "imm[" + to_string(opnd->float_value) + "]";
	}
	else if (opnd->type == IMM_INT_TYPE){
		label += "imm[" + to_string(opnd->value) + "]";
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
	default: return "__";
	}

}

/*implement these - may be write a program to automatically get these*/
string regname_to_string(uint reg){

#include "print_regs.h"
	return "ERROR";
}

string dr_operation_to_string(uint operation){

#include "print_ops.h"
	return "ERROR";

}


void flatten_to_expression(Node * head, std::ostream &file){

	file << opnd_to_string(head->symbol) << " " << operation_to_string(op_assign) << " ";

	print_tree(head,file);

}

void print_tree(Node * node, std::ostream &file){

	int no_srcs = node->srcs.size();

	if (no_srcs == 0){ /* we are at a leaf just print it*/
		file << opnd_to_string(node->symbol);
	}
	else if (no_srcs == 1){ /* unary operation */

		/* we can have a full overlap - partial overlaps will have more than one operand */
		if (node->operation == op_full_overlap){
			file << "{" << opnd_to_string(node->symbol) << " -> " << opnd_to_string(node->srcs[0]->symbol) << "}";
			file << "(";
			print_tree(node->srcs[0],file);
			file << ")";
		}
		else if (node->operation == op_assign){
			print_tree(node->srcs[0],file);
		}
		else{
			file << operation_to_string(node->operation) << " ";
			print_tree(node->srcs[0], file);
		}

	}
	else if((no_srcs == 2) && (node->operation != op_partial_overlap)){
			file << "(";
			print_tree(node->srcs[0], file);
			file << " " << operation_to_string(node->operation) << " ";
			print_tree(node->srcs[1], file);
			file << ")";		
	}
	else{
		ASSERT_MSG((node->operation == op_partial_overlap), ("ERROR: unexpected operation with more than two srcs\n"));
		/*here it is important to see how each source contributes
		another important problem is that what source updates first
		 */
		file << "(";
		for (int i = 0; i < no_srcs; i++){
			print_tree(node->srcs[0], file);
			if (i != no_srcs - 1){
				file << "," << endl;
			}
		}
		file << ")";
	}

}



