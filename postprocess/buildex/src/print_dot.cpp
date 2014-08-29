#include <fstream>
#include "node.h"
#include "defines.h"
#include "print_common.h"
#include "print_dot.h"
#include "print_halide.h"
#include <string>

using namespace std;

/*  order numbering for printing out */
static uint num = 0;

/* node */

string get_edge_string(uint from, uint to){

	return to_string(from) + " -> " + to_string(to) + ";";

}

uint number_tree_internal(Node * node){

	if (node->order_num == -1){
		node->order_num = num;
		num++;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		number_tree_internal(node->srcs[i]);
	}

	return num;

}

uint number_tree_nodes(Node * node){

	num = 0;
	return number_tree_internal (node);

}

string get_node_string(Node *node){

	return to_string(node->order_num) + " [label=\"" + operation_to_string(node->operation) + "\\n" + opnd_to_string(node->symbol) + "\"];";

}

void print_edges_recursive(ofstream &file, Node * node, uint * done, uint max_nodes){

	ASSERT_MSG((node->order_num < max_nodes), ("ERROR: the node number exceeds the maximum number of nodes\n"));

	if (done[node->order_num] == 1) return;

	done[node->order_num] = 1;

	for (int i = 0; i < node->srcs.size(); i++){
		file << get_edge_string(node->order_num, node->srcs[i]->order_num) << endl;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_edges_recursive(file, node->srcs[i], done, max_nodes);
	}

}

void print_edges(ofstream &file, Node * node, uint no_of_nodes){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	print_edges_recursive(file, node, done, no_of_nodes);

	delete[] done;

}

void print_nodes_recursive(ofstream &file, Node * node, uint * done, uint max_nodes){

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

void print_nodes(ofstream &file, Node * node, uint no_of_nodes){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	/* can write this as a recursive or an iterative procedure? do an DFS
	* assert that the node numbers cannot exceed the no_of_nodes
	*/

	print_nodes_recursive(file, node, done, no_of_nodes);


	delete[] done;
}

void print_to_dotfile(ofstream &file,Node * head, uint no_of_nodes, uint graph_no){


	file << "digraph G_" << graph_no << " {" << endl;
	DEBUG_PRINT( ("printing node...\n"), 2);
	print_nodes(file, head, no_of_nodes);
	DEBUG_PRINT(("printing edges...\n"), 2);
	print_edges(file, head, no_of_nodes);
	file << "}" << endl;
}


/* abs tree */

uint number_tree_internal(Abs_node * node){

	if (node->order_num == -1){
		node->order_num = num;
		num++;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		number_tree_internal(node->srcs[i]);
	}

	return num;

}

uint number_tree_nodes(Abs_node * node){

	num = 0;
	return number_tree_internal(node);

}

string get_node_string(Abs_node *node, bool abs){

	if (!abs){
		return to_string(node->order_num) + " [label=\"" + abs_node_to_string(node) + "\"];";
	}
	else{
		return to_string(node->order_num) + " [label=\"" + get_abs_node_string(node) + "\"];";
	}

}

void print_edges_recursive(ofstream &file, Abs_node * node, uint * done, uint max_nodes){

	ASSERT_MSG((node->order_num < max_nodes), ("ERROR: the node number exceeds the maximum number of nodes\n"));

	if (done[node->order_num] == 1) return;

	done[node->order_num] = 1;

	for (int i = 0; i < node->srcs.size(); i++){
		file << get_edge_string(node->order_num, node->srcs[i]->order_num) << endl;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_edges_recursive(file, node->srcs[i], done, max_nodes);
	}

}

void print_edges(ofstream &file, Abs_node * node, uint no_of_nodes){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	print_edges_recursive(file, node, done, no_of_nodes);

	delete[] done;

}

void print_nodes_recursive(ofstream &file, Abs_node * node, uint * done, uint max_nodes, bool abs){

	ASSERT_MSG((node->order_num < max_nodes), ("ERROR: the node number exceeds the maximum number of nodes\n"));

	if (done[node->order_num] == 0){
		/* print the node */
		file << get_node_string(node,abs) << endl;

		done[node->order_num] = 1;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_nodes_recursive(file, node->srcs[i], done, max_nodes,abs);
	}

}

void print_nodes(ofstream &file, Abs_node * node, uint no_of_nodes, bool abs){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	/* for the first node set indexes to zero -> this will make it print correctly and finally restore */
	uint ** indexes = new uint *[node->mem_info.dimensions];
	for (int i = 0; i < node->mem_info.dimensions; i++){
		indexes[i] = new uint[node->mem_info.dimensions + 1];
	}

	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions + 1; j++){
			indexes[i][j] = node->mem_info.indexes[i][j];
			if (i == j){
				node->mem_info.indexes[i][j] = 1;
			}
			else{
				node->mem_info.indexes[i][j] = 0;
			}
			
		}
	}

	print_nodes_recursive(file, node, done, no_of_nodes,abs);

	/* restore back the indexes */
	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions + 1; j++){
			node->mem_info.indexes[i][j] = indexes[i][j];
		}
	}

	/* clean up */
	delete[] done;
	for (int i = 0; i < node->mem_info.dimensions; i++){
		delete[] indexes[i];
	}
	delete indexes;
}

void print_to_dotfile(ofstream &file, Abs_node * head, uint no_of_nodes, uint graph_no, bool abs){

	file << "digraph G_abs_" << graph_no << " {" << endl;
	print_nodes(file, head, no_of_nodes,abs);
	print_edges(file, head, no_of_nodes);
	file << "}" << endl;
}



/* comp tree */

uint number_tree_internal(Comp_Abs_node * node){

	if (node->order_num == -1){
		node->order_num = num;
		num++;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		number_tree_internal(node->srcs[i]);
	}

	return num;

}

uint number_tree_nodes(Comp_Abs_node * node){

	num = 0;
	return number_tree_internal(node);

}

string get_node_string(Comp_Abs_node *node){

	string ret = to_string(node->order_num) + "[label = \"";

	for (int i = 0; i < node->nodes.size(); i++){
		Abs_node * instance = node->nodes[i];
		if (instance->type == INPUT_NODE || instance->type == OUTPUT_NODE || instance->type == INTERMEDIATE_NODE){
			ret += abs_node_to_string(instance) + "\\n";
		}
		else{
			ret += abs_node_to_string(instance);
			break;
		}
	}

	return ret + "\"];";

}

void print_edges_recursive(ofstream &file, Comp_Abs_node * node, uint * done, uint max_nodes){

	ASSERT_MSG((node->order_num < max_nodes), ("ERROR: the node number exceeds the maximum number of nodes\n"));

	if (done[node->order_num] == 1) return;

	done[node->order_num] = 1;

	for (int i = 0; i < node->srcs.size(); i++){
		file << get_edge_string(node->order_num, node->srcs[i]->order_num) << endl;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_edges_recursive(file, node->srcs[i], done, max_nodes);
	}

}

void print_edges(ofstream &file, Comp_Abs_node * node, uint no_of_nodes){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	print_edges_recursive(file, node, done, no_of_nodes);

	delete[] done;

}

void print_nodes_recursive(ofstream &file, Comp_Abs_node * node, uint * done, uint max_nodes){

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

void print_nodes(ofstream &file, Comp_Abs_node * node, uint no_of_nodes){

	uint * done = new uint[no_of_nodes];
	memset(done, 0, sizeof(uint)*no_of_nodes);

	/* can write this as a recursive or an iterative procedure? do an DFS
	* assert that the node numbers cannot exceed the no_of_nodes
	*/

	print_nodes_recursive(file, node, done, no_of_nodes);


	delete[] done;
}

void print_to_dotfile(ofstream &file, Comp_Abs_node * head, uint no_of_nodes, uint graph_no){

	file << "digraph G_abs_" << graph_no << " {" << endl;
	print_nodes(file, head, no_of_nodes);
	print_edges(file, head, no_of_nodes);
	file << "}" << endl;
}


