#include "expression_tree.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include "defines.h"
#include "print.h"



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

int Expression_tree::generate_hash(operand_t * opnd){

	if (opnd->type == REG_TYPE){
		return opnd->value / MAX_SIZE_OF_REG;
	}
	else if ((opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE)){
		int offset = (opnd->value) % (MAX_FRONTIERS - MEM_OFFSET);
		return offset + MEM_OFFSET;
	}

	return -1;  //this implies that the operand was an immediate

}

Node * Expression_tree::search_reg(uint reg_value){
	if (frontier[reg_value].amount == 1){
		return frontier[reg_value].bucket[0];
	}
	return NULL;
}

Node * Expression_tree::search_node(operand_t * opnd){

	uint hash = generate_hash(opnd);

	if (opnd->type == REG_TYPE){
		if (frontier[hash].amount == 1){
			return frontier[hash].bucket[0];
		}
	}
	else if( (opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE) ){
		uint64 value = opnd->value;
		uint width = opnd->width;
		for (int i = 0; i < frontier[hash].amount; i++){
			//we don't need to check for types as we seperate them out in hashing
			//could further optimize this search by having a type specific search algo.
			if ((frontier[hash].bucket[i]->symbol->value == value) && (frontier[hash].bucket[i]->symbol->width == width)){
				return frontier[hash].bucket[i];
			}
		}
	}

	return NULL;

}

void Expression_tree::remove_from_frontier(operand_t * opnd){

	uint hash = generate_hash(opnd);

	int amount = frontier[hash].amount;
	bool move = false;
	if (opnd->type == REG_TYPE){
		if (frontier[hash].amount == 1) frontier[hash].amount--;
	}
	else if ((opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE)) {
		uint64 value = opnd->value;
		uint width = opnd->width;
		for (int i = 0; i < amount; i++){
			if (move){
				frontier[hash].bucket[i - 1] = frontier[hash].bucket[i];
			}
			if ((frontier[hash].bucket[i]->symbol->value == value) && (frontier[hash].bucket[i]->symbol->width == width)){  //assumes that there cannot be two Nodes with same values
				//we found the place to remove
				move = true;
			}
		}

		if (move){
			int amount = --frontier[hash].amount;

			/* update memoization structure */
			if (amount == 0){
				for (int i = 0; i < mem_in_frontier.size(); i++){
					if (mem_in_frontier[i] == hash){
						mem_in_frontier.erase(mem_in_frontier.begin() + i);
						break;
					}
				}
			}
			
		}

	}
}


void Expression_tree::add_to_frontier(int hash, Node * node){


	assert(frontier[hash].amount <= SIZE_PER_FRONTIER);
	frontier[hash].bucket[frontier[hash].amount++] = node;

	/*if this a memory operand we should memoize it*/
	mem_in_frontier.push_back(hash);

}


void Expression_tree::get_full_overlap_nodes(vector<Node *> &nodes, operand_t * opnd){

	if (opnd->type == REG_TYPE){
		
		int wide_reg = opnd->value;
		uint reg = opnd->value;

		if ((reg <= DR_REG_BL) || (reg >= DR_REG_R8L && reg <= DR_REG_R15L)){
			/*here in the enum the offset is 16*/
			wide_reg -= 16;
			while (wide_reg >= 0){
				Node * node = search_reg(wide_reg);
				if (node != NULL) nodes.push_back(node);
				wide_reg -= 16;
			}
		}
		else if (reg <= DR_REG_BH){
			/*here the first stride is 20 then after it is 16*/
			wide_reg -= 20;
			while (wide_reg >= 0){
				Node * node = search_reg(wide_reg);
				if(node != NULL) nodes.push_back(node);
				wide_reg -= 16;
			}
		}
		else if (reg <= DR_REG_DIL){
			/* here the first stride is 28*/
			wide_reg -= 28;
			while (wide_reg >= 0){
				Node * node = search_reg(wide_reg);
				if (node != NULL) nodes.push_back(node);
				wide_reg -= 16;
			}
		}
		/* let's handle vector code as well - but not YMM */
		else if (reg <= DR_REG_MM7){
			/* here the offset is + */
			wide_reg += 8;
			Node * node = search_reg(wide_reg);
			if (node != NULL) nodes.push_back(node);
		}
	}
	else if ((opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE)){
		for (int i = 0; i < mem_in_frontier.size(); i++){
			uint index = mem_in_frontier[i];
			for (int j = 0; j < frontier[index].amount; i++){
				uint type = frontier[index].bucket[j]->symbol->type;
				if ((type == MEM_STACK_TYPE) || (type == MEM_HEAP_TYPE)){
					
					uint start = frontier[index].bucket[j]->symbol->value;
					uint width = frontier[index].bucket[j]->symbol->width;

					/*check whether this memory is fully contained within the current memory operand*/
					if ((start > opnd->value) && (start + width < opnd->value + opnd->width)){
						nodes.push_back(frontier[index].bucket[j]);
					}

				}
			}
			
		}
	}
}


void Expression_tree::get_partial_overlap_nodes(vector<Node *> &nodes, operand_t * opnd){

	if (opnd->type == REG_TYPE){
		
		int wide_reg = opnd->value;
		uint reg = opnd->value;

		if (reg <= DR_REG_BX){
			/*here in the enum the offset is 16*/
			wide_reg += 16;
			while (wide_reg <= DR_REG_BL){
				Node * node = search_reg(wide_reg);
				if (node != NULL) nodes.push_back(node);
				wide_reg += 16;
			}
			/*we have to also cater for AH to DH*/
			wide_reg -= 16;
			if ((wide_reg <= DR_REG_BL) && (wide_reg >= DR_REG_AL)){
				wide_reg += 4;
				Node * node = search_reg(wide_reg);
				if (node != NULL) nodes.push_back(node);
			}
		}
		else if ((reg >= DR_REG_R8W && reg <= DR_REG_R15W)){
			wide_reg += 16;
			Node * node = search_reg(wide_reg);
			if (node != NULL) nodes.push_back(node);
		}
		else if (reg <= DR_REG_DI){
			wide_reg -= 28;
			Node * node = search_reg(wide_reg);
			if (node != NULL) nodes.push_back(node);

		}
		/* let's handle vector code as well - but not YMM */
		else if ((reg >= DR_REG_XMM0) && (reg <= DR_REG_XMM7)){
			/* here the offset is + */
			wide_reg -= 8;
			Node * node = search_reg(wide_reg);
			if (node != NULL) nodes.push_back(node);
		}
	}
	else if ((opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE)){
		for (int i = 0; i < mem_in_frontier.size(); i++){
			uint index = mem_in_frontier[i];
			for (int j = 0; j < frontier[index].amount; i++){
				uint type = frontier[index].bucket[j]->symbol->type;
				if ((type == MEM_STACK_TYPE) || (type == MEM_HEAP_TYPE)){

					uint start = frontier[index].bucket[j]->symbol->value;
					uint width = frontier[index].bucket[j]->symbol->width;

					/*check whether this memory is strictly partially contained within the current memory operand*/
					
					
					if (((start >= opnd->value) && (start <= opnd->value - opnd->width - 1)) /* start within */
						&& (start + width > opnd->value + opnd->width))	/*end strictly after*/
					{
						nodes.push_back(frontier[index].bucket[j]);
					}

					else if ((start < opnd->value) /*start strictly before*/
						&& ((start + width - 1 >= opnd->value) && (start + width - 1 <= opnd->value + opnd->width - 1))) /* end within */
					{
						nodes.push_back(frontier[index].bucket[j]);
					}
				}
			}
		}
	}

}



/*helper function for adding dependancies */
void add_dependancy(Node * dst, Node * src,uint operation){

	int src_index = dst->srcs.size();

	dst->srcs.push_back(dst);
	if (dst->operation == -1) dst->operation = operation;
	
	src->prev.push_back(dst);
	src->pos.push_back(src_index);
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

	Node * dst = search_node(&instr->dst);

	vector<Node *> full_overlap_nodes;
	vector<Node *> partial_overlap_nodes;

	/* nodes that contain with in current node - we can delete these by replacing with the current destination node */
	get_full_overlap_nodes(full_overlap_nodes, &instr->dst);

	/*do we have nodes that are contain within the current dest?*/
	if (full_overlap_nodes.size() > 0){
		if (dst == NULL){
			dst = new Node(&instr->dst);
		}
		for (int i = 0; i < full_overlap_nodes.size(); i++){
			add_dependancy(full_overlap_nodes[i], dst, op_full_overlap);
			remove_from_frontier(full_overlap_nodes[i]->symbol);
		}
	}

	get_partial_overlap_nodes(partial_overlap_nodes, &instr->dst);

	/*do we have partial overlaps? 
	* also this is tricky as we have only one operation for node; so need to recreate a seperate node for the partial overlapped ones
	* and add that to frontier
	*/

	if (partial_overlap_nodes.size() > 0){
		if (dst == NULL){
			dst = new Node(&instr->dst);
		}
		for (int i = 0; i < partial_overlap_nodes.size(); i++){

			/* add the dependancy to current node; 
			 * remove the partial node from frontier; 
			 * create a duplicate partial node; 
			 * add that to the frontier
			 * add the depedancy to the newly created partial node
			 */

			add_dependancy(partial_overlap_nodes[i], dst, op_partial_overlap);
			remove_from_frontier(partial_overlap_nodes[i]->symbol);
			
			Node * partial_node = new Node(partial_overlap_nodes[i]->symbol);
			add_to_frontier(generate_hash(partial_node->symbol), partial_node);

			add_dependancy(partial_overlap_nodes[i], partial_node, op_partial_overlap);
		}
	}



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
	remove_from_frontier(&instr->dst);

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
			src = search_node(&instr->srcs[i]);
		}

		//when do we need another node? if node is not present or if the destination matches src ( e.g : i <- i + 1 )
		//we do not need to check for immediates here as src will point to a brand new Node in that case and hence will not enter 
		//the if statement
		if( (src == NULL) || (src == dst) ){  //I think now we can remove the src == dst check -> keeping for sanity purposes (why? because we remove the dst from the frontier)
			src = new Node(&instr->srcs[i]);
			add_node = true;
			DEBUG_PRINT(("node added\n"), 3);
		}


		/* assign operation optimization - space */
		bool assign_opt = false;
		
		if ( (instr->num_srcs == 1) && (instr->operation == op_assign) ){  //this is just an assign then remove the current node and place the new src node -> compiler didn't optimize for this?
			
			uint num_references = dst->prev.size();
			
			for (int i = 0; i < num_references; i++){

				src->prev.push_back(dst->prev[i]);
				src->pos.push_back(dst->pos[i]);
				dst->prev[i]->srcs[dst->pos[i]] = src;
				
				assign_opt = true;  /* this is here to differentitate between the head and the rest of the nodes */
			}

			if (assign_opt)  delete dst; /* we have broken all linkages, so just delete it */

		}

		/* update the tree + backward references */

		if (!assign_opt){

			uint src_index = dst->srcs.size();
			dst->srcs.push_back(src);
			src->prev.push_back(dst);
			src->pos.push_back(src_index);

		}

#ifdef DEBUG
#if DEBUG_LEVEL >= 3
		flatten_to_expression(head,cout);
		cout << endl;
#endif
#endif

		/* update the frontiers - include the sources to the frontier if new nodes created */
		if(add_node) add_to_frontier(hash_src,src);
			
	}

}

Node* Expression_tree::get_head(){
	return head;
}