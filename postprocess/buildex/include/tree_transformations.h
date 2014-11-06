#ifndef _TREE_TRANS_H
#define _TREE_TRANS_H

#include "node.h"
#include "meminfo.h"

void do_remove_signex(Node * node, Node * head);
void identify_parameters(Node * node, std::vector<pc_mem_region_t *> pc_mems);

bool remove_full_overlap_nodes_aggressive(Node * node, Node * head, uint width);
int remove_full_overlap_nodes_conservative(Node * node, Node * head);

void canonicalize_node(Node * node);
void simplify_identity_mul(Node * node);
void simplify_identity_add(Node * node);
void order_tree(Node * node);

#endif