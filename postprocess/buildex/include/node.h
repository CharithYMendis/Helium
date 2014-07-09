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

		/* the main identification material for the node */
		int operation;
		operand_t * symbol;

		/* to accommodate for partial registers we will use the upper 32 bits of the value field in operand_t
		   it will signify the start point of the register
		*/

		/*keep the backward references - use a container that can grow */
		vector<Node *> prev;
		vector<uint> pos;

		/*forward references also srcs of the destination */
		vector<Node *> srcs;
		int order_num;


		
	public:
		
		Node(operand_t * symbol);
		Node(uint type, uint value, uint width, float float_value);
		Node();
		~Node();
	

};

#endif