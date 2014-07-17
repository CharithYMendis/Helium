#include "tree_transformations.h"
#include "canonicalize.h"
#include "defines.h"
#include <map>


/* internal helper functions */
/* (src -> ref) => (src) */
void remove_forward_ref(Node * src, Node *ref){

	bool erase = false;
	for (int i = 0; i < src->srcs.size(); i++){
		if (src->srcs[i] == ref){
			src->srcs.erase(src->srcs.begin() + i);
			erase = true;
			break;
		}
	}

	/* need to update backward references of still connected nodes */
	if (erase){
		for (int i = 0; i < src->srcs.size(); i++){
			for (int j = 0; j < src->srcs[i]->prev.size(); j++){
				if (src->srcs[i]->prev[j] == src){
					src->srcs[i]->pos[j] = i;
				}
			}
		}
	}
}

/* (ref -> src) => (src) */
void remove_backward_ref(Node * src, Node * ref){

	for (int i = 0; i < src->prev.size(); i++){
		if (src->prev[i] == ref){
			src->prev.erase(src->prev.begin() + i);
			src->pos.erase(src->pos.begin() + i);
		}
	}

}

/* (dst -> ref -> src)  => (dst -> src) */
void change_ref(Node * dst, Node *ref, Node *src){

	remove_forward_ref(dst, ref);
	remove_backward_ref(src, ref);


	bool present = false;/* already there or not*/
	for (int i = 0; i < dst->srcs.size(); i++){
		if (dst->srcs[i] == src){
			present = true;
			break;
		}
	}

	if (!present){
		uint index = dst->srcs.size();
		dst->srcs.push_back(src);
		src->prev.push_back(dst);
		src->pos.push_back(index);
	}

}

void safely_delete(Node * node, Node * head){

	if ( (node->prev.size() == 0) && (node != head) ){
		/* remove any backward references to this node if existing ; we are assuming that there can only be one head node for the tree */
		for (int i = 0; i < node->srcs.size(); i++){
			remove_backward_ref(node->srcs[i], node);
		}
		delete node;
	}

}

void remove_sign_extended_nodes(Node * node, Node * head){

	if (node->operation == op_concat){
	
		ASSERT_MSG(((mem_range_to_reg(node->symbol) == DR_REG_VIRTUAL_1) || (mem_range_to_reg(node->symbol) == DR_REG_VIRTUAL_2)),("ERROR: concat operation not on a virtual node\n"));

		/* get the nodes in which direct value is used */
		vector<Node *> direct;

		for (int i = 0; i < node->srcs.size(); i++){
			if (node->srcs[i]->operation != op_signex){
				direct.push_back(node->srcs[i]);
			}
		}

		/* now check whether the sign extended nodes directly refer to direct ones
		* if so get rid of the dependancy; note that signex nodes can only have one src from which it is getting the sign extension
		*/

		for (int i = 0; i < node->srcs.size(); i++){
			if (node->srcs[i]->operation == op_signex){
				
				ASSERT_MSG((node->srcs[i]->srcs.size() == 1), ("ERROR: sign extended nodes should have exactly one src\n"));
				Node * ref = node->srcs[i]->srcs[0];

				for (int j = 0; j < direct.size(); j++){
					if (ref == direct[j]){ /*remove the sign extended node*/
						Node * snode = node->srcs[i];
						remove_forward_ref(node, snode);
						remove_backward_ref(ref, snode);
						safely_delete(snode,head);
					}
				}

			}
		}

		/* check if we need the virtual node at all */
		if (node->srcs.size() == 1){

			for (int i = 0; i < node->prev.size(); i++){
				change_ref(node->prev[i], node, node->srcs[0]);
			}
		}



	}
}


/* external tree transformations */
void do_remove_signex(Node * node, Node * head){

	remove_sign_extended_nodes(node, head);
	for (int i = 0; i < node->srcs.size(); i++){
		do_remove_signex(node->srcs[i], head);
	}

} 


/*  order numbering for printing out */
static uint num = 0;

uint number_tree(Node * node){

	if (node->order_num == -1){
		node->order_num = num;
		num++;
	}

	for (int i = 0; i < node->srcs.size(); i++){
		number_tree(node->srcs[i]);
	}

	return num;

}

uint number_expression_tree(Node * node){

	num = 0;
	return number_tree(node);

}


/*should implement to suite the current standard calling conventions used by the applications*/
static uint para_num = 0;

int get_parameter_num(){
	return para_num++;
}


void identify_parameters(Node * node){

	if (node->srcs.size() == 0){
		if (node->symbol->type == MEM_STACK_TYPE || node->symbol->type == REG_TYPE){
			node->is_para = true;
			node->para_num = get_parameter_num();
		}
	}
	else{
		for (int i = 0; i < node->srcs.size(); i++){
			identify_parameters(node->srcs[i]);
		}
	}

}