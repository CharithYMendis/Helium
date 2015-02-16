#ifndef _MEMDUMP_H
#define _MEMDUMP_H


#include <stdint.h>
#include <string>
#include <fstream>

#include "analysis/x86_analysis.h"
#include "imageinfo.h"
#include "memory/memregions.h"
#include "memory/meminstrace.h"

/* 
* following are mem layout dependant functions which convert concrete mem layout into a abstracted D-dimensional strucuture
* function naming convention -
C,R - column or row major layout
N,E - embedded or not embedded colors

*/

/* these should be private methods */
mem_regions_t * locate_image_CN2(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image);
mem_regions_t * locate_image_CN2_backward(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image);
mem_regions_t * locate_image_CN2_backward_write(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image);


/* 
*  This module can be used in two ways - the output is a more rich data structure with more information about the mem layout 
*	1. Use the instrace to build the mem info and with images and a config file for the program concerned get mem regions 
*   2. Use mem dumps to automatically figure out where the images are stored and mem layout attributes 
*/

/* use mem dump to identify the mem regions - this is independent of the instrace related mem info */
std::vector<mem_regions_t *> get_image_regions_from_dump(std::vector<string> filenames, std::string in_image_filename, std::string out_image_filename);


#endif