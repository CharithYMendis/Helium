#include <assert.h>
#include <fstream>
#include <iostream>

#include "utility/defines.h"
#include "common_defines.h"
#include "analysis/x86_analysis.h"
#include "trees/nodes.h"
#include "utility/print_helper.h"

using namespace std;


/* conc node */
Conc_Node::Conc_Node(operand_t * symbol)
{
	operand_t * operand = new operand_t();
	operand->type = symbol->type;
	operand->width = symbol->width;
	if (symbol->type == IMM_FLOAT_TYPE){
		operand->float_value = symbol->float_value;
	}
	else{
		operand->value = symbol->value;
	}

	this->symbol = operand;
	this->operation = -1;
	this->order_num = -1;
	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	
	this->is_para = false;
	this->is_double = false;

	this->line = 0;
	this->pc = 0;
	region = NULL;
	
}

void assign_mem_region(Conc_Node * node, vector<mem_regions_t *> regions){

	if (node->symbol->type == MEM_HEAP_TYPE || node->symbol->type == MEM_STACK_TYPE){
		node->region = get_mem_region(node->symbol->value, regions);
	}

}


Conc_Node::Conc_Node(operand_t * symbol, vector<mem_regions_t *> &regions) : Conc_Node(symbol){
	assign_mem_region(this, regions);
}

Conc_Node::Conc_Node(uint32_t type, uint64_t value, uint32_t width, float float_value)
{
	operand_t * sym = new operand_t;
	sym->type = type;
	if (type != IMM_FLOAT_TYPE){
		sym->value = value;
	}
	else{
		sym->float_value = float_value;
	}
	sym->width = width;

	this->symbol = sym;
	this->operation = -1;
	this->order_num = -1;

	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	
	this->is_para = false;
	this->is_double = false;

	this->line = 0;
	this->pc = 0;
	region = NULL;
	
}

Conc_Node::~Conc_Node()
{
	delete this->symbol;
}

/* yet to be filled */
string Conc_Node::get_node_string()
{
	return operation_to_string(this->operation) + "\\n" 
		+ opnd_to_string(this->symbol) + "\n"
		+ to_string(this->pc) + " "
		+ to_string(this->line);
}

bool Conc_Node::are_nodes_similar(Node * node){
	
	if (node->srcs.size() > 0){
		return (operation == node->operation) &&
			(srcs.size() == node->srcs.size());
	}
	else{
		if (symbol->type == node->symbol->type){
			if (symbol->type == IMM_INT_TYPE){
				return symbol->value == node->symbol->value;
			}
			else if (symbol->type == IMM_FLOAT_TYPE){
				return abs(symbol->float_value - node->symbol->float_value) < 1e-6;
			}
			else{
				return true;
			}
		}
		else{
			return false;
		}
	}
	

}

string Conc_Node::get_dot_string(){
	return get_node_string();
}

string Conc_Node::get_simpl_string(){
	throw "not implemented!";
}

