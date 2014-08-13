#ifndef _FILTER_LOGIC_H
#define _FILTER_LOGIC_H

#include "moduleinfo.h"
#include "imageinfo.h"
#include "meminfo.h"
#include <vector>



void filter_based_on_freq(moduleinfo_t * head, image_t * image, uint32_t low_percentage);
void filter_based_on_composition(moduleinfo_t * module);
void filter_mem_regions(std::vector<pc_mem_region_t *> &pc_mems, image_t * in_image, image_t * out_image, uint32_t min_threhsold);
void filter_based_on_memory_dependancy(std::vector<pc_mem_region_t *> &pc_mems, moduleinfo_t * head);


#endif