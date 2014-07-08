#ifndef _PLAIN_PRINT_H
#define _PLAIN_PRINT_H

#include "node.h"
#include "canonicalize.h"
#include <iostream>
#include <fstream>
#include "defines.h"

//this file contains the function that prints out the expression tree in a simple pseudo-code fashion 

void flatten_to_expression(Node * head, std::ostream &file);

string regname_to_string(uint reg);
string operation_to_string(uint operation);
string opnd_to_string(operand_t * opnd);
uint number_tree(Node * node);
string dr_operation_to_string(uint operation);

#endif