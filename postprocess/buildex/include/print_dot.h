#ifndef _PRINT_DOT_H
#define _PRINT_DOT_H

#include <fstream>
#include "node.h"
#include "defines.h"
#include "print_common.h"
#include "print_dot.h"
#include "build_abs_tree.h"
#include <string>


uint number_tree_nodes(Node * node);
uint number_tree_nodes(Abs_node * node);
uint number_tree_nodes(Comp_Abs_node * node);

void print_to_dotfile(ofstream &file, Node * head, uint no_of_nodes, uint graph_no);
void print_to_dotfile(ofstream &file, Abs_node * head, uint no_of_nodes, uint graph_no, bool abs);
void print_to_dotfile(ofstream &file, Comp_Abs_node * head, uint no_of_nodes, uint graph_no);
#endif