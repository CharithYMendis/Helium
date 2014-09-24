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

void build_tree(uint64 destination, int start_trace, int end_trace, std::ifstream &file, Expression_tree * tree, std::vector<disasm_t *> disasm);
void build_tree(uint64 destination, int start_trace, int end_trace, std::vector<cinstr_t *> &instrs, Expression_tree * tree);
std::vector<uint32_t> get_instrace_startpoints(std::ifstream &file, uint32_t pc);
bool are_conc_trees_similar(Node * first, Node * second);


#endif