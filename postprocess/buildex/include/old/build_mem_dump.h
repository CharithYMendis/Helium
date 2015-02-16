#ifndef _BUILD_MEM_DUMP_H
#define _BUILD_MEM_DUMP_H

#include "canonicalize.h"
#include "imageinfo.h"
#include <fstream>
#include "build_mem_instrace.h"
#include "memregions.h"

/* main interface for the application 
1. This will have routines which build mem regions from dumps
*/


/* 
*  This module can be used in two ways - the output is a more rich data structure with more information about the mem layout 
*	1. Use the instrace to build the mem info and with images and a config file for the program concerned get mem regions 
*   2. Use mem dumps to automatically figure out where the images are stored and mem layout attributes 
*/

/* use mem dump to identify the mem regions - this is independent of the instrace related mem info */
std::vector<mem_regions_t *> get_image_regions_from_dump(std::vector<string> filenames, std::string in_image_filename, std::string out_image_filename);
/*convert from mem regions identified by instrace to more information rich mem regions */
std::vector<mem_regions_t *> get_image_regions_from_instrace(std::vector<mem_info_t *> &mem, std::ifstream &config, image_t * in_image, image_t * out_image);  


/* merges / updates information that is present in mem_regions with instrace */
std::vector<mem_regions_t *> merge_instrace_and_dump_regions(std::vector<mem_regions_t *> &total_regions,
	std::vector<mem_info_t *> mem_info, std::vector<mem_regions_t *> mem_regions);

#endif

