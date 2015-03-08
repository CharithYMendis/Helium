#include <ostream>
#include <fstream>
#include <algorithm>

using namespace std;

#include "trees/nodes.h"
#include "analysis/x86_analysis.h"
#include "common_defines.h"
#include "utility/defines.h"


/* this implements the abstract class - Node */

/* constructors */
Node::Node(){
	order_num = -1;
	visited = false;
}

/* copy constructor */
Node::Node(const Node& node) :
operation(node.operation),
sign(node.sign),
symbol(node.symbol),
pc(node.pc),
is_para(node.is_para),
is_double(node.is_double),
para_num(node.para_num),
visited(false),
order_num(-1)
{

}

Node::~Node(){

}

/* tree transformations */
/* (src -> ref) => (src) */
bool Node::remove_forward_ref_single(Node *ref)
{

	bool erase = false;
	uint32_t erased_index = 0;

	for (int i = 0; i < this->srcs.size(); i++){
		if (this->srcs[i] == ref){
			this->srcs.erase(this->srcs.begin() + i);
			/* update the ref as well */
			for (int j = 0; j < ref->pos.size(); j++){
				if (ref->prev[j] == this && ref->pos[j] == i){
					ref->prev.erase(ref->prev.begin() + j);
					ref->pos.erase(ref->pos.begin() + j);
					j--;
					break;
				}
			}
			erase = true;
			erased_index = i;
			break;
		}
	}

	vector<pair<Node *, uint32_t> > node_counts;

	/* need to update backward references of still connected nodes */
	if (erase){
		for (int i = 0; i < this->srcs.size(); i++){

			uint32_t count = 0;
			for (int j = 0; j < node_counts.size(); j++){
				if (node_counts[j].first == this->srcs[i]){
					count = node_counts[j].second;
					node_counts[j].second++;
					break;
				}
			}
			if (count == 0){ node_counts.push_back(make_pair(this->srcs[i], 1)); }
			int found = 0;

			for (int j = 0; j < this->srcs[i]->prev.size(); j++){

				if (this->srcs[i]->prev[j] == this){
					if (found == count){
						this->srcs[i]->pos[j] = i;
						break;
					}
					else{
						found++;
					}
				}
			}
		}
	}

	for (int i = 0; i < this->srcs.size(); i++){
		Node * src = this->srcs[i];
		bool found = false;
		for (int j = 0; j < src->prev.size(); j++){
			if ((src->prev[j] == this) && (src->pos[j] == i)){
				found = true;
			}
		}
		ASSERT_MSG((found),("ERROR: remove forward ref wrong\n"));
	}

	return erase;
}

uint32_t Node::remove_forward_ref(Node * ref){

	int count = 0;
	while (remove_forward_ref_single(ref)){
		count++;
	}

	return count;

}

/* (ref -> src) => (src) */
bool Node::remove_backward_ref_single(Node *ref)
{
	for (int i = 0; i < this->prev.size(); i++){
		if (this->prev[i] == ref){
			this->prev.erase(this->prev.begin() + i);
			this->pos.erase(this->pos.begin() + i);
			return true;
		}
	}
	return false;
	
}

uint32_t Node::remove_backward_ref(Node * ref){
	
	uint32_t count = 0;
	while (remove_backward_ref_single(ref)){
		count++;
	}
	return count;
}

/* src => (src->ref) */
void Node::add_forward_ref(Node * ref)
{
	this->srcs.push_back(ref);
	ref->prev.push_back(this);
	ref->pos.push_back(this->srcs.size() - 1);
}

/* (dst -> ref) => (dst -> src) */
void Node::change_ref(Node * dst, Node * src){
	
	int index = -1;

	/* In place forward reference and push back back ward reference
	replacing it at the exact same location is important for
	non-associative operations*/

	for (int i = 0; i < dst->srcs.size(); i++){
		if (dst->srcs[i] == this){
			dst->srcs[i] = src;
			dst->srcs[i]->prev.push_back(dst);
			dst->srcs[i]->pos.push_back(i);
			index = i;
		}

	}

	/* dst->remove_forward_ref(this); redundant*/

}

/* (dst -> ref -> src)  => (dst -> src) */
void Node::remove_intermediate_ref(Node * dst, Node *src)
{
	this->change_ref(dst, src);
	this->remove_forward_ref(src);

}

void Node::safely_delete(Node * head)
{
	if ((this->prev.size() == 0) && (this != head)){
		/* remove any backward references to this node if existing ; we are assuming that there can only be one head node for the tree */
		/* no need to remove the forward reference as it is only within the deletable node */
		for (int i = 0; i < this->srcs.size(); i++){
			this->srcs[i]->remove_backward_ref(this);
		}
		delete this;
	}
}

void Node::remove_forward_all_srcs()
{
	for (int i = 0; i < this->srcs.size(); i++){
		Node * src = this->srcs[i];
		this->remove_forward_ref(src);
	}
}


/* node canonicalization routines */

bool is_operation_associative(uint32_t operation){
	switch (operation){
	case op_add:
	case op_mul:
		return true;
	default:
		return false;
	}
}

bool sort_function(pair<Node *, uint32_t> first_node, pair<Node *, uint32_t> second_node){
	return (first_node.first->symbol->value < second_node.first->symbol->value);
}

void Node::congregate_node(Node * head)
{

	/* This removes the current node and lifts its children to the parent node */
	DEBUG_PRINT(("entered canc.node \n"), 4);

	for (int i = 0; i < (int)this->prev.size(); i++){
		//cout << node->prev[i]->operation << " " << node->operation << endl;
		if ((is_operation_associative(this->operation)) && (this->operation == this->prev[i]->operation)){
			DEBUG_PRINT(("canc. opportunity \n"), 4);
			Node * prev_node = this->prev[i];

			uint32_t rem = prev_node->remove_forward_ref(this);

			if (rem){
				for (int j = 0; j < this->srcs.size(); j++){
					prev_node->add_forward_ref(this->srcs[j]);
				}
				i--;
			}

		}
	}

	this->safely_delete(head);
}

void Node::order_node()
{
	vector<pair<Node *, uint32_t> > other;
	vector<pair<Node *, uint32_t> > heap;

	if (this->operation != op_mul && this->operation != op_add) return;

	for (int i = 0; i < this->srcs.size(); i++){

		if (this->srcs[i]->symbol->type == MEM_HEAP_TYPE){
			heap.push_back(make_pair(this->srcs[i], i));
		}
		else{
			other.push_back(make_pair(this->srcs[i], i));
		}

	}

	sort(heap.begin(), heap.end(), sort_function);

	/*first rearrange the other elements*/
	for (int i = 0; i < other.size(); i++){
		this->srcs[i] = other[i].first;
		for (int j = 0; j < other[i].first->prev.size(); j++){
			if (other[i].first->prev[j] == this && other[i].first->pos[j] == other[i].second){
				other[i].first->pos[j] = i;
			}
		}
	}

	/* now for the heap */
	uint32_t beg = other.size();
	for (int i = 0; i < heap.size(); i++){
		this->srcs[beg + i] = heap[i].first;
		for (int j = 0; j < heap[i].first->prev.size(); j++){
			if (heap[i].first->prev[j] == this && heap[i].first->pos[j] == heap[i].second){
				heap[i].first->pos[j] = beg + i;
			}
		}
	}
}

bool Node::are_nodes_similar(std::vector<Node *> nodes){

	if (nodes.size() == 0) return true;

	Node * node = nodes[0];

	for (int i = 1; i < nodes.size(); i++){
		if (!node->are_nodes_similar(nodes[i])){
			return false;
		}
	}

	return true;

}


/* there are more tree transformations -> please look at them */