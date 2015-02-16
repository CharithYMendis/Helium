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

