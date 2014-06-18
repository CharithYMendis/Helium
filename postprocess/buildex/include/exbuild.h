#ifndef _EXBUILD_H
#define _EXBUILD_H

#include <stdio.h>
#include "node.h"
#include "canonicalize.h"
#include  "..\..\..\include\output.h"
#include <fstream>

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


	public:
		Expression_tree();
		~Expression_tree();
		
		void update_frontier(rinstr_t * instr);
		void flatten_to_expression(std::ostream &file);

private:
	int generate_hash(operand_t * opnd);
	Node * search_node(int hash, int value);
	void remove_from_frontier(int hash, int value);
	void add_to_frontier(int hash, Node * node);

	//printing routines
	void print_operation(int operation,std::ostream &file);
	void print_operand(operand_t * opnd,std::ostream &file);
	void print_tree(Node * node, std::ostream & file);



};




#endif