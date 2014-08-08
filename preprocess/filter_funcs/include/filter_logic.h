#ifndef _FILTER_LOGIC_H
#define _FILTER_LOGIC_H

#include "moduleinfo.h"
#include "image_manipulation.h"

void filter_based_on_freq(moduleinfo_t * head, image_t * image, uint32_t low_percentage);
void filter_based_on_composition(moduleinfo_t * module);


#endif