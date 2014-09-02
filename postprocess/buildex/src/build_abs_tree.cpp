#include "build_abs_tree.h"
#include <math.h>
#include "utilities.h"
#include "memregions.h"


Abs_node::Abs_node(){
	order_num = -1;
}

Comp_Abs_node::Comp_Abs_node(){
	order_num = -1;
}


Abs_tree::Abs_tree(){
	head = NULL;
}

/* fills the abs_node from node and mem_regions */
void fill_abs_node(Abs_node * abs_node, Node * node, vector<mem_regions_t *> &mem_regions){

	if (node->symbol->type == MEM_HEAP_TYPE){

		mem_regions_t * mem = get_mem_region(node->symbol->value, mem_regions);
		abs_node->operation = node->operation;

		switch (mem->type){
		case IMAGE_INPUT: abs_node->type = INPUT_NODE; break;
		case IMAGE_OUTPUT: abs_node->type = OUTPUT_NODE; break; 
		case IMAGE_INTERMEDIATE: abs_node->type = INTERMEDIATE_NODE; break;
		}
		abs_node->width = node->symbol->width;

		abs_node->mem_info.associated_mem = mem;
		abs_node->mem_info.dimensions = mem->dimensions;
		abs_node->mem_info.indexes = new int * [mem->dimensions];
		abs_node->mem_info.pos = new int[mem->dimensions];

		vector<int> pos = get_mem_position(mem, node->symbol->value);
		for (int i = 0; i < mem->dimensions; i++){
			abs_node->mem_info.indexes[i] = new int[mem->dimensions + 1];
			abs_node->mem_info.pos[i] = pos[i];
		}
	}
	else if ((node->symbol->type == MEM_STACK_TYPE) || (node->symbol->type == REG_TYPE)){ /*parameters can be here*/
		abs_node->operation = node->operation;
		abs_node->width = node->symbol->width;
		if (node->is_para){
			abs_node->type = PARAMETER;
			abs_node->value = node->para_num;
		}
		else{
			abs_node->type = OPERATION_ONLY;
		}
	}
	else if (node->symbol->type == IMM_INT_TYPE){
		abs_node->operation = node->operation;
		abs_node->value = node->symbol->value;
		abs_node->width = node->symbol->width;
		abs_node->type = IMMEDIATE_INT;
	}
	else if (node->symbol->type == IMM_FLOAT_TYPE){
		abs_node->operation = node->operation;
		abs_node->float_value = node->symbol->float_value;
		abs_node->width = node->symbol->width;
		abs_node->type = IMMEDIATE_FLOAT;
	}
	else{
		ASSERT_MSG((false), ("ERROR: node type unknown\n"));
	}

	abs_node->range = node->symbol->value;

	/* change this sometime */
	abs_node->sign = false;

}

/* this builds the abs tree from the concrete tree */
void Abs_tree::build_abs_tree(Abs_node * abs_node, Node * node, vector<mem_regions_t *> &mem_regions){

	if (abs_node == NULL){
		abs_node = head = new Abs_node();
		fill_abs_node(head, node, mem_regions);
	}

	for (int i = 0; i < node->srcs.size(); i++){
		Abs_node * new_node = new Abs_node();
		abs_node->srcs.push_back(new_node);
		new_node->prev = abs_node;
		fill_abs_node(new_node, node->srcs[i], mem_regions);
		build_abs_tree(new_node, node->srcs[i], mem_regions);
	}

}

bool Abs_tree::are_abs_trees_similar(vector<Abs_node *> abs_nodes){
	
	if (abs_nodes.size() == 0) return true;

	Abs_node * first_node = abs_nodes[0];

	for (int i = 1; i < abs_nodes.size(); i++){
		if ((first_node->type != abs_nodes[i]->type) || (first_node->operation != abs_nodes[i]->operation)){
			return false;
		}
	}

	/*check whether all the nodes have same number of sources*/
	uint no_srcs = abs_nodes[0]->srcs.size();

	for (int i = 1; i < abs_nodes.size(); i++){
		if (abs_nodes[i]->srcs.size() != no_srcs){
			return false;
		}
	}
	
	/* recursively check whether the src nodes are similar*/
	for (int i = 0; i < no_srcs; i++){
		vector<Abs_node *> nodes;
		for (int j = 0; j < abs_nodes.size(); j++){
			nodes.push_back(abs_nodes[j]->srcs[i]);
		}
		bool ret = are_abs_trees_similar(nodes);
		if (!ret) return false;
	}

	return true;

}

void Abs_tree::seperate_intermediate_buffers(Abs_node * node){

}


/* compound tree implementation */
Comp_Abs_tree::Comp_Abs_tree(){

	head = NULL;

}

void Comp_Abs_tree::build_compound_tree(Comp_Abs_node * comp_node, vector<Abs_node *> abs_nodes){

	if (abs_nodes.size() == 0) return;

	if (comp_node == NULL){
		comp_node = head = new Comp_Abs_node();
	}

	for (int i = 0; i < abs_nodes.size(); i++){
		comp_node->nodes.push_back(abs_nodes[i]);
	}

	for (int i = 0; i < abs_nodes[0]->srcs.size(); i++){
		Comp_Abs_node * new_node = new Comp_Abs_node();
		new_node->prev = comp_node;
		comp_node->srcs.push_back(new_node);

		vector<Abs_node *> new_abs_nodes;

		for (int j = 0; j < abs_nodes.size(); j++){
			new_abs_nodes.push_back(abs_nodes[j]->srcs[i]);
		}
		build_compound_tree(new_node, new_abs_nodes);
	}


}


void Comp_Abs_tree::abstract_buffer_indexes(Comp_Abs_node * comp_node){

	Comp_Abs_node * head_for_ex = comp_node;

	/* assert that the comp node is an input or an intermediate node */
	for (int i = 0; i < head_for_ex->srcs.size(); i++){
		abstract_buffer_indexes(head_for_ex, head_for_ex->srcs[i]);
	}

}

void Comp_Abs_tree::abstract_buffer_indexes(Comp_Abs_node * head, Comp_Abs_node * node){

	Abs_node * first = node->nodes[0];

	if ((first->type == INPUT_NODE) || (first->type == INTERMEDIATE_NODE)){
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

		/*cout << "A" << endl;
		printout_matrices(A);*/


		for (int dim = 0; dim < first->mem_info.dimensions; dim++){
			vector<double> b;
			//for (int i = 0; i < node->nodes.size(); i++){
			for (int i = 0; i < node->nodes.size(); i++){
				b.push_back((double)node->nodes[i]->mem_info.pos[dim]);
			}
			/*cout << "b" << endl;
			printout_vector(b);*/

			vector<double> results = solve_linear_eq(A, b);

			/*cout << "results" << endl;
			printout_vector(results);*/

			ASSERT_MSG((results.size() == (first->mem_info.dimensions + 1)), ("ERROR: the result vector is inconsistent\n"));
			for (int i = 0; i < results.size(); i++){
				first->mem_info.indexes[dim][i] = double_to_int(results[i]);
				//cout << first->mem_info.indexes[dim][i] << " ";
			}
			//cout << endl;
			
		}
	
	}
	else if ((first->type == SUBTREE_BOUNDARY)){
		return;
	}


	for (int i = 0; i < node->srcs.size(); i++){
		abstract_buffer_indexes(head, node->srcs[i]);
	}

}

Abs_node * Comp_Abs_tree::compound_to_abs_tree(){

	if (head == NULL){
		return NULL;
	}
	else{
		return head->nodes[0];
	}
	
}