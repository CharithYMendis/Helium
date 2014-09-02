#ifndef _BUILDEX_EXTRACT_MEMREGIONS_H
#define _BUILDEX_EXTRACT_MEMREGIONS_H

#include <stdint.h>
#include <string>
#include "imageinfo.h"
#include "memregions.h"

/* 
* following are mem layout dependant functions which convert concrete mem layout into a abstracted D-dimensional strucuture
* function naming convention -
C,R - column or row major layout
N,E - embedded or not embedded colors

*/

mem_regions_t * locate_image_CN2(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image);


#endif

