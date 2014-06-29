#ifndef _PLAIN_PRINT_H
#define _PLAIN_PRINT_H

#include "node.h"
#include "canonicalize.h"
#include <iostream>
#include <fstream>
#include "defines.h"

//this file contains the function that prints out the expression tree in a simple pseudo-code fashion 

void flatten_to_expression(Node * head, std::ostream &file);

#endif