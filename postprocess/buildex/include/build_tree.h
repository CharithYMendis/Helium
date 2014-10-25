#ifndef _BUILD_TREE_H
#define _BUILD_TREE_H

#include <fstream>
#include <iostream>
#include <vector>
#include "fileparser.h"
#include "defines.h"
#include "expression_tree.h"
#include "memregions.h"


#define FILE_BEGINNING  -2
#define FILE_ENDING		-1

void build_tree(uint64 destination, uint32_t stride, vector<uint32_t> start_points, int start_trace, int end_trace, 
	Expression_tree * tree, vec_cinstr &instrs, vector<disasm_t *> disasm);
void build_tree(uint64 destination, int start_trace, int end_trace, std::ifstream &file, Expression_tree * tree, std::vector<disasm_t *> disasm);

Expression_tree * create_tree_for_dest(uint64_t dest, uint32_t stride, ifstream &instrace_file, vector<uint32_t> start_points,
	int32_t start_trace, int32_t end_trace, vector<disasm_t *> disasm);
vector<uint32_t> get_instrace_startpoints(vec_cinstr &instrs, uint32_t pc);
bool are_conc_trees_similar(Node * first, Node * second);
vector< vector<Expression_tree *> >  categorize_trees(vector<Expression_tree * > trees);
vector<Expression_tree *> get_similar_trees(vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride, vec_cinstr &instrs,
	vector<uint32_t> start_points, int32_t end_trace, vector<disasm_t *> disasm);
vector< vector <Expression_tree *> > cluster_trees(vector<mem_regions_t *> mem_regions, vector<uint32_t> start_points, 
	vec_cinstr &instrs, vector<disasm_t *> disasm, string output_folder);

void cinstr_convert_reg(cinstr_t * instr);



#endif