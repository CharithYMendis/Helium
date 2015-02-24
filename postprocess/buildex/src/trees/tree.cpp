#include <ostream>
#include <fstream>
#include <algorithm>

using namespace std;

#include "trees/trees.h"
#include "analysis/x86_analysis.h"
#include "common_defines.h"
#include "defines.h"
#include "utility/print_helper.h"


Tree::Tree(){

	head = NULL;
	num_nodes = 0;
	tree_num = -1;

}

void Tree::set_head(Node * node){
	head = node;
}

void Tree::collect_all_nodes(Node * node, std::vector<Node *> &nodes)
{
	if (find(nodes.begin(), nodes.end(), node) == nodes.end()){
		nodes.push_back(node);
	}

	for (int i = 0; i < node->srcs.size(); i++){
		collect_all_nodes(node->srcs[i], nodes);
	}
}

Tree::~Tree(){

	vector<Node *> nodes;
	collect_all_nodes(head, nodes);
	for (int i = 0; i < nodes.size(); i++){
		delete nodes[i];
	}


}

Node * Tree::get_head()
{
	return head;
}


/* pre-order traversal of the tree */
void * Tree::traverse_tree(Node * node, void * value, node_mutator mutator, return_mutator ret_mutator){

	void * node_val = mutator(node, value);
	vector<void *> traverse_val;


	for (int i = 0; i < node->srcs.size(); i++){
		traverse_val.push_back(traverse_tree(node->srcs[i], value, mutator, ret_mutator));
	}

	return ret_mutator(node_val, traverse_val, value); 
} 

void Tree::canonicalize_tree()
{
	/* first congregate the tree and then order the nodes */
	traverse_tree(head,this,
		[](Node * node, void * value)->void* {

		Tree * tree = static_cast<Tree *>(value);
		node->congregate_node(tree->get_head());
		return NULL;

	}, empty_ret_mutator);

	traverse_tree(head, NULL,
		[](Node * node, void * value)->void* {

		node->order_node();
		return NULL;
	}, empty_ret_mutator);


}

void Tree::simplify_tree() /* simplification routines are not written at the moment */
{

}


void Tree::print_dot(std::ostream &file)
{

	/* print the nodes */
	string nodes = "";

	traverse_tree(head, &nodes,
		[](Node * node, void * value)->void* {


		if (node->visited == false){
			string * node_string = static_cast<string *>(value);
			*node_string += dot_get_node_string(node->order_num, node->get_dot_string()) + "\n";
			node->visited = true;
		}

		return NULL;

	}, empty_ret_mutator);


	cleanup_visit();

	/* print the edges */
	string edges = "";

	traverse_tree(head, &edges,
		[](Node * node, void * value)->void* {


		if (node->visited == false){
			string * edge_string = static_cast<string *>(value);
			for (int i = 0; i < node->srcs.size(); i++){
				*edge_string += dot_get_edge_string(node->order_num, node->srcs[i]->order_num) + "\n";
			}
			node->visited = true;
			
		}

		return NULL;

	}, empty_ret_mutator);

	cleanup_visit();

}

void Tree::print_tree_recursive(Node * node, std::ostream &file){

	int no_srcs = node->srcs.size();

	if (no_srcs == 0){ /* we are at a leaf just print it*/
		file << opnd_to_string(node->symbol);
	}
	else if (no_srcs == 1){ /* unary operation */

		/* we can have a full overlap - partial overlaps will have more than one operand */
		if (node->operation == op_full_overlap){
			file << "{" << opnd_to_string(node->symbol) << " -> " << opnd_to_string(node->srcs[0]->symbol) << "}";
			file << "(";
			print_tree_recursive(node->srcs[0], file);
			file << ")";
		}
		else if (node->operation == op_assign){
			print_tree_recursive(node->srcs[0], file);
		}
		else{
			file << operation_to_string(node->operation) << " ";
			print_tree_recursive(node->srcs[0], file);
		}

	}
	else if ((no_srcs == 2) && (node->operation != op_partial_overlap)){
		file << "(";
		print_tree_recursive(node->srcs[0], file);
		file << " " << operation_to_string(node->operation) << " ";
		print_tree_recursive(node->srcs[1], file);
		file << ")";
	}
	else{
		//ASSERT_MSG((node->operation == op_partial_overlap), ("ERROR: unexpected operation with more than two srcs\n"));
		/*here it is important to see how each source contributes
		another important problem is that what source updates first
		*/
		/*file << "(";
		for (int i = 0; i < no_srcs; i++){
		print_tree(node->srcs[0], file);
		if (i != no_srcs - 1){
		file << "," << endl;
		}
		}
		file << ")";*/
		file << "(";
		file << operation_to_string(node->operation) << " ";
		for (int i = 0; i < no_srcs; i++){
			print_tree_recursive(node->srcs[0], file);
			if (i != no_srcs - 1){
				file << ",";
			}
		}
		file << ")";

	}

}

void Tree::print_tree(std::ostream &file)
{
	print_tree_recursive(head, file);

}

void * node_numbering(Node * node, void * value){
	if (node->order_num == -1){
		node->order_num = (*(int *)value)++;
	}
	return NULL;
}

void * empty_ret_mutator(void * value, vector<void *> values, void * ori_value){
	return NULL;
}

void Tree::number_tree_nodes()
{
	traverse_tree(head, &num_nodes, node_numbering, empty_ret_mutator);
}

void Tree::cleanup_visit()
{
	traverse_tree(head, &num_nodes,
		[](Node * node, void * value)->void* {
		node->visited = false;
		return NULL;
		}, empty_ret_mutator);
}

void Tree::copy_exact_tree_structure(Tree * tree, void * peripheral_data, node_to_node node_creation)
{
	/* assumption the tree is numbered and the visited state is cleared */
	ASSERT_MSG((tree->get_head()->order_num != -1), ("ERROR: the concrete tree is not numbered\n"));
	ASSERT_MSG((tree->get_head()->visited == false), ("ERROR: the visit state of the concrete tree is not cleared\n"));


	/* get all the nodes and the nodes to which it is connected */
	vector< pair<Node *, vector<int> > > tree_map;
	tree_map.resize(tree->num_nodes); /* allocate space for the the nodes */
	for (int i = 0; i < tree_map.size(); i++){
		tree_map[i].first = NULL;
	}


	traverse_tree(tree->get_head(), &tree_map,
		[](Node * node, void * value)->void* {

		vector< pair<Node *, vector<int> > > * map = (vector< pair<Node *, vector<int> > > *)value;
		if ((*map)[node->order_num].first == NULL){
			for (int i = 0; i < node->srcs.size(); i++){
				(*map)[node->order_num].second.push_back(node->srcs[i]->order_num);
			}
		}

	}, empty_ret_mutator);

	/* now create the new tree as a vector */
	vector < pair<Node *, vector<int> > > new_tree_map;

	for (int i = 0; i < new_tree_map.size(); i++){

		new_tree_map.push_back(make_pair(
			node_creation(tree_map[i].first, peripheral_data),
			tree_map[i].second)
			);
	}


	set_head(new_tree_map[0].first);
	/* now create the linkage structure */
	for (int i = 0; i < new_tree_map.size(); i++){
		Node * node = new_tree_map[i].first;
		vector<int> srcs = new_tree_map[i].second;
		for (int j = 0; j < srcs.size(); i++){
			node->srcs.push_back(new_tree_map[srcs[j]].first);
			new_tree_map[srcs[j]].first->prev.push_back(node);
			new_tree_map[srcs[j]].first->pos.push_back(j);
		}
	}

	/* done! -> may have a better algorithm check */


}

void Tree::copy_unrolled_tree_structure(Tree * tree, void * peripheral_data, node_to_node node_creation){

	set_head(node_creation(tree->get_head(),peripheral_data));
	copy_unrolled_tree_structure(tree->get_head(), get_head(), peripheral_data, node_creation);

}

void Tree::copy_unrolled_tree_structure(Node * from, Node * to, void * peripheral_data, node_to_node node_creation){

	for (int i = 0; i < from->srcs.size(); i++){
		Node * new_node = node_creation(from->srcs[i], peripheral_data);
		to->srcs.push_back(new_node);
		new_node->prev.push_back(to);
		new_node->pos.push_back(i);
		copy_unrolled_tree_structure(from->srcs[i], new_node, peripheral_data, node_creation);
	}

}

void Tree::change_head_node()
{
	if (head->operation == op_assign){ /* then this node can be removed */
		ASSERT_MSG((head->srcs.size() == 1), ("ERROR: unexpected number of operands\n"));
		Node * new_head = head->srcs[0];
		new_head->prev.clear();
		new_head->pos.clear();
		delete head;
		head = new_head;
	}
}


bool Tree::are_trees_similar(Tree * tree)
{
	vector<Node *> nodes;
	nodes.push_back(get_head());
	nodes.push_back(tree->get_head());
	return are_trees_similar(nodes);
}

bool Tree::are_trees_similar(std::vector<Tree *> trees)
{

	vector<Node *> nodes;
	for (int i = 0; i < trees.size(); i++){
		nodes.push_back(trees[i]->get_head());
	}

	return are_trees_similar(nodes);

}

bool Tree::are_trees_similar(std::vector<Node *> node)
{
	if (!are_trees_similar(node)) return false;
	

	if (node.size() > 0){
		for (int i = 0; i < node[0]->srcs.size(); i++){
			vector<Node *> nodes;
			for (int j = 0; j < node.size(); j++){
				nodes.push_back(node[j]->srcs[i]);
			}
			if (!are_trees_similar(nodes)) return false;
		}
	}
	
	return true;

}





