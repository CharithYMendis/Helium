#include "exbuild.h"
#include <assert.h>
#include <fstream>
#include <iostream>

#define MAX_FRONTIERS		1000
#define SIZE_PER_FRONTIER	5
#define MEM_OFFSET			200
#define REG_REGION			(MAX_FRONTIERS - MEM_OFFSET)

using namespace std;

//constructors and destructors

Expression_tree::Expression_tree(){

	head = NULL;
	frontier = new frontier_t [MAX_FRONTIERS];

	for(int i=0; i<MAX_FRONTIERS; i++){

		frontier[i].bucket = new Node * [SIZE_PER_FRONTIER];
		frontier[i].amount = 0;
	}

}

Expression_tree::~Expression_tree(){
	//need to destroy all the objects and dynamic arrays created


}

//frontier update functions

Node * Expression_tree::search_node(int hash, int value){
	
	for(int i=0; i<frontier[hash].amount; i++){
		//we don't need to check for types as we seperate them out in hashing
		//could further optimize this search by having a type specific search algo.
		if(frontier[hash].bucket[i]->symbol->value == value){  
			return frontier[hash].bucket[i];
		}
	}

	return NULL;

}

int Expression_tree::generate_hash(operand_t * opnd){

	if(opnd->type == REG_TYPE){
		return opnd->value;
	}
	else if( (opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE) ){
		int offset = (opnd->value) % (MAX_FRONTIERS - MEM_OFFSET);
		return offset + MEM_OFFSET;
	}

	return -1;  //this implies that the operand was an immediate

}

void Expression_tree::remove_from_frontier(int hash, int value){

	int amount = frontier[hash].amount;
	bool move = false;
	for(int i=0; i<amount; i++){

		if(move){
			frontier[hash].bucket[i-1] = frontier[hash].bucket[i];
		}

		if(frontier[hash].bucket[i]->symbol->value == value){  //assumes that there cannot be two Nodes with same values
			//we found the place to remove
			move = true;
		}
		
	}
	
	if(move){
		frontier[hash].amount--;
	}

}

void Expression_tree::add_to_frontier(int hash, Node * node){


	assert(frontier[hash].amount <= SIZE_PER_FRONTIER);
	frontier[hash].bucket[frontier[hash].amount++] = node;

}
		
void Expression_tree::update_frontier(rinstr_t * instr){

	if(head == NULL){
		head = new Node(&instr->dst);
		head->operation = op_assign;
		int hash = generate_hash(&instr->dst);

		//we cannot have a -1 here! - give out an error in future
		if(hash != -1){
			int amount = frontier[hash].amount;
			frontier[hash].bucket[amount] = head;
			frontier[hash].amount++;
		}
	}

	//TODO : optimization for assigns which would lead imbalanced trees -> space optimization

	//first get the destination
	int hash_dst = generate_hash(&instr->dst);

	cout << "dst hash: " << hash_dst << endl;

	Node * dst = search_node(hash_dst,instr->dst.value);

	if(dst == NULL){
		cout << "not interested" << endl;
		return;  //this instruction does not affect the slice
	}
	else{
		cout << "interested" << endl;
	}

	//update operation
	dst->operation = instr->operation;
	cout << "operation:" << dst->operation << endl;

	//ok now to remove the destination from the frontiers
	remove_from_frontier(hash_dst,instr->dst.value);

	//update srcs
	for(int i=0; i<instr->num_srcs; i++){
		
		//first check whether there are existing nodes in the frontier for these sources
		//if there is we know that the same definition of src is used for this, so we can just point to it rather than 
		//creating a new node -> space and time efficient
		int hash_src = generate_hash(&instr->srcs[i]);

		cout << "src hash: " << hash_src << endl;

		bool add_node = false;
		Node * src;  //now the node can be imme or another 
		if(hash_src == -1){
			src = new Node(&instr->srcs[i]);
		}
		else{
			src = search_node(hash_src,instr->srcs[i].value);
		}

		//when do we need another node? if node is not present or if the destination matches src ( e.g : i <- i + 1 )
		//we do not need to check for immediates here as src will point to a brand new Node in that case and hence will not enter 
		//the if statement
		if( (src == NULL) || (src == dst) ){  //I think now we can remove the src == dst check -> keeping for sanity purposes
			src = new Node(&instr->srcs[i]);
			add_node = true;
			cout << "added node" << endl;
		}

			

		//update the node information
		if(i==0) dst->left = src;
		else	 dst->right = src;

		flatten_to_expression(cout);
		cout << endl;

		//update the frontiers - include the sources to the frontier if new nodes created
		if(add_node) add_to_frontier(hash_src,src);
			
	}

	

}

//printing out the final expression - these should be changed when the application evolves 
//should add support for printing to other formats when application evolves

void Expression_tree::flatten_to_expression(std::ostream &file){

	//first print the head
	print_operand(head->symbol,file);
	//print operation of =
	print_operation(head->operation,file);

	//now print the tree
	print_tree(head->left,file);

}

void Expression_tree::print_tree(Node * node, std::ostream &file){
	
	//print the values only if the current node is a leaf
	if(  (node->right == NULL) && (node->left == NULL) ){
		print_operand(node->symbol,file);
		return;
	}


	file << " ( ";
	if(node->left != NULL){
		print_tree(node->left,file);
	}

	//if mov operation then don't print it; others print the operation
	if( (node->operation != op_assign) && (node->operation != -1) ){
		print_operation(node->operation,file);
	}

	if(node->right != NULL){
		print_tree(node->right,file);
	}

	file << " ) ";



}

void Expression_tree::print_operation(int operation, std::ostream &file){

	switch(operation){
	case op_assign : file << "="; break;
	case op_add : file << "+"; break;
	case op_sub : file << "-"; break;
	case op_mul : file << "*"; break;
	case op_div : file << "/"; break;
	case op_mod : file << "%"; break;
	case op_lsh : file << "<<"; break;
	case op_rsh : file << ">>"; break;
	case op_not : file << "~"; break;
	case op_xor : file << "^"; break;
	case op_and : file << "&"; break;
	case op_or : file << "|" ; break;
	default : file << "__" ; break;
	}

}

void Expression_tree::print_operand(operand_t * opnd, std::ostream &file){

	if(opnd->type == REG_TYPE){
		file << "r[" << opnd->value << "]" ;
	}
	else if( (opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE) ){
		file << "m[" << opnd->value << "]" ;
	}
	else if(opnd->type == IMM_INT_TYPE){
		file << opnd->value ;
	}
	else if(opnd->type == IMM_FLOAT_TYPE){
		file << opnd->float_value ;
	}

}
