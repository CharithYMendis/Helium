 #ifndef _NODE_H
 #define _NODE_H
 
#include  "..\..\..\include\output.h"

#define NODE_RIGHT	1
#define NODE_LEFT	2
#define NODE_NONE   3
 
 class Node{
	public:
		int operation;
		Node * left;
		Node * right;
		Node * prev;
		operand_t * symbol;
		uint lr;

		
	public:
		
		Node(operand_t * symbol);
		Node();
		~Node();
		
		Node * get_left();
		Node * get_right();
		int get_operation();
		operand_t * get_symbol();
		
		void set_left(Node * left);
		void set_right(Node * right);
		void set_operation(int operation);
		void set_symbol(operand_t * symbol);

};

#endif