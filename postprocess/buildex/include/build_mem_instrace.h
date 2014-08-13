#ifndef _MEM_LAYOUT_H
#define _MEM_LAYOUT_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include "expression_tree.h"
#include "canonicalize.h"
#include "meminfo.h"

using namespace std;

void create_mem_layout(std::ifstream &in, vector<mem_info_t *> &mem_info);
void create_mem_layout(std::ifstream &in, vector<pc_mem_region_t *> &pc_mems);

void create_mem_layout(vector<cinstr_t * > &instrs, vector<mem_info_t *> &mem_info);
void create_mem_layout(vector<cinstr_t * > &instrs, vector<pc_mem_region_t *> &pc_mems);



#endif
