#ifndef _TREE_TRANS_H
#define _TREE_TRANS_H

#include "node.h"

void do_remove_signex(Node * node, Node * head);
void identify_parameters(Node * node);

bool remove_full_overlap_nodes_aggressive(Node * node, Node * head, uint width);
int remove_full_overlap_nodes_conservative(Node * node, Node * head);

void canonicalize_node(Node * node);

#endif