#include "extract_memregions.h"
#include "memregions.h"
#include "common_defines.h"
#include <iostream>
#include "utilities.h"

using namespace std;


/*
* following are mem layout dependant functions which convert concrete mem layout into a abstracted D-dimensional strucuture
* function naming convention -
C,R - column or row major layout
N,E - embedded or not embedded colors
1,2,3... - number of dimensions searching for
*/

/* basic photoshop image layout - invert, blur etc. */
mem_regions_t * locate_image_CN2(char * values, uint32_t size, uint64_t * start, uint64_t * end, image_t * image){

	/* we will be searching for the first line of the image */

	bool image_found = false;
	int i;
	uint32_t count = 0;

	for (i = 0; i < size; i++){

		print_progress(&count, 100000);
		

		bool success = true;
		for (int j = 0; j < image->width; j++){
			if ((values[i + j] & 0xff) != image->image_array[j]){ /* this may need to be changed */
				success = false;
				break;
			}
		}

		if (success){ /* verify the region actually has the image */
			vector<uint32_t> start_points;
			int start = 0;
			int last = 0;
			/* get the starting points of each image line */
			for (int j = i; j < size; j++){
				bool found = true;
				for (int k = 0; k < image->width; k++){
					if ((values[j + k] & 0xff) != image->image_array[last + k]){
						found = false;
						break;
					}
				}
				if (found){
					start = j;
					last += image->width;
					j += image->width - 1;
					start_points.push_back(start);
					if (last == image->width * image->height) { break; }
				}
			}
			/* if we covered the entire image */
			if (last == image->width * image->height){

				/* check whether the gaps are uniform */
				vector<uint32_t> gaps;
				for (int j = 1; j < start_points.size(); j++){
					gaps.push_back(start_points[j] - start_points[j - 1]);
				}

				if (gaps.size() == 0){
					mem_regions_t * region = new mem_regions_t();

					region->bytes_per_pixel = 1;
					region->dimensions = 2;

					region->strides[0] = 1;
					region->strides[1] = image->width;

					region->padding_filled = 1;
					region->padding[0] = 0;

					region->extents[0] = image->width;
					region->extents[1] = image->height;

					/*region start and the end */
					region->start = start_points[0];
					region->end = start_points[start_points.size() - 1] + region->strides[1];

					return region;
				}
				else{

					uint32_t found = true;
					uint32_t comp_val = gaps[0];
					for (int j = 1; j < gaps.size(); j++){
						if (comp_val != gaps[j]){
							found = false; break;
						}
					}
					if (found){
						mem_regions_t * region = new mem_regions_t();
						
						region->bytes_per_pixel = 1;
						region->dimensions = 2;
						
						region->strides[0] = 1;
						region->strides[1] = gaps[0];

						region->padding_filled = 1;
						region->padding[0] = gaps[0] - image->width;
						
						region->extents[0] = image->width;
						region->extents[1] = image->height;

						/* region start and the end */
						region->start = start_points[0];
						region->end = start_points[start_points.size() - 1] + region->strides[1];

						return region;
					}
				}
			}
		}
	}

	return NULL;

}