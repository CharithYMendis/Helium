#ifndef _MEMINSTRACE_H
#define _MEMINSTRACE_H

#include <vector>
#include <fstream>
#include "memory/memregions.h"
#include "meminfo.h"
#include "analysis/x86_analysis.h"

void create_mem_layout(std::ifstream &in, std::vector<mem_info_t *> &mem_info);
void create_mem_layout(std::ifstream &in, std::vector<pc_mem_region_t *> &pc_mems);

void create_mem_layout(std::vector<cinstr_t * > &instrs, std::vector<mem_info_t *> &mem_info);
void create_mem_layout(std::vector<cinstr_t * > &instrs, std::vector<pc_mem_region_t *> &pc_mems);


#endif