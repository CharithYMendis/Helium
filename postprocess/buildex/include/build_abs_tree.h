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


/* type and processing explanation 

abs_nodes types and processing are determined by the Node symbol types and the mem_regions types, therefore, we can see as building the abs tree 
as a unification of the information extracted by the instrace(building the concrete tree) and memdump(locating image locations and properties). Further
abstraction can be performed on this tree.

type(Abs_node) <- unify ( type(Node->symbol) , type(mem_region) )

Next, we assume that the bottom of the tree should be Heap Nodes (image dependancies), Stack or Regs(parameters) or immediate ints, immediate floats

if (Node->symbol) is heap 
 *check for associated memory region -> this should be non null
 *get the x,y,c co-ordinates of the symbol value and store in the struct mem_info
 *mem_info struct
    -> associated mem  - mem_region
	-> dimensions - the dimensionality of the mem_region -> this depends on the image layout
	-> indexes - this carries the final abstracted out index coefficients when we express the filter algebrically
	-> pos - this lifts the flat memory address into coordinates in the dimensional space
  *type will depend on whether the associated mem region is input, output or intermediate

if (Node->symbol is reg or stack)
 *parameter? Need to identify statically
 *operation_only? If not parameter

 if(Node->symbol immed int or float)

 subtree boundary - check for similar subtrees that assign to intermediate nodes

*/


/* Abs tree node */
class Abs_node {

public:

	uint operation;
	
	/* some type information about the node - what type of node is this */
	uint type; 

	/* operand characteristics */
	uint width;
	uint range; /* what is this? */
	bool sign;

	union {
		int value; /* this is for storage of immediate, reg values etc.*/
		float float_value;
		struct {
			mem_regions_t * associated_mem;
			uint dimensions;
			int ** indexes; /* 2-dimensional array */
			int * pos;
		} mem_info;
	};

	/* linkages to other nodes */
	vector<Abs_node *> srcs;
	Abs_node * prev;

	/*node number*/
	uint order_num;

	/*conditional or not*/
	jump_info_t * jump;

	Abs_node();

};

/* compound node */
class Comp_Abs_node{

public:
	vector<Abs_node *> nodes;
	vector<Comp_Abs_node *> srcs;
	Comp_Abs_node * prev;

	/*node number*/
	uint order_num;

	Comp_Abs_node();

};

/* Abs tree */
class Abs_tree{

public:

	Abs_node * head;

	Abs_tree();
	
	/* all are static methods actually */
	void build_abs_tree(Abs_node * abs_node, Node * node, vector<mem_regions_t *> &mem_regions);
	void seperate_intermediate_buffers(Abs_node * node);
	static bool are_abs_trees_similar(vector<Abs_node *> abs_nodes);

	


};

/* compound abs tree */
class Comp_Abs_tree{

public:
	Comp_Abs_node * head;

	Comp_Abs_tree();

	/* all are static methods actually */
	void build_compound_tree(Comp_Abs_node * comp_node, vector<Abs_node *> abs_nodes);
	static void abstract_buffer_indexes(Comp_Abs_node * comp_node);
	Abs_node * compound_to_abs_tree();

private:

	static void abstract_buffer_indexes(Comp_Abs_node * head, Comp_Abs_node * node);

};


#endif

