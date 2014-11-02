 #ifndef _NODE_H
 #define _NODE_H
 
#include  "..\..\..\dr_clients\include\output.h"
#include <vector>
#include <stdint.h>

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
		vector< pair<bool, Node *> > conditional_nodes;

		/*keep the backward references - use a container that can grow */
		vector<Node *> prev;
		vector<uint> pos;

		/*forward references also srcs of the destination */
		vector<Node *> srcs;

		/*variables for debugging*/
		uint32_t pc;
		uint32_t line;

		/*auxiliary variables*/
		int order_num;
		int para_num; 
		bool is_para;

		
	public:
		
		Node(operand_t * symbol);
		Node(uint type, uint64 value, uint width, float float_value);
		Node();
		~Node();
	
};






#endif