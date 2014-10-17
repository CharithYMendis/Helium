#ifndef _EXBUILD_H
#define _EXBUILD_H

#include <stdio.h>
#include "node.h"
#include "canonicalize.h"
#include  "..\..\..\dr_clients\include\output.h"
#include <fstream>

#define MAX_FRONTIERS		1000
#define SIZE_PER_FRONTIER	10
#define MEM_OFFSET			200
#define MEM_REGION			(MAX_FRONTIERS - MEM_OFFSET)
#define REG_REGION			MEM_OFFSET

//frontier type with book keeping
struct frontier_t {

	Node ** bucket;
	int amount;

} ;


//this class has builds up the expression
class Expression_tree {

	private:
		Node * head;
		frontier_t * frontier;  //this is actually a hash list keeping pointers to the Nodes already allocated

		/* memoization structures for partial mem and reg writes and reads */
		vector<uint> mem_in_frontier;

		/* opt: can have nodes for immediates? */


	public:
		Expression_tree();
		~Expression_tree();
		
		bool update_frontier(rinstr_t * instr, uint32_t pc, std::string disasm, uint32_t line);
		Node * get_head();
		

private:

	int generate_hash(operand_t * opnd);
	Node * search_node(operand_t * opnd);
	Node * create_or_get_node(operand_t * opnd);
	void remove_from_frontier(operand_t * opnd);
	void add_to_frontier(int hash, Node * node);

	void get_full_overlap_nodes(vector<Node *> &nodes, operand_t * opnd);
	void split_partial_overlaps(vector < pair <Node *, vector<Node *> > > &nodes, operand_t * opnd, uint hash);
	void get_partial_overlap_nodes(vector<pair<Node *, vector<Node *> > > &nodes, operand_t * opnd);

	void print_tree_internal(Node * node, std::ostream &file);

};




#endif