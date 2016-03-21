#include <assert.h>
#include <fstream>
#include <iostream>

#include "utility/defines.h"
#include "analysis/x86_analysis.h"
#include "trees/trees.h"
#include "trees/nodes.h"
#include "utility/print_helper.h"
#include "utilities.h"

using namespace std;

Comp_Abs_Tree::Comp_Abs_Tree() : Tree()
{

}

Comp_Abs_Tree::~Comp_Abs_Tree()
{
	

}

/* these routines are specific to compound trees which needs traversal of number of similar trees 
   together. These can be abstracted and lifted to act on general trees, but as this is the only case we opt
   to have these specialized functions
*/
void Comp_Abs_Tree::build_compound_tree_unrolled(Comp_Abs_Node *comp_node, std::vector<Abs_Node *> abs_nodes){

	for (int i = 0; i < abs_nodes[0]->srcs.size(); i++){

		vector<Abs_Node *> nodes;
		
		for (int j = 0; j < abs_nodes.size(); j++){
			nodes.push_back(static_cast<Abs_Node *>(abs_nodes[j]->srcs[i]));
		}

		Comp_Abs_Node * new_node = new Comp_Abs_Node(nodes);
		new_node->prev.push_back(comp_node);
		new_node->pos.push_back(i);
		comp_node->srcs.push_back(new_node);

		build_compound_tree_unrolled(new_node, nodes);
	}
}

void Comp_Abs_Tree::build_compound_tree_unrolled(std::vector<Abs_Tree *> abs_trees){

	if (abs_trees.size() == 0) return;

	vector<Abs_Node *> nodes;
	for (int i = 0; i < abs_trees.size(); i++){
		nodes.push_back(static_cast<Abs_Node *>(abs_trees[i]->get_head()));
	}
	Comp_Abs_Node * comp_node = new Comp_Abs_Node(nodes);
	set_head(comp_node);
	this->recursive = abs_trees[0]->recursive;
	build_compound_tree_unrolled(comp_node, nodes);

}

void Comp_Abs_Tree::traverse_trees(vector<Abs_Node *> abs_nodes,
	vector< pair < vector <Abs_Node *>, vector <int> > >  &node_pos){

	if (abs_nodes.size() == 0) return;

	int order_num = abs_nodes[0]->order_num;
	Abs_Node * abs_node = abs_nodes[0];

	if (node_pos[order_num].first.size() == 0){
		node_pos[order_num].first = abs_nodes;
		for (int i = 0; i < abs_node->srcs.size(); i++){
			node_pos[order_num].second.push_back(abs_node->srcs[i]->order_num);

			vector<Abs_Node *> new_srcs;
			for (int j = 0; j < abs_nodes.size(); j++){
				new_srcs.push_back(static_cast<Abs_Node *>(abs_nodes[j]->srcs[i]));
			}

			traverse_trees(new_srcs, node_pos);
		}
	
	}

}

void Comp_Abs_Tree::build_compound_tree_exact(std::vector<Abs_Tree *> abs_trees)
{

	if (abs_trees.size() == 0) return;
	
	/* assumption the tree is numbered and the visited state is cleared */
	ASSERT_MSG((abs_trees[0]->get_head()->order_num != -1), ("ERROR: the concrete tree is not numbered\n"));
	ASSERT_MSG((abs_trees[0]->get_head()->visited == false), ("ERROR: the visit state of the concrete tree is not cleared\n"));


	/* get all the nodes and the nodes to which it is connected */
	vector< pair< vector<Abs_Node *> , vector<int> > > tree_map;
	tree_map.resize(abs_trees[0]->num_nodes); /* allocate space for the the nodes */
	
	/* create the set of Abs_Node heads */
	vector<Abs_Node *> nodes;
	for (int i = 0; i < abs_trees.size(); i++){
		nodes.push_back(static_cast<Abs_Node *>(abs_trees[i]->get_head()));
	}

	traverse_trees(nodes, tree_map);

	/* now create the new tree as a vector */
	vector < pair<Comp_Abs_Node *, vector<int> > > new_tree_map;

	for (int i = 0; i < new_tree_map.size(); i++){

		new_tree_map.push_back(make_pair(
			new Comp_Abs_Node(tree_map[i].first),
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



}

Comp_Abs_Node * get_indirect_access_node(Comp_Abs_Node * node){


	if (node->srcs.size() == 0){
		if (node->nodes[0]->mem_info.associated_mem != NULL){
			return node;
		}
		else{
			return NULL;
		}
	}

	Comp_Abs_Node * ret;

	for (int i = 0; i < node->srcs.size(); i++){
		ret = get_indirect_access_node((Comp_Abs_Node *)node->srcs[i]);
		if (ret != NULL) break;
	}


	return ret;

}

int32_t is_indirect_access(Comp_Abs_Node * node){

	int32_t pos = -1;
	// is this node indirect? 
	for (int i = 0; i < node->srcs.size(); i++){
		Comp_Abs_Node * src = (Comp_Abs_Node *)node->srcs[i];
		if (src->nodes[0]->operation == op_indirect){
			pos = i;
			break;
		}
	}

	return pos;

}

void remove_same_values(bool * deleted_index, vector < vector< double> > &A){

	deleted_index[A[0].size() - 1] = false;
	uint32_t temp_count = 0;

	/* check if a particular dimension is the same? */
	for (int j = 0; j < A[0].size() - 1; j++){

		bool same = true;
		double value = A[0][j];
		for (int i = 1; i < A.size(); i++){
			if (abs(value - A[i][j]) > 1e-6){
				same = false; break;
			}
		}

		/* if same true */
		if (same){
			for (int i = 0; i < A.size(); i++){
				A[i].erase(A[i].begin() + j);
			}
			j--;
		}
		deleted_index[temp_count++] = same;

	}

}


void abstract_buffer_indexes_traversal(Comp_Abs_Node * head, Comp_Abs_Node * node){

	Abs_Node * first = node->nodes[0];

	if (node->visited){
		return;
	}
	else{
		node->visited = true;
	}

	bool indirect = (is_indirect_access(node) != -1);

	if ( ((first->type == Abs_Node::INPUT_NODE) || (first->type == Abs_Node::INTERMEDIATE_NODE) || (first->type == Abs_Node::OUTPUT_NODE)) && !indirect ){
		/*make a system of linear equations and solve them*/

		vector<vector<double> > A;
		//for (int i = 0; i < head->nodes.size(); i++){
		for (int i = 0; i < node->nodes.size(); i++){
			vector<double> coeff;
			for (int j = 0; j < head->nodes[i]->mem_info.dimensions; j++){
				coeff.push_back((double)head->nodes[i]->mem_info.pos[j]);
			}
			coeff.push_back(1.0);
			A.push_back(coeff);
		}

		printout_matrices(A);
		bool  * deleted_index = new bool[A[0].size()];
		remove_same_values(deleted_index, A);
		cout << endl;
		printout_matrices(A);


		for (int dim = 0; dim < first->mem_info.dimensions; dim++){
			vector<double> b;
			//for (int i = 0; i < node->nodes.size(); i++){
			for (int i = 0; i < node->nodes.size(); i++){
				b.push_back((double)node->nodes[i]->mem_info.pos[dim]);
			}

			printout_vector(b);
			vector<double> results = solve_linear_eq(A, b);

			uint32_t head_dimensions = head->nodes[0]->mem_info.dimensions;
			//ASSERT_MSG((results.size() == (first->mem_info.dimensions + 1)), ("ERROR: the result vector is inconsistent\n"));
			uint32_t tcount = 0;
			for (int i = 0; i < head_dimensions + 1; i++){
				if (deleted_index[i] == false){
					first->mem_info.indexes[dim][i] = double_to_int(results[tcount++]);
				}
				else{
					first->mem_info.indexes[dim][i] = 0;
				}
			}
		}

	}
	else if (first->type == Abs_Node::IMMEDIATE_INT){

		vector<vector<double> > A;
		//for (int i = 0; i < head->nodes.size(); i++){
		for (int i = 0; i < node->nodes.size(); i++){
			vector<double> coeff;
			for (int j = 0; j < head->nodes[i]->mem_info.dimensions; j++){
				coeff.push_back((double)head->nodes[i]->mem_info.pos[j]);
			}
			coeff.push_back(1.0);
			A.push_back(coeff);
		}


		//cout << "imm" << endl;
		//printout_matrices(A);

		bool  * deleted_index = new bool[A[0].size()];
		remove_same_values(deleted_index, A);

		vector<double> b;

		first->mem_info.indexes = new int * [1];
		first->mem_info.indexes[0] = new int[head->nodes[0]->mem_info.dimensions + 1];
		first->mem_info.dimensions = 1;
		first->mem_info.head_dimensions = head->nodes[0]->mem_info.dimensions;

		for (int i = 0; i < node->nodes.size(); i++){
			b.push_back((long long)node->nodes[i]->symbol->value);
		}

		//printout_vector(b);

		vector<double> results = solve_linear_eq(A, b);

		uint32_t head_dimensions = head->nodes[0]->mem_info.dimensions;
		//ASSERT_MSG((results.size() == (first->mem_info.dimensions + 1)), ("ERROR: the result vector is inconsistent\n"));
		uint32_t tcount = 0;
		for (int i = 0; i < head_dimensions + 1; i++){
			if (deleted_index[i] ==  false){
				first->mem_info.indexes[0][i] = double_to_int(results[tcount++]);
			}
			else{
				first->mem_info.indexes[0][i] = 0;
			}
		}

	}
	else if ((first->type == Abs_Node::SUBTREE_BOUNDARY)){
		return;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		abstract_buffer_indexes_traversal(head, static_cast<Comp_Abs_Node *>(node->srcs[i]));
	}

}



void Comp_Abs_Tree::abstract_buffer_indexes()
{
	Comp_Abs_Node * act_head = static_cast<Comp_Abs_Node *>(get_head());

	int32_t pos = is_indirect_access(act_head);
	if (pos != -1){
		Comp_Abs_Node * node = static_cast<Comp_Abs_Node *>(act_head->srcs[pos]);
		act_head = get_indirect_access_node(node);
		DEBUG_PRINT(("indirect head node access\n"), 2);
	}

	/* assert that the comp node is an input or an intermediate node */
	for (int i = 0; i < head->srcs.size(); i++){
		abstract_buffer_indexes_traversal(act_head, static_cast<Comp_Abs_Node*>(head->srcs[i]));
	}

	cleanup_visit();
}

Abs_Tree * Comp_Abs_Tree::compound_to_abs_tree()
{
	Abs_Node * node = static_cast<Abs_Node *>(static_cast<Comp_Abs_Node *>(get_head())->nodes[0]);
	Abs_Tree * tree = new Abs_Tree();
	tree->recursive = recursive;
	tree->set_head(node);
	return tree;

}

std::string Comp_Abs_Tree::serialize_tree()
{
	throw "not implemented!";
}

void Comp_Abs_Tree::construct_tree(std::string stree)
{
	throw "not implemented!";
}