#include <fstream>
#include "node.h"
#include "defines.h"
#include "print.h"
#include "print_dot.h"
#include <string>

using namespace std;

void print_edges(ofstream &file, Node * node);
void print_nodes(ofstream &file, Node * node, uint no_of_nodes);


string get_edge_string(uint from, uint to){

	return to_string(from) + " -> " + to_string(to) + ";";

}

string get_node_string(Node *node){

	return to_string(node->order_num) + " [label=\"" + operation_to_string(node->operation) + "\\n" + opnd_to_string(node->symbol) + "\"];";

}

void print_to_dotfile(ofstream &file,Node * head, uint no_of_nodes){


	file << "digraph G {" << endl;
	print_nodes(file, head, no_of_nodes);
	print_edges(file, head);
	file << "}" << endl;
}


void print_edges(ofstream &file, Node * node){

	for (int i = 0; i < node->srcs.size(); i++){
		file << get_edge_string(node->order_num, node->srcs[i]->order_num) << endl;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_edges(file, node->srcs[i]);
	}

}



/* DAG traversal */
void print_nodes_recursive(ofstream &file, Node * node, uint * done,uint max_nodes){

	ASSERT_MSG((node->order_num < max_nodes), ("ERROR: the node number exceeds the maximum number of nodes\n"));

	if (done[node->order_num] == 0){
		/* print the node */
		file << get_node_string(node) << endl;

		done[node->order_num] = 1;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_nodes_recursive(file, node->srcs[i], done, max_nodes);
	}

}

void print_nodes(ofstream &file, Node * node,uint no_of_nodes){
	
	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	/* can write this as a recursive or an iterative procedure? do an DFS
	 * assert that the node numbers cannot exceed the no_of_nodes
	*/

	print_nodes_recursive(file, node, done, no_of_nodes);
	

	delete[] done;
}



