#include <assert.h>
#include <fstream>
#include <iostream>

#include "utility\defines.h"
#include "analysis\x86_analysis.h"
#include "trees\trees.h"
#include "utility\print_helper.h"


Abs_Tree::Abs_Tree() : Tree()
{

}

Abs_Tree::~Abs_Tree()
{

}

Node * abs_node_from_conc_node(void * head, void * node, void * peripheral_data){

	vector<mem_regions_t *> regions = *(vector<mem_regions_t *> *)peripheral_data;
	Abs_Node * new_node = new Abs_Node(static_cast<Conc_Node *>(head),static_cast<Conc_Node *>(node), regions);
	return new_node;

}

void Abs_Tree::build_abs_tree_unrolled(Conc_Tree * tree, std::vector<mem_regions_t *> &mem_regions)
{
	this->recursive = tree->recursive;
	this->dummy_tree = tree->dummy_tree;
	copy_unrolled_tree_structure(tree, &mem_regions, abs_node_from_conc_node);
}

void Abs_Tree::build_abs_tree_exact(Conc_Tree * tree, std::vector<mem_regions_t *> &mem_regions){

	this->recursive = tree->recursive;
	this->dummy_tree = tree->dummy_tree;
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

	bool inserted = false;

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
		}

		return NULL;

	}, empty_ret_mutator);

	return nodes;

}

vector<Abs_Node *> Abs_Tree::retrieve_parameters(){

	vector<Abs_Node *> nodes;
	traverse_tree(get_head(), &nodes,
		[](Node * node, void * value)->void*{

		Abs_Node * abs_node = static_cast<Abs_Node *>(node);
		if (abs_node->type == Abs_Node::PARAMETER){
			vector<Abs_Node *> *nodes = (vector<Abs_Node *> *)value;
			for (int i = 0; i < (*nodes).size(); i++){
				if ((*nodes)[i]->para_num
					== abs_node->para_num){
					return NULL;
				}
			}
			(*nodes).push_back(abs_node);
		}

		return NULL;

	}, empty_ret_mutator);

	return nodes;
}

vector<Abs_Node *> Abs_Tree::get_buffer_region_nodes(){

	vector<Abs_Node *> nodes;

	traverse_tree(head, &nodes, 
		[](Node * node, void * value)->void*{
		
		Abs_Node * abs_node = (Abs_Node *)node;
		if (abs_node->type == Abs_Node::INPUT_NODE || abs_node->type == Abs_Node::OUTPUT_NODE || abs_node->type == Abs_Node::INTERMEDIATE_NODE){
			vector<Abs_Node *> * nodes = (vector<Abs_Node *> *)value;
			for (int i = 1; i < (*nodes).size(); i++){ /* trick to get rid of the head node */
				if (abs_node->mem_info.associated_mem == (*nodes)[i]->mem_info.associated_mem){
					return NULL;
				}
			}
			(*nodes).push_back(abs_node);
		}

		return NULL;

	}, empty_ret_mutator);

	nodes.erase(nodes.begin()); /* remove head node */

	return nodes;
}


uint32_t Abs_Tree::get_maximum_dimensions(){


	uint32_t ret = 0;

	traverse_tree(head, &ret, 
		[](Node * node, void * value)->void*{
		uint32_t * max_dim = static_cast<uint32_t *>(value);
		Abs_Node * abs_node = static_cast<Abs_Node *>(node);

		if (abs_node->type == Abs_Node::INPUT_NODE || abs_node->type == Abs_Node::OUTPUT_NODE || abs_node->type == Abs_Node::INTERMEDIATE_NODE){
			if (abs_node->mem_info.dimensions > *max_dim) *max_dim = abs_node->mem_info.dimensions;
		}

		return NULL;


	}, empty_ret_mutator);

	return ret;

}


void Abs_Tree::print_dot_algebraic(std::ostream &file, std::string name, uint32_t number, std::vector<std::string> vars){


	/* make sure the head node indexes are zeroed */
	Abs_Node * node = static_cast<Abs_Node *>(head);
	
	uint ** indexes = new uint32_t *[node->mem_info.dimensions];
	for (int i = 0; i < node->mem_info.dimensions; i++){
		indexes[i] = new uint32_t[node->mem_info.head_dimensions + 1];
	}

	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.head_dimensions + 1; j++){
			indexes[i][j] = node->mem_info.indexes[i][j];
			if (i == j){
				node->mem_info.indexes[i][j] = 1;
			}
			else{
				node->mem_info.indexes[i][j] = 0;
			}
		}
	}


	ASSERT_MSG((vars.size() > 0), ("ERROR: please use the non-var version for printing\n"));

	/* print the nodes */
	string nodes = "";
	vars.insert(vars.begin(), nodes);

	file << "digraph G_" << name << "_" << number << " {" << endl;

	cleanup_visit();

	traverse_tree(head, &vars,
		[](Node * node, void * value)->void* {


		if (node->visited == false){

			vector<string> * vec_string = static_cast<vector<string> *>(value);
			Abs_Node * abs_node = static_cast<Abs_Node *>(node);
			vector<string> vec_vars((*vec_string).begin() + 1, (*vec_string).end());

			(*vec_string)[0] += dot_get_node_string(abs_node->order_num, abs_node->get_dot_string(vec_vars)) + "\n";
			node->visited = true;
		}

		return NULL;

	}, empty_ret_mutator);


	nodes = vars[0];

	file << nodes << endl;

	cleanup_visit();

	/* print the edges */
	string edges = "";

	traverse_tree(head, &edges,
		[](Node * node, void * value)->void* {


		if (node->visited == false){
			string * edge_string = static_cast<string *>(value);
			for (int i = 0; i < node->srcs.size(); i++){
				*edge_string += dot_get_edge_string(node->order_num, node->srcs[i]->order_num) + "\n";
			}
			node->visited = true;

		}

		return NULL;

	}, empty_ret_mutator);

	cleanup_visit();

	file << edges << endl;

	file << "}" << endl;

	/* restore back the indexes */
	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions + 1; j++){
			node->mem_info.indexes[i][j] = indexes[i][j];
		}
	}

	/* clean up */
	for (int i = 0; i < node->mem_info.dimensions; i++){
		delete[] indexes[i];
	}
	delete indexes;



}



Abs_Node * Abs_Tree::find_indirect_node(Abs_Node * node){

	if (node->srcs.size() == 0){
		if (node->mem_info.associated_mem != NULL){
			return node;
		}
		else{
			return NULL;
		}
	}

	Abs_Node * ret;

	for (int i = 0; i < node->srcs.size(); i++){
		ret = find_indirect_node((Abs_Node *)node->srcs[i]);
		if (ret != NULL) break;
	}


	return ret;

}


