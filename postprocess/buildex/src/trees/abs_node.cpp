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

Abs_Node::Abs_Node(Conc_Node * head, Conc_Node * conc_node, vector<mem_regions_t *> &mem_regions) : Node(*conc_node) {

	

	mem_regions_t * mem = NULL;
	this->mem_info.associated_mem = NULL;
	if (conc_node->symbol->type == MEM_STACK_TYPE || conc_node->symbol->type == MEM_HEAP_TYPE){
		mem = get_mem_region(conc_node->symbol->value, mem_regions);
	}

	this->minus = false;
	this->operation = conc_node->operation;
	this->para_num = conc_node->para_num;
	this->symbol = conc_node->symbol;
	this->func_name = conc_node->func_name;

	bool filter = false;
	if (conc_node == head) filter = true;
	else if (conc_node->srcs.size() == 0) filter = true;
	else{
		for (int i = 0; i < conc_node->srcs.size(); i++){
			if (conc_node->srcs[i]->operation == op_indirect) filter = true;
		}
	}

	int32_t pos = head->is_node_indirect();
	if (pos != -1){
		head = (Conc_Node *)head->srcs[pos]->srcs[0];
	}

	/* mem buffers in the head or in the leaves with at most an indirection */
	if (mem != NULL && filter){

		//ASSERT_MSG((conc_node == head || conc_node->srcs.size() <= 1), ("ERROR: mem buffers in the head or in the leaves with at most an indirection\n"));

		mem_regions_t * mem = get_mem_region(conc_node->symbol->value, mem_regions);
		this->operation = conc_node->operation;
		this->symbol = conc_node->symbol;

		switch (mem->direction){
		case MEM_INPUT: this->type = INPUT_NODE; break;
		case MEM_OUTPUT: this->type = OUTPUT_NODE; break;
		case MEM_INTERMEDIATE: this->type = INTERMEDIATE_NODE; break;
		}


		this->mem_info.associated_mem = mem;
		this->mem_info.dimensions = mem->dimensions;
		this->mem_info.indexes = new int *[mem->dimensions];
		this->mem_info.pos = new int[mem->dimensions];

		mem_regions_t * head_region = get_mem_region(head->symbol->value, mem_regions);
		ASSERT_MSG(head_region != NULL, ("ERROR: the head region cannot be NULL"));

		this->mem_info.head_dimensions = head_region->dimensions;

		vector<int> pos = get_mem_position(mem, conc_node->symbol->value);
		for (int i = 0; i < mem->dimensions; i++){
			this->mem_info.indexes[i] = new int[head_region->dimensions + 1];
			this->mem_info.pos[i] = pos[i];
		}

	}
	else if ((conc_node->symbol->type == MEM_HEAP_TYPE) || (conc_node->symbol->type == MEM_STACK_TYPE) || (conc_node->symbol->type == REG_TYPE)){ /*parameters can be here*/
		this->symbol = conc_node->symbol;
		//ASSERT_MSG((mem == NULL), ("ERROR: we cannot have a buffer here\n"));
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
		for (int j = 0; j < this->mem_info.head_dimensions; j++){


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

		if (this->mem_info.indexes[i][this->mem_info.head_dimensions] != 0){
			if (!first){
				ret += "+";
			}
			ret += to_string(this->mem_info.indexes[i][this->mem_info.head_dimensions]);
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
		return to_string((int32_t)symbol->value);
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

string Abs_Node::get_immediate_string(vector<string> vars){

	string ret = "";
	for (int i = 0; i < mem_info.head_dimensions + 1; i++){
		if (i == mem_info.head_dimensions){
			ret += to_string(mem_info.indexes[0][i]);
		}
		else if(mem_info.indexes[0][i] != 0){
			ret += to_string(mem_info.indexes[0][i]) + " * " + vars[i] + " + ";
		}
	}

	return ret;

}

string Abs_Node::get_symbolic_string(vector<string> vars){

	string ret = "";
	if ((type == INPUT_NODE) || (type == OUTPUT_NODE) || (type == INTERMEDIATE_NODE)){
		ret = get_mem_string(vars);
	}
	else if (type == IMMEDIATE_INT){
		ret = get_immediate_string(vars);
	}
	else{
		ret = get_node_string();
	}

	//ret += " " + to_string(this->minus);
	return ret;

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
				return true;
				//return symbol->value == abs_node->symbol->value;
			}
			else if (type == IMMEDIATE_FLOAT){
				return abs(symbol->float_value - abs_node->symbol->float_value) < 1e-6;
			}
			else if (type == INPUT_NODE || type == OUTPUT_NODE || type == INTERMEDIATE_NODE){
				if (node->srcs.size() > 0 && node->srcs[0]->operation == op_indirect) return true;
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

