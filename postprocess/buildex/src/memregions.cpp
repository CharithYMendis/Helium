#include "memregions.h"
#include <iostream>
#include <string>
#include "common_defines.h"

using namespace std;

/* extracting information from the mem regions */

mem_regions_t * get_mem_region(uint64_t value, vector<mem_regions_t *> &mem_regions){

	for (int i = 0; i < mem_regions.size(); i++){
		if ((mem_regions[i]->start <= value) && (mem_regions[i]->end >= value)){
			return mem_regions[i];
		}
	}

	return NULL;

}

uint64_t get_mem_location(vector<int> base, vector<int> offset, mem_regions_t * mem_region, bool * success){

	ASSERT_MSG((base.size() == mem_region->dimensions), ("ERROR: dimensions dont match up\n"));

	for (int i = 0; i < base.size(); i++){
		base[i] += offset[i];
	}

	for (int i = 0; i < base.size(); i++){
		if (base[i] >= mem_region->extents[i]){
			*success = false;
			return 0;
		}
	}

	*success = true;

	uint64_t ret_addr = mem_region->start;

	for (int i = 0; i < base.size(); i++){
		ret_addr += mem_region->strides[i] * base[i];
	}

	return ret_addr;


}

vector<int> get_mem_position(mem_regions_t * mem_region, uint64_t mem_value){

	vector<int> pos;
	vector<int> r_pos;

	/* dimensions would always be width dir(x), height dir(y) */

	/*get the row */
	uint64_t offset = mem_value - mem_region->start;

	for (int i = mem_region->dimensions - 1; i >= 0; i--){
		int point_offset = offset / mem_region->strides[i];
		if (point_offset >= mem_region->extents[i]){ point_offset = -1; }
		r_pos.push_back(point_offset);
		offset -= point_offset * mem_region->strides[i];
	}

	pos.assign(r_pos.rbegin(), r_pos.rend());

	return pos;

}



/* random mem locations */
mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions){

	DEBUG_PRINT(("selecting a random output region now.......\n"), 2);

	/*get the number of intermediate and output regions*/
	uint32_t no_regions = 0;
	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->type == IMAGE_INTERMEDIATE || regions[i]->type == IMAGE_OUTPUT){
			no_regions++;
		}
	}


	uint32_t random = rand() % no_regions;
	no_regions = 0;

	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->type == IMAGE_INTERMEDIATE || regions[i]->type == IMAGE_OUTPUT){
			if (no_regions == random){
				DEBUG_PRINT(("random output region seleted\n"), 1);
				return regions[i];
			}
			no_regions++;
		}
	}

	return NULL; /*should not reach this point*/


}

uint64_t get_random_mem_location(mem_regions_t *  region, uint32_t seed){

	DEBUG_PRINT(("selecting a random output location now.....\n"), 2);

	srand(seed);

	uint32_t random_num = abs(rand());
	
	vector<int> base;
	vector<int> offset(region->dimensions, 0);

	for (int i = 0; i < region->dimensions; i++){
		base.push_back(random_num % region->extents[i]);
	}

	bool success;
	uint64_t mem_location = get_mem_location(base, offset, region, &success);

	ASSERT_MSG(success, ("ERROR: random memory location out of bounds\n"));

	return mem_location;
}



/* printing functions - debug */

void print_mem_regions(mem_regions_t * region){

	cout << "bytes per color = " << region->bytes_per_pixel << endl;


	cout << "start = " << region->start << endl;
	cout << "end = " << region->end << endl;


	cout << "type = ";
	switch (region->type){
	case IMAGE_INPUT:  cout << "image input" << endl; break;
	case IMAGE_OUTPUT: cout << "image output" << endl; break;
	case IMAGE_INTERMEDIATE: cout << "image intermediate" << endl; break;
	}


	cout << "extents : " << endl;
	for (int i = 0; i < region->dimensions; i++){
		cout << region->extents[i] << endl;
	}

	cout << "strides : " << endl;
	for (int i = 0; i < region->dimensions; i++){
		cout << region->strides[i] << endl;
	}

	cout << "padding : " << endl;
	for (int i = 0; i < region->padding_filled; i++){
		cout << region->padding[i] << endl;
	}

	cout << "name = " << region->name << endl;

	cout << "-------------------------------------------------------------------" << endl;


}

void print_mem_regions(vector<mem_regions_t *> regions){

	cout << "------------------------------MEM REGIONS ----------------------------" << endl;

	for (int i = 0; i < regions.size(); i++){
		print_mem_regions(regions[i]);
	}

}


