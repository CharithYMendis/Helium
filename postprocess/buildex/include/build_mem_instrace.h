#ifndef _MEM_LAYOUT_H
#define _MEM_LAYOUT_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include "expression_tree.h"
#include "canonicalize.h"

using namespace std;

//directions
#define MEM_INPUT		0x1
#define MEM_OUTPUT		0x2


struct mem_info_t {

	uint32_t type;  /*mem type*/
	uint32_t direction; /* input / output */

	/* start and end instructions */
	uint64_t start;
	uint64_t end;

	/* stride */
	vector<pair<uint, uint> > stride_freqs;
};

void create_mem_layout(std::ifstream &in, vector<mem_info_t *> &mem_info);
void create_mem_layout(vector<cinstr_t * > &instrs, vector<mem_info_t *> &mem_info);
void print_mem_layout(vector<mem_info_t *> &mem_info);

void random_dest_select(vector<mem_info_t *> &mem_info, uint64_t * dest, uint32_t * stride);
int get_most_probable_stride(vector<pair<uint, uint> > &strides);


#endif
