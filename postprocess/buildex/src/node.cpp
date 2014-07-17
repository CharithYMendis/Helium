#include <stdio.h>
#include "node.h"
#include "defines.h"


Node::Node(operand_t * symbol){

	this->symbol = symbol;
	this->operation = -1;
	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	this->order_num = -1;
	this->is_para = false;

}

Node::Node(uint type, uint value, uint width, float float_value){

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
	
}

Node::~Node(){

}







