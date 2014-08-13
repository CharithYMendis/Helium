#include "build_mem_dump.h"

#include <iostream>
#include <string>
#include "defines.h"
#include <stdlib.h>
#include "utilities.h"

void get_input_output_mem_regions(ofstream &dump_in, ofstream &dump_out,
	image_t * in_image, image_t * out_image, vector<mem_regions_t *> &mem_regions){

}

/* read the config file to fill in the information  */
void populate_mem_region_from_config(ifstream &config, mem_regions_t * region){
	/*layout, padding*/
	while (!config.eof()){
		string line;
		getline(config, line);
		if (!line.empty()){
			vector<string> splitted = split(line, '=');
			ASSERT_MSG(splitted.size() == 2, ("expected a name / value pair\n"));

			string name = splitted[0];
			string value = splitted[1];

			/* now process the string */
			if (name.compare("layout") == 0){
				if (value.compare("col-major") == 0) region->layout = COLUMN_MAJOR;
				else if (value.compare("row-major") == 0) region->layout = ROW_MAJOR;
			}
			else if (name.compare("padding") == 0){
				region->padding = atoi(value.c_str());
			}

		}
	}
}


void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions,
	ifstream &config, image_t * in_image, image_t * out_image){

	uint inputs = 0;
	uint outputs = 0;
	uint intermediates = 0;


	for (int i = 0; i < mem.size(); i++){
		if (mem[i]->type == MEM_HEAP_TYPE){

			mem_regions_t * region = new mem_regions_t();
			if (mem[i]->direction == MEM_INPUT){
				region->type = IMAGE_INPUT;
				region->name = "input_" + to_string(++inputs);
			}
			else if (mem[i]->direction == MEM_OUTPUT){
				region->type = IMAGE_OUTPUT;
				region->name = "output_" + to_string(++outputs);
			}
			else{
				region->type = IMAGE_INTERMEDIATE;
				region->name = "inter_" + to_string(++intermediates);
			}

			region->start = mem[i]->start;
			region->end = mem[i]->end;
			region->stride = mem[i]->prob_stride;
			region->size = (mem[i]->end - mem[i]->start) / region->stride;

			/* select the image that matches this mem */



			/* get non volatile (relatively) values from the config file */
			populate_mem_region_from_config(config,region);


#ifdef DP_KERNAL
			region->colors = 1;
			region->bytes_per_color = 4;
			region->layout = XY_LAYOUT;
			region->padding = 0;
			region->width = 10;
			region->height = 10;
#endif
#ifdef BLUR_KERNAL
			region->colors = 3;
			region->bytes_per_color = 1;
			region->layout = XY_LAYOUT;
			region->padding = 0;
			region->width = 30;
			region->height = 30;
#endif


			if (region->layout & XY_LAYOUT == XY_LAYOUT){
				region->scanline_width = region->colors * region->bytes_per_color * region->width + region->padding;
			}
			else if (region->layout & YX_LAYOUT == YX_LAYOUT){
				region->scanline_width = region->colors * region->bytes_per_color * region->height + region->padding;
			}

			mem_regions.push_back(region);

		}
	}


}

void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions){

	uint inputs = 0;
	uint outputs = 0;
	uint intermediates = 0;


	for (int i = 0; i < mem.size(); i++){
		if (mem[i]->type == MEM_HEAP_TYPE){
			
			mem_regions_t * region = new mem_regions_t();
			if (mem[i]->direction == MEM_INPUT){
				region->type = IMAGE_INPUT;
				region->name = "input_" + to_string(++inputs);
			}
			else if (mem[i]->direction == MEM_OUTPUT){
				region->type = IMAGE_OUTPUT;
				region->name = "output_" + to_string(++outputs);
			}
			else{
				region->type = IMAGE_INTERMEDIATE;
				region->name = "inter_" + to_string(++intermediates);
			}

			region->start = mem[i]->start;
			region->end = mem[i]->end;
			region->stride = get_most_probable_stride(mem[i]->stride_freqs);
			region->size = (mem[i]->end - mem[i]->start) / region->stride;

			

			/* hardcoded values which should be read from the image */
			
#ifdef DP_KERNAL
			region->colors = 1;
			region->bytes_per_color = 4;
			region->layout = XY_LAYOUT;
			region->padding = 0;
			region->width = 10; 
			region->height = 10; 
#endif
#ifdef BLUR_KERNAL
			region->colors = 3;
			region->bytes_per_color = 1;
			region->layout = XY_LAYOUT;
			region->padding = 0;
			region->width = 30;
			region->height = 30;
#endif


			if (region->layout & XY_LAYOUT == XY_LAYOUT){
				region->scanline_width = region->colors * region->bytes_per_color * region->width + region->padding;
			}
			else if (region->layout & YX_LAYOUT == YX_LAYOUT){
				region->scanline_width = region->colors * region->bytes_per_color * region->height + region->padding;
			}

			mem_regions.push_back(region); 

		}
	}
}

void print_mem_regions(vector<mem_regions_t *> regions){
	
	cout << "------------------------------MEM REGIONS ----------------------------" << endl;

	for (int i = 0; i < regions.size(); i++){
		print_mem_regions(regions[i]);
	}

}

void print_mem_regions(mem_regions_t * region){

	cout << "colors = " << region->colors << endl;
	cout << "bytes per color = " << region->bytes_per_color << endl;


	cout << "start = " << region->start << endl;
	cout << "end = " << region->end << endl;

	cout << "stride = " << region->stride << endl;
	cout << "size = " << region->size << endl;

	cout << "type = ";
	switch (region->type){
	case IMAGE_INPUT:  cout << "image input" << endl; break;
	case IMAGE_OUTPUT: cout << "image output" << endl; break;
	case IMAGE_INTERMEDIATE: cout << "image intermediate" << endl; break;
	}

	cout << "layout = ";
	if (region->layout & XY_LAYOUT == XY_LAYOUT){
		cout << "row major" << endl;
	}
	else if (region->layout & YX_LAYOUT == YX_LAYOUT){
		cout << "column major" << endl;
	}
	else if (region->layout & EMBEDDED_COLORS == EMBEDDED_COLORS){
		cout << "colors embedded" << endl;
	}

	cout << "scanline width = " << region->scanline_width << endl;
	cout << "padding = " << region->padding << endl;
	cout << "width = " << region->width << endl;
	cout << "height = " << region->height << endl;

	cout << "name = " << region->name << endl;

	cout << "-------------------------------------------------------------------" << endl;


}


mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions){

	DEBUG_PRINT(("selecting a random output region now.......\n"), 1);

	/*get the number of intermediate and output regions*/
	uint no_regions = 0;
	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->type == IMAGE_INTERMEDIATE || regions[i]->type == IMAGE_OUTPUT){
			no_regions++;
		}
	}


	uint random = rand() % no_regions;
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

uint64 get_random_mem_location(mem_regions_t *  region, uint seed, uint trys, bool * ok){

	DEBUG_PRINT(("selecting a random output location now.....\n"), 1);

	while (trys--){

		srand(seed);

		uint * mem;

		/*generate a random number*/
		if (region->layout & XY_LAYOUT == XY_LAYOUT){
			uint mem_array[] = { rand() % region->height, rand() % region->width, rand() % region->colors };
			mem = mem_array;
		}
		else if (region->layout & YX_LAYOUT == YX_LAYOUT){
			uint mem_array[] = { rand() % region->width, rand() % region->height, rand() % region->colors };
			mem = mem_array;
		}

		vector<uint> base(mem, mem + DIMENSIONS);
		vector<int> offset(DIMENSIONS, 0);
		bool success;

		uint64 mem_location = get_mem_location(base, offset, region, &success);

		if (success){
			*ok = true;
			return mem_location;
		}
	}

	*ok = false;
	return 0;
}


uint64 get_mem_location(vector<uint> base, vector<int> offset, mem_regions_t * mem_region, bool * success){

	ASSERT_MSG((base.size() == DIMENSIONS), ("ERROR: there should be three dimensions\n"));

	for (int i = 0; i < base.size(); i++){
		base[i] += offset[i];
	}

	if (mem_region->layout & XY_LAYOUT == XY_LAYOUT){
		DEBUG_PRINT(("get_mem_location - height - %d / width - %d / color -  %d\n", base[0], base[1], base[2]), 3);
		if ((base[0] >= mem_region->height) || (base[1] >= mem_region->width) || (base[2] >= mem_region->colors)){
			*success = false;
		}
		else{
			*success = true;
		}
	}
	else if (mem_region->layout & YX_LAYOUT == YX_LAYOUT){
		DEBUG_PRINT(("get_mem_location - width - %d / width - %d / color -  %d\n", base[0], base[1], base[2]), 3);
		if ((base[0] >= mem_region->width) || (base[1] >= mem_region->height) || (base[2] >= mem_region->colors)){
			*success = false;
		}
		else{
			*success = true;
		}
	}

	

	return mem_region->start + (base[0] * mem_region->scanline_width) + ( (base[1] * mem_region->colors) + base[2] ) * mem_region->bytes_per_color;

}

vector<uint> get_mem_position(mem_regions_t * mem_region, uint64 mem_value){

	vector<uint> dims;

	/* dimensions would always be row, col, color etc (0 based) */

	/*get the row */
	uint64 offset = mem_value - mem_region->start;
	uint row = offset / mem_region->scanline_width;

	/*get the column*/
	uint64 col_offset = offset - row * mem_region->scanline_width;
	uint column = col_offset / (mem_region->colors  * mem_region->bytes_per_color);
	
	/*get the color for this pixel*/
	uint color_offset = col_offset - column * (mem_region->colors  * mem_region->bytes_per_color);
	uint color = color_offset / mem_region->bytes_per_color;

	DEBUG_PRINT(("get_mem_position - row - %d, col - %d, color - %d\n", row, column, color), 3);
	
	if (mem_region->layout == XY_LAYOUT){
		ASSERT_MSG((mem_region->height > row), ("ERROR: This memory location is out of bounds\n"));
		ASSERT_MSG((mem_region->width > column), ("ERROR: This memory location is out of bounds\n"));
		ASSERT_MSG((mem_region->colors > color), ("ERROR: This memory location is out of bounds\n"));
		
	}
	else if (mem_region->layout == YX_LAYOUT) {
		ASSERT_MSG((mem_region->width > row), ("ERROR: This memory location is out of bounds\n"));
		ASSERT_MSG((mem_region->height > column), ("ERROR: This memory location is out of bounds\n"));
		ASSERT_MSG((mem_region->colors > color), ("ERROR: This memory location is out of bounds\n"));
	}

	dims.push_back(row);
	dims.push_back(column);
	dims.push_back(color); /* will be 0 for only one color */

	return dims;


}

mem_regions_t * get_mem_region(uint64 value, vector<mem_regions_t *> &mem_regions){

	for (int i = 0; i < mem_regions.size(); i++){
		if ((mem_regions[i]->start <= value) && (mem_regions[i]->end >= value)){
			return mem_regions[i];
		}
	}

	return NULL;

}