#include "print.h"



static void print_tree(Node * node, std::ostream &file);
static void print_operation(int operation, std::ostream &file);
static void print_operand(operand_t * opnd, std::ostream &file);


void flatten_to_expression(Node * head, std::ostream &file){

	//first print the head
	print_operand(head->symbol, file);
	//print operation of =
	print_operation(op_assign, file);

	//now print the tree
	print_tree(head, file);

}

void print_tree(Node * node, std::ostream &file){

	//print the values only if the current node is a leaf
	if ((node->right == NULL) && (node->left == NULL)){
		print_operand(node->symbol, file);
		return;
	}

	//pre-print parans
	if (node->operation != op_assign){
		file << " ( ";
	}
	if (node->operation == op_concat){
		file << " { ";
	}

	if (node->left != NULL){
		print_tree(node->left, file);
	}

	//if mov operation then don't print it; others print the operation
	if ((node->operation != op_assign) && (node->operation != -1)){
		print_operation(node->operation, file);
	}

	if (node->right != NULL){
		print_tree(node->right, file);
	}

	//post-print parans
	if (node->operation == op_concat){
		file << " } ";
	}
	if (node->operation != op_assign){
		file << " ) ";
	}



}

void print_operation(int operation, std::ostream &file){

	switch (operation){
	case op_assign: file << " = "; break;
	case op_add: file << " + "; break;
	case op_sub: file << " - "; break;
	case op_mul: file << " * "; break;
	case op_div: file << " / "; break;
	case op_mod: file << " % "; break;
	case op_lsh: file << " << "; break;
	case op_rsh: file << " >> "; break;
	case op_not: file << " ~ "; break;
	case op_xor: file << " ^ "; break;
	case op_and: file << " & "; break;
	case op_or: file << " | "; break;
	case op_concat: file << " , "; break;
	case op_signex: file << " SE "; break;
	default: file << " __ "; break;
	}

}

void print_operand(operand_t * opnd, std::ostream &file){

	if (opnd->type == REG_TYPE){
		file << " r[" << opnd->value << "] ";
	}
	else if ((opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE)){
		file << " m[" << opnd->value << "] ";
	}
	else if (opnd->type == IMM_INT_TYPE){
		file << " " << opnd->value << " ";
	}
	else if (opnd->type == IMM_FLOAT_TYPE){
		file << " " << opnd->float_value << " ";
	}

}
