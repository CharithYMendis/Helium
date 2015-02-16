#include <assert.h>
#include <fstream>
#include <iostream>
#include "defines.h"

#include "trees/nodes.h"

using namespace std;

Comp_Abs_Node::Comp_Abs_Node() : Node(){

}

Comp_Abs_Node::Comp_Abs_Node(Node * node) : Node(*node){

}

Comp_Abs_Node::Comp_Abs_Node(vector<Abs_Node *> abs_nodes){
	nodes = abs_nodes;
}

string Comp_Abs_Node::get_node_string()
{
	string ret = to_string(this->order_num) + "[label = \"";

	for (int i = 0; i < this->nodes.size(); i++){
		Abs_Node * instance = dynamic_cast<Abs_Node*>(this->nodes[i]);

		if (instance->type == Abs_Node::INPUT_NODE || instance->type == Abs_Node::OUTPUT_NODE || instance->type == Abs_Node::INTERMEDIATE_NODE){
			ret += instance->get_node_string() + "\\n";
		}
		else{
			ret += instance->get_node_string();
			break;
		}
	}

	return ret + "\"];";
}

bool Comp_Abs_Node::are_nodes_similar(Node * node){

	Comp_Abs_Node * comp_node = static_cast<Comp_Abs_Node *>(node);
	
	if (comp_node->nodes.size() == 0) return nodes.size() == 0;
	if (nodes.size() == 0) return comp_node->nodes.size() == 0;

	return nodes[0]->are_nodes_similar(comp_node->nodes[0]);


}