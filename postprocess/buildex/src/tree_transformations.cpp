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

	int index = -1;

	/* In place forward reference and push back back ward reference */
	for (int i = 0; i < dst->srcs.size(); i++){
		if (dst->srcs[i] == ref){
			dst->srcs[i] = src;
			dst->srcs[i]->prev.push_back(dst);
			dst->srcs[i]->pos.push_back(i);
		}
		index = i;
	}

	/* remove the backward reference of the changed node */
	if (index != -1){
		for (int i = 0; i < ref->pos.size(); i++){
			if (ref->pos[i] == index){
				ref->pos.erase(ref->pos.begin() + i);
				ref->prev.erase(ref->prev.begin() + i);
			}
		}
	}

}

void safely_delete(Node * node, Node * head){

	if ( (node->prev.size() == 0) && (node != head) ){
		/* remove any backward references to this node if existing ; we are assuming that there can only be one head node for the tree */
		/* no need to remove the forward reference as it is only within the deletable node */
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
			safely_delete(node, head);
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

/* full overlap removal - we can remove a full overlap as long as we can prove that the small width can be propagated downwards */
void remove_full_overlap_node(Node * node, Node * head){

	ASSERT_MSG((node->operation == op_full_overlap), ("ERROR: The node should be a full overlap\n"));

	for (int i = 0; i < node->prev.size(); i++){
		change_ref(node->prev[i], node, node->srcs[0]);
	}
	safely_delete(node,head);

}

bool remove_full_overlap_nodes_aggressive(Node * node, Node * head, uint width){


	int wanted_width;
	if (node->operation == op_full_overlap) wanted_width = node->symbol->width;
	else wanted_width = width;

	/* check whether it was possible remove overlap */
	bool is_possible = true;
	for (int i = 0; i < node->srcs.size(); i++){
		if (!remove_full_overlap_nodes_aggressive(node->srcs[i], head, wanted_width)){
			is_possible = false;
		}
	}


	if (node->operation == op_full_overlap){

		ASSERT_MSG((node->srcs.size() == 1), ("ERROR: full overlaps should only have one source\n"));

		/* check whether the overlap is at the end of the range */
		Node * overlap = node->srcs[0];
		if (overlap->symbol->value + overlap->symbol->width == node->symbol->value + node->symbol->width){
			/*now check whether the width */
			if (is_possible){
				remove_full_overlap_node(node, head);
			}
		}
		return true;
	}
	else if (node->operation == op_rsh){
		return  (width == node->srcs[0]->symbol->width);
	}
	else{
		return true;
	}

}

/* width wanted is from the end of the range */
int remove_full_overlap_nodes_conservative(Node * node, Node * head){

	/* leaf of the nodes; return the node size */
	if (node->srcs.size() == 0){
		return node->symbol->width;
	}

	vector<int> widths;
	uint max_width = 0;

	/* get the widths of the nodes connected to this node  + the min width */
	for (int i = 0; i < node->srcs.size(); i++){

		uint ret_width = remove_full_overlap_nodes_conservative(node->srcs[i], head);
		widths.push_back(ret_width);
		max_width = (max_width < ret_width) ? ret_width : max_width;

	}

	/* return the width depending on the node's operation */
	if (node->operation == op_full_overlap){
		
		ASSERT_MSG((node->srcs.size() == 1), ("ERROR: full overlaps should only have one source\n"));

		/* check whether the overlap is at the end of the range */
		Node * overlap = node->srcs[0];
		if (overlap->symbol->value + overlap->symbol->width == node->symbol->value + node->symbol->width){
			/*now check whether the width */
			if (max_width <= node->symbol->width){
				remove_full_overlap_node(node, head);
			}
		}
		return node->symbol->width; /* this should smaller than the source node width */

	}
	else if (node->operation == op_partial_overlap){
		return node->symbol->width; /* this is bigger than the source node widths - but we need to use all */
	}
	else if (node->operation == op_signex){
		return 0;  /* this is just a sign extension */
	}
	else{
		uint ret_size;
		if (node->operation == op_div){
			ret_size = widths[0];
		}
		else if (node->operation == op_mod){
			ret_size = (widths[0] < widths[1]) ? widths[0] : widths[1];
		}
		else if (node->operation == op_lsh || node->operation == op_rsh){
			ret_size = widths[0];
		}
		else{
			ret_size = max_width;
		}
		
		if (node->symbol->width < ret_size){
			ret_size = node->symbol->width;
		}

		return ret_size;

	}

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