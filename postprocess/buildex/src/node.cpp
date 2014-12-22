#include <stdio.h>
#include "node.h"
#include "defines.h"
#include <iostream>

using namespace std;

Node::Node(operand_t * symbol){

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
	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	this->order_num = -1;
	this->is_para = false;
	this->line = 0;
	this->pc = 0;
	this->is_double = false;
}

Node::Node(uint type, uint64 value, uint width, float float_value){

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
	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	this->order_num = -1;
	this->is_para = false;
	this->line = 0;
	this->pc = 0;
	this->is_double = false;
	
}

Node::~Node(){
	delete this->symbol;
}







