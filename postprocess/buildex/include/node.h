 #ifndef _NODE_H
 #define _NODE_H
 
#include  "..\..\..\include\output.h"
#include <vector>

#define NODE_RIGHT	1
#define NODE_LEFT	2
#define NODE_NONE   3

#define MAX_REFERENCES 10

using namespace std;
 
 class Node{
	public:

		int operation;
		Node * left;
		Node * right;
		uint num_references;
		operand_t * symbol;

		//keep the backward references - use a container that can grow
		vector<Node *> prev;
		vector<uint> lr;

		
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