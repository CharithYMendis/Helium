#ifndef _BUILD_TREE_H
#define _BUILD_TREE_H


#include <fstream>
#include <iostream>
#include <vector>
#include "fileparser.h"
#include "defines.h"
#include "expression_tree.h"


#define FILE_BEGINNING  -2
#define FILE_ENDING		-1

void build_tree(uint64 destination, int start_trace, int end_trace, ifstream &file, Expression_tree * tree);
void build_tree(uint64 destination, int start_trace, int end_trace, vector<cinstr_t *> &instrs, Expression_tree * tree);

#endif