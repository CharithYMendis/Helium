#include <assert.h>
#include <fstream>
#include <iostream>

#include "utility/defines.h"
#include "trees/nodes.h"
#include "analysis/x86_analysis.h"
#include "utility/print_helper.h"

using namespace std;

Abs_Node::Abs_Node() : Node()
{ 

}

Abs_Node::Abs_Node(Conc_Node * conc_node, vector<mem_regions_t *> &mem_regions) : Node(*conc_node) {



	mem_regions_t * mem = NULL;
	if (conc_node->symbol->type == MEM_STACK_TYPE || conc_node->symbol->type == MEM_HEAP_TYPE){
		mem = get_mem_region(conc_node->symbol->value, mem_regions);
	}

	if (mem != NULL){

		mem_regions_t * mem = get_mem_region(conc_node->symbol->value, mem_regions);
		this->operation = conc_node->operation;
		this->symbol = conc_node->symbol;
		this->para_num = conc_node->para_num;

		switch (mem->direction){
		case MEM_INPUT: this->type = INPUT_NODE; break;
		case MEM_OUTPUT: this->type = OUTPUT_NODE; break;
		case MEM_INTERMEDIATE: this->type = INTERMEDIATE_NODE; break;
		}


		this->mem_info.associated_mem = mem;
		this->mem_info.dimensions = mem->dimensions;
		this->mem_info.indexes = new int *[mem->dimensions];
		this->mem_info.pos = new int[mem->dimensions];

		vector<int> pos = get_mem_position(mem, conc_node->symbol->value);
		for (int i = 0; i < mem->dimensions; i++){
			this->mem_info.indexes[i] = new int[mem->dimensions + 1];
			this->mem_info.pos[i] = pos[i];
		}

	}
	else if ((conc_node->symbol->type == MEM_STACK_TYPE) || (conc_node->symbol->type == REG_TYPE)){ /*parameters can be here*/
		this->operation = conc_node->operation;
		if (conc_node->is_para){
			this->type = PARAMETER;
		}
		else{
			this->type = OPERATION_ONLY;
		}
	}
	else if (conc_node->symbol->type == IMM_INT_TYPE){
		this->operation = conc_node->operation;
		
		this->type = IMMEDIATE_INT;
	}
	else if (conc_node->symbol->type == IMM_FLOAT_TYPE){
		this->operation = conc_node->operation;
		
		this->type = IMMEDIATE_FLOAT;
	}
	else{
		ASSERT_MSG((false), ("ERROR: conc_node type unknown\n"));
	}

	this->is_double = conc_node->is_double;
	/* change this sometime */
	this->sign = false;



}

Abs_Node::~Abs_Node()
{

}

/* input(10,10) */
string Abs_Node::get_mem_string(){

	string ret = this->mem_info.associated_mem->name + "(";

	for (int i = 0; i < this->mem_info.dimensions; i++){
		if (i < this->mem_info.dimensions - 1){
			ret += to_string(this->mem_info.pos[i]) + ",";
		}
		else{
			ret += to_string(this->mem_info.pos[i]) + ")";
		}
	}

	return ret;

}

/* input(x,y) */
string Abs_Node::get_mem_string(vector<string> vars){

	string ret = this->mem_info.associated_mem->name + "(";
	for (int i = 0; i < this->mem_info.dimensions; i++){

		bool first = true;
		for (int j = 0; j < this->mem_info.dimensions; j++){


			if (this->mem_info.indexes[i][j] == 1){
				if (!first){
					ret += "+";
				}
				ret += vars[j];
				first = false;

			}
			else if (this->mem_info.indexes[i][j] != 0){
				if (!first){
					ret += "+";
				}
				ret += "(" + to_string(this->mem_info.indexes[i][j]) + ")" + " * " + vars[j];
				first = false;

			}

		}

		if (this->mem_info.indexes[i][this->mem_info.dimensions] != 0){
			if (!first){
				ret += "+";
			}
			ret += to_string(this->mem_info.indexes[i][this->mem_info.dimensions]);
		}

		if (i != this->mem_info.dimensions - 1){
			ret += ",";
		}
		else{

			ret += ")";
		}

	}

	return ret;

}

string Abs_Node::get_node_string()
{
	if ((this->type == INPUT_NODE) || (this->type == OUTPUT_NODE) || (this->type == INTERMEDIATE_NODE)){
		return get_mem_string();
	}
	else if (this->type == IMMEDIATE_INT){
		return to_string(symbol->value);
	}
	else if (this->type == IMMEDIATE_FLOAT){
		return to_string(symbol->float_value);
	}
	else if (this->type == OPERATION_ONLY){
		return operation_to_string(this->operation);
	}
	else if (this->type == PARAMETER){
		return "p_" + to_string(this->para_num);
	}

	return "";
}

string Abs_Node::get_symbolic_string(vector<string> vars){


	if ((type == INPUT_NODE) || (type == OUTPUT_NODE) || (type == INTERMEDIATE_NODE)){
		return get_mem_string(vars);
	}
	else{
		return get_node_string();
	}

}

bool Abs_Node::are_nodes_similar(Node * node){

	Abs_Node * abs_node = static_cast<Abs_Node *>(node);

	if (type == abs_node->type){
		if (type == OPERATION_ONLY){
			return (operation == abs_node->operation) &&
				(srcs.size() == abs_node->srcs.size());
		}
		else{
			if (type == IMMEDIATE_INT){
				return symbol->value == abs_node->symbol->value;
			}
			else if (type == IMMEDIATE_FLOAT){
				return abs(symbol->float_value - abs_node->symbol->float_value) < 1e-6;
			}
			else if (type == INPUT_NODE || type == OUTPUT_NODE || type == INTERMEDIATE_NODE){
				return mem_info.associated_mem == abs_node->mem_info.associated_mem;
			}
			else{
				return true;
			}
		}
	}
	else{
		return false;
	}

}

string Abs_Node::get_dot_string(){
	return get_node_string();
}

string Abs_Node::get_dot_string(vector<string> vars){
	return get_symbolic_string(vars);
}

string Abs_Node::get_simpl_string(){
	throw "not implemented!";
}

