#include "expression_tree.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include "defines.h"
#include "print.h"

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


	//TODO: have precomputed nodes for immediate integers -> can we do it for floats as well? -> just need to point to them in future (space optimization)

	if(head == NULL){
		head = new Node(&instr->dst);
		//head->operation = op_assign;
		int hash = generate_hash(&instr->dst);

		//we cannot have a -1 here! - give out an error in future
		if(hash != -1){
			int amount = frontier[hash].amount;
			frontier[hash].bucket[amount] = head;
			frontier[hash].amount++;
		}
	}

	//first get the destination
	int hash_dst = generate_hash(&instr->dst);

	DEBUG_PRINT(("dst_hash - %d\n", hash_dst), 3);

	Node * dst = search_node(hash_dst,instr->dst.value);

	if(dst == NULL){
		DEBUG_PRINT(("not affecting the frontier\n"), 3);
		return;  //this instruction does not affect the slice
	}
	else{
		DEBUG_PRINT(("affecting the frontier\n"), 3);
	}

	//update operation
	dst->operation = instr->operation;
	DEBUG_PRINT(("operation - %d\n", dst->operation), 3);


	//ok now to remove the destination from the frontiers
	remove_from_frontier(hash_dst,instr->dst.value);

	//update srcs
	for(int i=0; i<instr->num_srcs; i++){
		
		//first check whether there are existing nodes in the frontier for these sources
		//if there is we know that the same definition of src is used for this, so we can just point to it rather than 
		//creating a new node -> space and time efficient
		int hash_src = generate_hash(&instr->srcs[i]);

		DEBUG_PRINT(("src hash - %d\n", hash_src), 3);

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
			DEBUG_PRINT(("node added\n"), 3);
		}


		/* assign operation optimization - space */
		bool assign_opt = false;
		
		if ( (instr->num_srcs == 1) && (instr->operation == op_assign) ){  //this is just an assign then remove the current node and place the new src node -> compiler didn't optimize for this?
			for (int i = 0; i < dst->num_references; i++){
				src->num_references++;
				src->prev.push_back(dst->prev[i]);
				if (dst->lr[i] == NODE_RIGHT){
					dst->prev[i]->right = src;
					src->lr.push_back(NODE_RIGHT);
				}
				else if (dst->lr[i] == NODE_LEFT){
					dst->prev[i]->left = src;
					src->lr.push_back(NODE_LEFT);
				}
				assign_opt = true;
			}
		}

		/* update the tree + backward references */

		if (!assign_opt){
			src->num_references++;
			if (instr->num_srcs == 1){   //unary operation so place the operand to the right
				ASSERT_MSG(i == 0, ("ERROR: not the first source in a single source instruction\n"));
				dst->right = src;
				src->prev.push_back(dst);
				src->lr.push_back(NODE_RIGHT);
			}
			else{
				if (i == 0){
					dst->left = src;
					src->prev.push_back(dst);
					src->lr.push_back(NODE_LEFT);
				}
				else{
					dst->right = src;
					src->prev.push_back(dst);
					src->lr.push_back(NODE_RIGHT);
				}
			}
		}

#ifdef DEBUG
#if DEBUG_LEVEL >= 3
		flatten_to_expression(head,cout);
		cout << endl;
#endif
#endif

		//update the frontiers - include the sources to the frontier if new nodes created
		if(add_node) add_to_frontier(hash_src,src);
			
	}

	

}

Node* Expression_tree::get_head(){
	return head;
}