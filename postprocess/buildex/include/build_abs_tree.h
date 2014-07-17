#ifndef _ABS_TREE_H
#define _ABS_TREE_H


#include "defines.h"
#include "expression_tree.h"
#include "node.h"
#include "tree_transformations.h"
#include "build_mem_instrace.h"
#include "build_mem_dump.h"
#include <vector>



/* types */
#define OPERATION_ONLY		0
#define INPUT_NODE			1
#define OUTPUT_NODE			2
#define INTERMEDIATE_NODE	3
#define IMMEDIATE_INT		4
#define IMMEDIATE_FLOAT		5
#define PARAMETER			6
#define UNRESOLVED_SYMBOL	7
#define SUBTREE_BOUNDARY	8


/* Abs tree node */
class Abs_node {

public:

	uint operation;
	
	/* some type information about the node */
	uint type; 
	uint width;
	bool sign;

	union {
		int value;
		float float_value;
		struct {
			mem_regions_t * associated_mem;
			uint dimensions;
			uint ** indexes; /* 2-dimensional array */
			uint * pos;
		} mem_info;
	};

	/* linkages to other nodes */
	vector<Abs_node *> srcs;
	Abs_node * prev;


};

/* compound node */
class Comp_Abs_node{

public:
	vector<Abs_node *> nodes;
	vector<Comp_Abs_node *> srcs;
	Comp_Abs_node * prev;

};

/* Abs tree */
class Abs_tree{

public:

	Abs_node * head;

	Abs_tree();
	
	/* all are static methods actually */
	void build_abs_tree(Abs_node * abs_node, Node * node, vector<mem_regions_t *> &mem_regions);
	void seperate_intermediate_buffers(Abs_node * node);
	bool are_abs_trees_similar(vector<Abs_node *> abs_nodes);

	


};

/* compound abs tree */
class Comp_Abs_tree{

	Comp_Abs_node * head;

	Comp_Abs_tree();

	/* all are static methods actually */
	void build_compound_tree(Comp_Abs_node * comp_node, vector<Abs_node *> abs_nodes);
	void abstract_buffer_indexes(Comp_Abs_node * comp_node);
	Abs_node * compound_to_abs_tree();

private:

	void abstract_buffer_indexes(Comp_Abs_node * head, Comp_Abs_node * node);

};


#endif

