#ifndef _MEMANALYSIS_H
#define _MEMANALYSIS_H

#include <vector>

#include "memory/memregions.h"
#include "meminfo.h"
#include "imageinfo.h"

/* merges / updates information that is present in mem_regions with instrace */
std::vector<mem_regions_t *> merge_instrace_and_dump_regions(std::vector<mem_regions_t *> &total_regions,
	std::vector<mem_info_t *> mem_info, std::vector<mem_regions_t *> mem_regions);
/*convert from mem regions identified by instrace to more information rich mem regions */
std::vector<mem_regions_t *> get_image_regions_from_instrace(std::vector<mem_info_t *> &mem, std::ifstream &config, image_t * in_image, image_t * out_image);

#endif