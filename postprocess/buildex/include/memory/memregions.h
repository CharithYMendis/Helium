#ifndef _BUILDEX_MEM_REGIONS_H
#define _BUILDEX_MEM_REGIONS_H

#include <stdint.h>
#include <string>
#include <vector>
#include "meminfo.h"
#include "analysis\staticinfo.h"
#include "analysis\x86_analysis.h"

#define DIMENSIONS 3

#define INPUT_BUFFER			0x1
#define OUTPUT_BUFFER			0x2
#define INTERMEDIATE_BUFFER		0x4


struct mem_regions_t {

	/* image related information */
	uint32_t bytes_per_pixel;

	/* characteristics of the memory region */

	/* this gives if it is mem read/write or both for a particular buffer */
	uint32_t direction;							
	
	/* this gives the memory region type based on dependancy analysis */
	uint32_t type;

	/* type of the memory region based on dump */
	uint32_t dump_type;


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

	mem_regions_t(){
		type = 0;
		direction = 0;
		dump_type = 0;
	}

};

/* updating mem_region properties */
void mark_possible_buffers(std::vector<pc_mem_region_t *> &pc_mem, std::vector<mem_regions_t *> &mem_regions, std::vector<Static_Info *> &info, vec_cinstr &instrs);
void remove_possible_stack_frames(std::vector<pc_mem_region_t *> &pc_mem, std::vector<mem_info_t *> &mem, std::vector<Static_Info *> &info, vec_cinstr &instrs);


/* extracting random locations, memregions */
std::vector<uint64_t> get_nbd_of_random_points(std::vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride);
std::vector<uint64_t> get_nbd_of_random_points_2(std::vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride);
mem_regions_t* get_random_output_region(std::vector<mem_regions_t *> regions);
uint64_t get_random_mem_location(mem_regions_t * region, uint32_t seed);

/* extracting exact locations, memregions */
mem_regions_t * get_mem_region(uint64_t value, std::vector<mem_regions_t *> &mem_regions); /* extracting mem regions */
uint64_t get_mem_location(std::vector<int> base, std::vector<int> offset, mem_regions_t * mem_region, bool * success); /*extracting mem locations - given the D-dimensional coordinates this retrieves the memory address */
std::vector<int> get_mem_position(mem_regions_t * mem_region, uint64_t mem_value); /* given a memory location get the memory position in D-dimensional co-ordinates */
bool is_within_mem_region(mem_regions_t* mem, uint64_t value);

/* prints out the identified mem regions */
void print_mem_regions(std::vector<mem_regions_t *> regions);
void print_mem_regions(mem_regions_t * region);


#endif