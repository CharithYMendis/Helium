#ifndef _PLAIN_PRINT_H
#define _PLAIN_PRINT_H

#include "node.h"
#include "build_abs_tree.h"
#include "canonicalize.h"
#include <iostream>
#include <fstream>
#include "defines.h"

//this file contains the function that prints out the expression tree in a simple pseudo-code fashion 

void print_node_tree(Node * head, std::ostream &file);

string regname_to_string(uint reg);
string operation_to_string(uint operation);
string opnd_to_string(operand_t * opnd);
string dr_operation_to_string(uint operation);
string abs_node_to_string(Abs_node* node);
string dr_logical_to_string(uint32_t opcode);

#endif