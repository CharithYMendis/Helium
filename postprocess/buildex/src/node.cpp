#include <stdio.h>
#include "node.h"


Node::Node(operand_t * symbol){

	this->symbol = symbol;
	this->operation = -1;
	srcs.reserve(2); /* mostly it will be two sources */
	prev.reserve(2); /* mostly it will be one - but let's reserve two*/
	this->order_num = -1;

}

Node::~Node(){
	delete[] symbol;
}







