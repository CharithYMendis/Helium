#ifndef _BUILD_MEM_DUMP_H
#define _BUILD_MEM_DUMP_H


#include "canonicalize.h"
#include "imageparser.h"
#include <fstream>
#include "build_mem_instrace.h"

#define DIMENSIONS 3

/* layout options */
#define XY_LAYOUT		 0x1  /* row major*/
#define YX_LAYOUT		 0x2  /* column major*/
#define EMBEDDED_COLORS  0x4  /* whether colors are embedded within a pixel */

/* type options */
#define IMAGE_INPUT			0x1
#define IMAGE_OUTPUT		0x2
#define IMAGE_INTERMEDIATE	0x3

using namespace std;

struct mem_regions_t {

	/* color and other image related information */
	uint colors;
	uint bytes_per_color;
	uint stride;

	/* characteristics of the memory region */
	uint type;
	uint layout;
	uint scanline_width;
	uint padding;

	/* physical demarcations of the memory regions */
	uint64 start;
	uint64 end;
	uint64 size;

	/* width and height of the picture - these are derived values */
	uint width;
	uint height;

	/*name of the mem region */
	string name;

};

mem_regions_t * get_similar_region(ofstream &dmp, Image * image);
void get_input_output_mem_regions(ofstream &dump_in, ofstream &dump_out, ofstream &image_in, ofstream &image_out, vector<mem_regions_t *> &mem_regions);
void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions);  /*convert from mem regions identified by instrace to dump*/
void print_mem_regions(vector<mem_regions_t *> regions);
void print_mem_regions(mem_regions_t * region);

uint64 get_random_mem_location(mem_regions_t *  region, uint seed, uint trys, bool * ok);
mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions);
mem_regions_t * get_mem_region(uint64 value, vector<mem_regions_t *> &mem_regions);
uint64 get_mem_location(vector<uint> base, vector<int> offset, mem_regions_t * mem_region, bool * success);
vector<uint> get_mem_position(mem_regions_t * mem_region, uint64 mem_value);



#endif

