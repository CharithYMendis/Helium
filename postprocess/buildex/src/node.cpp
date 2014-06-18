#include <stdio.h>
#include "node.h"

Node::Node(operand_t * symbol){
	this->symbol = symbol;
	this->left = NULL;
	this->right = NULL;
	this->operation = -1;
}

Node::~Node(){
	delete[] symbol;
}

Node * Node::get_left(){
	return left;
}

Node * Node::get_right(){
	return right;
}

int Node::get_operation(){
	return operation;
}

operand_t * Node::get_symbol(){
	return symbol;
}

void Node::set_left(Node * left){
	this->left = left;
}

void Node::set_right(Node * right){
	this->right = right;
}

void Node::set_operation(int operation){
	this->operation = operation;
}

void Node::set_symbol(operand_t * symbol){
	this->symbol = symbol;
}






