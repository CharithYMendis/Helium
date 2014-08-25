#ifndef _BUILD_MEM_DUMP_H
#define _BUILD_MEM_DUMP_H

#include "canonicalize.h"
#include "imageinfo.h"
#include <fstream>
#include "build_mem_instrace.h"

#define DIMENSIONS 3

/* layout options */
#define ROW_MAJOR		 0x1  /* row major*/
#define COLUMN_MAJOR	 0x2  /* column major*/

/* type options */
#define IMAGE_INPUT			0x1
#define IMAGE_OUTPUT		0x2
#define IMAGE_INTERMEDIATE	0x3

using namespace std;

struct mem_regions_t {

	/* image related information */
	uint bytes_per_pixel;  
	uint stride;						//stride in bytes from one pixel to another

	/* characteristics of the memory region */
	uint type;							//image input, output or intermediate
	uint layout;						//row, column major
	uint scanline_width;				//in bytes the total scanline size
	uint padding[4];					//padding for all 4 directions 

	/* physical demarcations of the memory regions */
	uint64 start;
	uint64 end;

	/* dimensions of the image - read from the image itself */
	uint width;
	uint height;
	uint colors;

	/*name of the mem region */
	string name;

};



/* 
*  This module can be used in two ways - the output is a more rich data structure with more information about the mem layout 
*	1. Use the instrace to build the mem info and with images and a config file for the program concerned get mem regions 
*   2. Use mem dumps to automatically figure out where the images are stored and mem layout attributes 
*/

/* use mem dump to identify the mem regions - this is independent of the instrace related mem info */
void get_input_output_mem_regions(ofstream &dump_in, ofstream &dump_out, 
	image_t * in_image, image_t * out_image, vector<mem_regions_t *> &mem_regions);
/*convert from mem regions identified by instrace to more information rich mem regions */
void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions,
	ifstream &config, image_t * in_image, image_t * out_image);  


/*
* debug 
*/
/* prints out the identified mem regions */
void print_mem_regions(vector<mem_regions_t *> regions);

/*
* information retrieval functions 
*/

/* extracting mem regions */
mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions);
mem_regions_t * get_mem_region(uint64 value, vector<mem_regions_t *> &mem_regions);

/*extracting mem locations*/
/* given the (x,y,c) coordinates this returns the memory address */
uint64 get_mem_location(vector<uint> base, vector<int> offset, mem_regions_t * mem_region, bool * success);
/* given a memory location get the memory position in (x,y,c)*/
vector<uint> get_mem_position(mem_regions_t * mem_region, uint64 mem_value);
/* gets a random mem location given in given number of trys */
uint64 get_random_mem_location(mem_regions_t *  region, uint seed, uint trys, bool * ok);


#endif

