#ifndef _BUILDEX_MEM_REGIONS_H
#define _BUILDEX_MEM_REGIONS_H

#include <stdint.h>
#include <string>
#include <vector>

#define DIMENSIONS 3

/* type options */
#define IMAGE_INPUT			0x1
#define IMAGE_OUTPUT		0x2
#define IMAGE_INTERMEDIATE	0x3


struct mem_regions_t {

	/* image related information */
	uint32_t bytes_per_pixel;

	/* characteristics of the memory region */
	uint32_t type;							//image input, output or intermediate

	uint32_t dimensions;

	/* emulate halide's buffer_t structure here */
	uint32_t extents[DIMENSIONS];
	uint32_t strides[DIMENSIONS];
	uint32_t min[DIMENSIONS];

	uint32_t padding_filled;				//padding array filled right, left, up and down
	uint32_t padding[4];					//padding for all 4 directions 

	/* physical demarcations of the memory regions */
	uint64_t start;
	uint64_t end;

	/*name of the mem region */
	std::string name;

	/* pcs which reference this region */
	std::vector<uint32_t > pcs;

};

/* get random memory location */
std::vector<uint64_t> get_nbd_of_random_points(std::vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride);
std::vector<uint64_t> get_nbd_of_random_points_2(std::vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride);

/* extracting mem regions */
mem_regions_t * get_mem_region(uint64_t value, std::vector<mem_regions_t *> &mem_regions);

/*extracting mem locations - given the D-dimensional coordinates this retrieves the memory address */
uint64_t get_mem_location(std::vector<int> base, std::vector<int> offset, mem_regions_t * mem_region, bool * success);
/* given a memory location get the memory position in D-dimensional co-ordinates */
std::vector<int> get_mem_position(mem_regions_t * mem_region, uint64_t mem_value);

/* extracting random locations */
mem_regions_t* get_random_output_region(std::vector<mem_regions_t *> regions);
uint64_t get_random_mem_location(mem_regions_t * region, uint32_t seed);

bool is_within_mem_region(mem_regions_t* mem, uint64_t value);

/* prints out the identified mem regions */
void print_mem_regions(std::vector<mem_regions_t *> regions);


#endif