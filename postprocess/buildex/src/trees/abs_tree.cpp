#include <assert.h>
#include <fstream>
#include <iostream>

#include "defines.h"
#include "analysis\x86_analysis.h"
#include "trees\trees.h"


Abs_Tree::Abs_Tree() : Tree()
{

}

Abs_Tree::~Abs_Tree()
{

}

Node * abs_node_from_conc_node(void * node, void * peripheral_data){

	vector<mem_regions_t *> regions = *(vector<mem_regions_t *> *)peripheral_data;
	Abs_Node * new_node = new Abs_Node(static_cast<Conc_Node *>(node), regions);
	return new_node;

}

void Abs_Tree::build_abs_tree_unrolled(Conc_Tree * tree, std::vector<mem_regions_t *> &mem_regions)
{
	copy_unrolled_tree_structure(tree, &mem_regions, abs_node_from_conc_node);
}

void Abs_Tree::build_abs_tree_exact(Conc_Tree * tree, std::vector<mem_regions_t *> &mem_regions){

	copy_exact_tree_structure(tree, &mem_regions, abs_node_from_conc_node);

}

void Abs_Tree::seperate_intermediate_buffers()
{
	throw "not implemented!";
}

std::string Abs_Tree::serialize_tree()
{
	throw "not implemented!";
}

void Abs_Tree::construct_tree(std::string stree)
{
	throw "not implemented!";
}

bool check_node_repitition(vector<Abs_Node *> &stack, Abs_Node* node){

	ASSERT_MSG(((node->type == Abs_Node::OUTPUT_NODE) || (node->type == Abs_Node::INTERMEDIATE_NODE)), ("ERROR: the node should be an intermediate or an output buffer\n"));

	for (int i = 0; i < stack.size(); i++)
	{
		ASSERT_MSG((stack[i]->type != Abs_Node::INPUT_NODE), ("ERROR: input nodes should be immediately removed from the stack at the exit\n"));
		if ((node->type == stack[i]->type) && (node->mem_info.dimensions == stack[i]->mem_info.dimensions)){
			bool same = (stack[i]->mem_info.associated_mem == node->mem_info.associated_mem);
			if (same) return true;
		}
	}

	return false;
}

/* will check for recursion at all levels; including intermediate buffers */
bool check_recursive_tree(vector<Abs_Node *> &stack, Abs_Node * node){

	bool inserted = true;

	if (node->type == Abs_Node::OUTPUT_NODE || node->type == Abs_Node::INTERMEDIATE_NODE){
		if (check_node_repitition(stack, node)){
			return true;
		}
		stack.push_back(node);
		inserted = true;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		Abs_Node * src = static_cast<Abs_Node *>(node->srcs[i]);
		bool ret = check_recursive_tree(stack, src);
		if (ret){
			if (inserted) stack.pop_back();
			return true;
		}
	}

	if (inserted) stack.pop_back();

	return false;

}

bool Abs_Tree::is_tree_recursive(){

	vector<Abs_Node *> stack;
	Abs_Node *  node = static_cast<Abs_Node *>(get_head());

	return check_recursive_tree(stack, node);

}

vector<Abs_Node *> Abs_Tree::retrieve_input_nodes(){

	vector<Abs_Node *> nodes;
	traverse_tree(get_head(),&nodes,
		[](Node * node, void * value)->void*{

		Abs_Node * abs_node = static_cast<Abs_Node *>(node);
		if (abs_node->type == Abs_Node::INPUT_NODE){
			vector<Abs_Node *> *nodes = (vector<Abs_Node *> *)value;
			for (int i = 0; i < (*nodes).size(); i++){
				if ((*nodes)[i]->mem_info.associated_mem
					== abs_node->mem_info.associated_mem){
					return NULL;
				}
			}
			(*nodes).push_back(abs_node);
			return NULL;
		}


	}, empty_ret_mutator);

}

