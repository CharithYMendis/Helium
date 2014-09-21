#include "expression_tree.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include "defines.h"
#include "print_common.h"



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


Node * Expression_tree::search_node(operand_t * opnd){

	uint hash = generate_hash(opnd);

	uint64 value = opnd->value;
	uint width = opnd->width;
	for (int i = 0; i < frontier[hash].amount; i++){
		//we don't need to check for types as we seperate them out in hashing
		//could further optimize this search by having a type specific search algo.
		if ((frontier[hash].bucket[i]->symbol->value == value) && (frontier[hash].bucket[i]->symbol->width == width)){
			return frontier[hash].bucket[i];
		}
	}


	return NULL;

}

void Expression_tree::remove_from_frontier(operand_t * opnd){


	ASSERT_MSG(((opnd->type != IMM_INT_TYPE) && (opnd->type != IMM_FLOAT_TYPE)), ("ERROR: immediate types cannot be in the frontier\n"));

	uint hash = generate_hash(opnd);

	int amount = frontier[hash].amount;
	bool move = false;
	
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

		ASSERT_MSG((frontier[hash].amount > 0), ("ERROR: at least one element should have been deleted\n"));

		int amount = --frontier[hash].amount;
		/* update memoization structure for memory operands */
		if (amount == 0 && (opnd->type != REG_TYPE) ){
			for (int i = 0; i < mem_in_frontier.size(); i++){
				if (mem_in_frontier[i] == hash){
					mem_in_frontier.erase(mem_in_frontier.begin() + i);
					break;
				}
			}
		}
			
	}

	
}


void Expression_tree::add_to_frontier(int hash, Node * node){

	ASSERT_MSG(((node->symbol->type != IMM_INT_TYPE) && (node->symbol->type != IMM_FLOAT_TYPE)), ("ERROR: immediate types cannot be in the frontier\n"));

	ASSERT_MSG((frontier[hash].amount <= SIZE_PER_FRONTIER),("ERROR: bucket size is full\n"));
	frontier[hash].bucket[frontier[hash].amount++] = node;

	/*if this a memory operand we should memoize it*/
	if (node->symbol->type != REG_TYPE){
		vector<uint>::iterator it;
		it = find(mem_in_frontier.begin(), mem_in_frontier.end(), hash);
		if (it == mem_in_frontier.end()){
			mem_in_frontier.push_back(hash);
		}
	}

}


void Expression_tree::get_full_overlap_nodes(vector<Node *> &nodes, operand_t * opnd){

	ASSERT_MSG(((opnd->type != IMM_INT_TYPE) && (opnd->type != IMM_FLOAT_TYPE)), ("ERROR: immediate types cannot be in the frontier\n"));

	DEBUG_PRINT(("checking for full overlap nodes...\n"), 5);

	if (opnd->type == REG_TYPE){
		uint hash = generate_hash(opnd);
		for (int i = 0; i < frontier[hash].amount; i++){

			uint start = frontier[hash].bucket[i]->symbol->value;
			uint width = frontier[hash].bucket[i]->symbol->width;

			if (((start > opnd->value) && (start + width <= opnd->value + opnd->width))  ||
				((start >= opnd->value) && (start + width < opnd->value + opnd->width))){
				DEBUG_PRINT(("reg full overlap found\n"), 5);
				nodes.push_back(frontier[hash].bucket[i]);
			}

		}
	}
	else if ( (opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE) ){

		if (debug_level > 5){
			for (int i = 0; i < mem_in_frontier.size(); i++){
				printf("%d-", mem_in_frontier[i]);
			}
			printf("\n");
		}

		for (int i = 0; i < mem_in_frontier.size(); i++){
			uint index = mem_in_frontier[i];
			for (int j = 0; j < frontier[index].amount; j++){
				uint type = frontier[index].bucket[j]->symbol->type;
				if ((type == MEM_STACK_TYPE) || (type == MEM_HEAP_TYPE)){
					
					uint start = frontier[index].bucket[j]->symbol->value;
					uint width = frontier[index].bucket[j]->symbol->width;

					/*check whether this memory is fully contained within the current memory operand*/
					if (((start > opnd->value) && (start + width <= opnd->value + opnd->width)) ||
						((start >= opnd->value) && (start + width < opnd->value + opnd->width))){
						DEBUG_PRINT(("reg full overlap found\n"), 5);
						nodes.push_back(frontier[index].bucket[j]);
					}

				}
			}
			
		}
	}
}

Node * Expression_tree::create_or_get_node(operand_t * opnd){

	Node * node = search_node(opnd);
	if (node == NULL){
		node = new Node(opnd);
	}
	return node;

}

void Expression_tree::split_partial_overlaps(vector<pair<Node *,vector<Node *> > > &nodes, operand_t * opnd,uint hash){

	for (int i = 0; i < frontier[hash].amount; i++){

		Node * split_node = frontier[hash].bucket[i];
		uint start = frontier[hash].bucket[i]->symbol->value;
		uint width = frontier[hash].bucket[i]->symbol->width;

		vector<Node *> splits;

		if (opnd->type == split_node->symbol->type){

			if (((start >= opnd->value) && (start <= opnd->value - opnd->width - 1)) /* start within */
				&& (start + width > opnd->value + opnd->width))	/*end strictly after*/
			{
				operand_t * first = new operand_t;
				operand_t * second = new operand_t;
				*first = { split_node->symbol->type, opnd->value + width - start, { start } };
				*second = { split_node->symbol->type, width - first->width, { opnd->value + width } };

				splits.push_back(create_or_get_node(first));
				splits.push_back(create_or_get_node(second));
				nodes.push_back(make_pair(split_node, splits));

				DEBUG_PRINT(("partial - %s %s\n", opnd_to_string(first), opnd_to_string(second)), 5);
			}

			else if ((start < opnd->value) /*start strictly before*/
				&& ((start + width - 1 >= opnd->value) && (start + width - 1 <= opnd->value + opnd->width - 1))) /* end within */
			{
				operand_t * first = new operand_t;
				operand_t * second = new operand_t;
				*first = { split_node->symbol->type, opnd->value - start, { start } };
				*second = { split_node->symbol->type, width - first->width, { opnd->value } };

				splits.push_back(create_or_get_node(first));
				splits.push_back(create_or_get_node(second));
				nodes.push_back(make_pair(split_node, splits));

				DEBUG_PRINT(("partial - %s %s\n", opnd_to_string(first), opnd_to_string(second)), 5);
			}

			else if ((start < opnd->value) && (start + width > opnd->value + opnd->width)) /* strictly within start and end */ {

				operand_t * first = new operand_t;
				operand_t * second = new operand_t;

				*first = { split_node->symbol->type, opnd->value - start, { start } };
				*second = { split_node->symbol->type, width - first->width - opnd->width, { opnd->value + width } };

				splits.push_back(create_or_get_node(first));
				splits.push_back(create_or_get_node(opnd));
				splits.push_back(create_or_get_node(second));
				nodes.push_back(make_pair(split_node, splits));

				DEBUG_PRINT(("partial - %s %s %s\n", opnd_to_string(first), opnd_to_string(opnd), opnd_to_string(second)), 5);

			}
		}
	}
}


void Expression_tree::get_partial_overlap_nodes(vector<pair<Node *, vector<Node *> > > &nodes, operand_t * opnd){

	DEBUG_PRINT(("checking for partial overlap nodes...\n"), 5);
	if (opnd->type == REG_TYPE){
		uint hash = generate_hash(opnd);
		split_partial_overlaps(nodes, opnd, hash);
		
	}
	else if ((opnd->type == MEM_STACK_TYPE) || (opnd->type == MEM_HEAP_TYPE)){
		for (int i = 0; i < mem_in_frontier.size(); i++){
			uint index = mem_in_frontier[i];
			split_partial_overlaps(nodes, opnd, index);
		}
	}

}


/*helper function for adding dependancies */
void add_dependancy(Node * dst, Node * src,uint operation){

	int src_index = dst->srcs.size();

	dst->srcs.push_back(src);
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


	vector<Node *> full_overlap_nodes;
	vector<pair<Node *, vector<Node *> > >  partial_overlap_nodes;

	/*first get the partial overlap nodes - if dest is part of the frontier of a wide region it will be part of the nodes returned*/
	get_partial_overlap_nodes(partial_overlap_nodes, &instr->dst);

	if (partial_overlap_nodes.size() > 0){
		for (int i = 0; i < partial_overlap_nodes.size(); i++){
			Node * node =  partial_overlap_nodes[i].first;
			vector<Node *> overlaps = partial_overlap_nodes[i].second;
			remove_from_frontier(node->symbol);
			for (int j = 0; j < overlaps.size(); j++){
				add_dependancy(node, overlaps[i], op_partial_overlap);
				add_to_frontier(generate_hash(overlaps[i]->symbol), overlaps[i]);
			}
		}
	}

	/*get the destination -> the partial overlap may have created the destination if it was contained with in a wide mem region*/
	int hash_dst = generate_hash(&instr->dst);
	DEBUG_PRINT(("dst_hash : %d, frontier amount : %d\n", hash_dst, frontier[hash_dst].amount), 4);
	Node * dst = search_node(&instr->dst);

	/* now get the full overlap nodes */
	/* nodes that contain with in current node - we can delete these by replacing with the current destination node */
	get_full_overlap_nodes(full_overlap_nodes, &instr->dst);

	/*do we have nodes that are contain within the current dest?*/
	if (full_overlap_nodes.size() > 0){
		if (dst == NULL){
			dst = new Node(&instr->dst);
		}
		for (int i = 0; i < full_overlap_nodes.size(); i++){
			DEBUG_PRINT(("full overlap - %s\n", opnd_to_string(full_overlap_nodes[i]->symbol).c_str()), 4);
			add_dependancy(full_overlap_nodes[i], dst, op_full_overlap);
			remove_from_frontier(full_overlap_nodes[i]->symbol);
		}
	}

	if(dst == NULL){
		DEBUG_PRINT(("not affecting the frontier\n"), 4);
		return;  //this instruction does not affect the slice
	}
	else{
		DEBUG_PRINT(("dst - %s : affecting the frontier\n", opnd_to_string(dst->symbol).c_str()), 4);
		if (debug_level >= 6){
			DEBUG_PRINT(("current expression:\n"), 5);
			print_node_tree(head, cout);
			cout << endl;
		}
	}

	//update operation
	dst->operation = instr->operation;
	DEBUG_PRINT(("operation : %s\n", operation_to_string(dst->operation).c_str()), 4);


	//ok now to remove the destination from the frontiers
	remove_from_frontier(&instr->dst);
	/* assign operation optimization - space */
	bool assign_opt = false;
	//update srcs
	for(int i=0; i<instr->num_srcs; i++){
		
		//first check whether there are existing nodes in the frontier for these sources
		//if there is we know that the same definition of src is used for this, so we can just point to it rather than 
		//creating a new node -> space and time efficient
		int hash_src = generate_hash(&instr->srcs[i]);

		DEBUG_PRINT(("src_hash : %d, frontier amount : %d\n", hash_src, frontier[hash_src].amount), 4);

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
			DEBUG_PRINT(("new node added to the frontier\n"), 4);
		}

		DEBUG_PRINT(("src - %s\n", opnd_to_string(src->symbol).c_str()), 4);


		
		
		if ( (instr->num_srcs == 1) && (instr->operation == op_assign) ){  //this is just an assign then remove the current node and place the new src node -> compiler didn't optimize for this?
			
			uint num_references = dst->prev.size();
			
			for (int i = 0; i < num_references; i++){

				src->prev.push_back(dst->prev[i]);
				src->pos.push_back(dst->pos[i]);
				dst->prev[i]->srcs[dst->pos[i]] = src;
				
				assign_opt = true;  /* this is here to differentitate between the head and the rest of the nodes */
			}

			DEBUG_PRINT(("optimizing assign\n"), 4);

			if (assign_opt)  delete dst; /* we have broken all linkages, so just delete it */


		}

		/* update the tree + backward references */

		if (!assign_opt){

			uint src_index = dst->srcs.size();
			dst->srcs.push_back(src);
			src->prev.push_back(dst);
			src->pos.push_back(src_index);
			

		}

		

		if(debug_level >= 6){
			DEBUG_PRINT(("current expression after adding src\n"), 5);
			print_node_tree(head,cout);
			cout << endl;
		}

		/* update the frontiers - include the sources to the frontier if new nodes created */
		if(add_node) add_to_frontier(hash_src,src);

		DEBUG_PRINT(("completed adding a src\n"), 4);
			
	}

	/*if (!assign_opt){
		canonicalize_node(dst);
	}*/

	

}

Node* Expression_tree::get_head(){
	return head;
}
