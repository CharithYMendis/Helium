#ifndef _EXALGO_SYMPY_H
#define _EXALGO_SYMPY_H

#include <string>
#include <node.h>


int sym_ex(const char * input_expression, const char * output_expression);
std::string get_simplify_string(Node * node);

#endif