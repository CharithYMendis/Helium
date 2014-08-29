#include "build_mem_dump.h"

#include <iostream>
#include <string>
#include "defines.h"
#include <stdlib.h>
#include "utilities.h"
#include <sys\stat.h>
#include "imageinfo.h"

/* BIG ASSUMPTION - we assume that every mem_region is of one color */

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
				//region->padding = atoi(value.c_str());
			}

		}
	}
}


void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions,
	ifstream &config, image_t * in_image, image_t * out_image){

//	uint inputs = 0;
//	uint outputs = 0;
//	uint intermediates = 0;
//
//
//	for (int i = 0; i < mem.size(); i++){
//		if (mem[i]->type == MEM_HEAP_TYPE){
//
//			mem_regions_t * region = new mem_regions_t();
//			if (mem[i]->direction == MEM_INPUT){
//				region->type = IMAGE_INPUT;
//				region->name = "input_" + to_string(++inputs);
//			}
//			else if (mem[i]->direction == MEM_OUTPUT){
//				region->type = IMAGE_OUTPUT;
//				region->name = "output_" + to_string(++outputs);
//			}
//			else{
//				region->type = IMAGE_INTERMEDIATE;
//				region->name = "inter_" + to_string(++intermediates);
//			}
//
//			region->start = mem[i]->start;
//			region->end = mem[i]->end;
//			region->stride = mem[i]->prob_stride;
//			region->size = (mem[i]->end - mem[i]->start) / region->stride;
//
//			/* select the image that matches this mem */
//
//			/* select the image with the lowest percentage error and assign width and height */
//			double out_size = out_image->height * out_image->width;
//			double in_size = in_image->width * in_image->height;
//
//			double err_in = (abs((double)region->size - in_size) / (double)region->size);
//			double err_out = (abs((double)region->size - out_size) / (double)region->size);
//
//			if (err_in > 3 && err_out > 3){ //300% error
//				delete region;
//				continue;
//			}
//			else{
//				if (region->type == IMAGE_INPUT){
//					region->width = in_image->width;
//					region->height = in_image->height;
//				}
//				else if(region->type == IMAGE_OUTPUT){
//					region->width = out_image->width;
//					region->height = out_image->height;
//				}
//				else{
//					if (err_in < err_out){
//						region->width = in_image->width;
//						region->height = in_image->height;
//					}
//					else{
//						region->width = out_image->width;
//						region->height = out_image->height;
//					}
//				}
//			
//			}
//
//
//			/* get non volatile (relatively) values from the config file */
//			populate_mem_region_from_config(config,region);
//
//
//#ifdef DP_KERNAL
//			region->colors = 1;
//			region->bytes_per_color = 4;
//			region->layout = XY_LAYOUT;
//			region->padding = 0;
//			region->width = 10;
//			region->height = 10;
//#endif
//#ifdef BLUR_KERNAL
//			region->colors = 3;
//			region->bytes_per_color = 1;
//			region->layout = XY_LAYOUT;
//			region->padding = 0;
//			region->width = 30;
//			region->height = 30;
//#endif
//
//
//			if (region->layout & XY_LAYOUT == XY_LAYOUT){
//				region->scanline_width = region->colors * region->bytes_per_color * region->width + region->padding;
//			}
//			else if (region->layout & YX_LAYOUT == YX_LAYOUT){
//				region->scanline_width = region->colors * region->bytes_per_color * region->height + region->padding;
//			}
//
//			mem_regions.push_back(region);
//
//		}
//	}
//

}

void get_input_output_mem_regions(vector<mem_info_t *> &mem, vector<mem_regions_t *> &mem_regions){

//	uint inputs = 0;
//	uint outputs = 0;
//	uint intermediates = 0;
//
//
//	for (int i = 0; i < mem.size(); i++){
//		if (mem[i]->type == MEM_HEAP_TYPE){
//			
//			mem_regions_t * region = new mem_regions_t();
//			if (mem[i]->direction == MEM_INPUT){
//				region->type = IMAGE_INPUT;
//				region->name = "input_" + to_string(++inputs);
//			}
//			else if (mem[i]->direction == MEM_OUTPUT){
//				region->type = IMAGE_OUTPUT;
//				region->name = "output_" + to_string(++outputs);
//			}
//			else{
//				region->type = IMAGE_INTERMEDIATE;
//				region->name = "inter_" + to_string(++intermediates);
//			}
//
//			region->start = mem[i]->start;
//			region->end = mem[i]->end;
//			region->stride = get_most_probable_stride(mem[i]->stride_freqs);
//			region->size = (mem[i]->end - mem[i]->start) / region->stride;
//
//			
//
//			/* hardcoded values which should be read from the image */
//			
//#ifdef DP_KERNAL
//			region->colors = 1;
//			region->bytes_per_color = 4;
//			region->layout = XY_LAYOUT;
//			region->padding = 0;
//			region->width = 10; 
//			region->height = 10; 
//#endif
//#ifdef BLUR_KERNAL
//			region->colors = 3;
//			region->bytes_per_color = 1;
//			region->layout = XY_LAYOUT;
//			region->padding = 0;
//			region->width = 30;
//			region->height = 30;
//#endif
//
//
//			if (region->layout & XY_LAYOUT == XY_LAYOUT){
//				region->scanline_width = region->colors * region->bytes_per_color * region->width + region->padding;
//			}
//			else if (region->layout & YX_LAYOUT == YX_LAYOUT){
//				region->scanline_width = region->colors * region->bytes_per_color * region->height + region->padding;
//			}
//
//			mem_regions.push_back(region); 
//
//		}
//	}
}

void print_mem_regions(vector<mem_regions_t *> regions){
	
	cout << "------------------------------MEM REGIONS ----------------------------" << endl;

	for (int i = 0; i < regions.size(); i++){
		//print_mem_regions(regions[i]);
	}

}

void print_mem_regions(mem_regions_t * region){

	cout << "colors = " << region->colors << endl;
	cout << "bytes per color = " << region->bytes_per_pixel << endl;


	cout << "start = " << region->start << endl;
	cout << "end = " << region->end << endl;

	cout << "stride = " << region->stride << endl;

	cout << "type = ";
	switch (region->type){
	case IMAGE_INPUT:  cout << "image input" << endl; break;
	case IMAGE_OUTPUT: cout << "image output" << endl; break;
	case IMAGE_INTERMEDIATE: cout << "image intermediate" << endl; break;
	}

	/*cout << "layout = ";
	if (region->layout & XY_LAYOUT == XY_LAYOUT){
		cout << "row major" << endl;
	}
	else if (region->layout & YX_LAYOUT == YX_LAYOUT){
		cout << "column major" << endl;
	}
	else if (region->layout & EMBEDDED_COLORS == EMBEDDED_COLORS){
		cout << "colors embedded" << endl;
	}*/

	cout << "scanline width = " << region->scanline_width << endl;
	cout << "padding = " << region->padding << endl;
	cout << "width = " << region->width << endl;
	cout << "height = " << region->height << endl;

	cout << "name = " << region->name << endl;

	cout << "-------------------------------------------------------------------" << endl;


}




uint64 get_mem_location(vector<uint> base, vector<int> offset, mem_regions_t * mem_region, bool * success){

	ASSERT_MSG((base.size() == DIMENSIONS), ("ERROR: there should be three dimensions\n"));

	for (int i = 0; i < base.size(); i++){
		base[i] += offset[i];
	}

	/*if (mem_region->layout & XY_LAYOUT == XY_LAYOUT){
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
	}*/

	

	return mem_region->start + (base[0] * mem_region->scanline_width) + ( (base[1] * mem_region->colors) + base[2] ) * mem_region->bytes_per_pixel;

}

/* one color assumption and column major assumption */
vector<uint> get_mem_position(mem_regions_t * mem_region, uint64 mem_value){

	vector<uint> dims;

	/* dimensions would always be width dir(x), height dir(y) */

	/*get the row */
	uint64 offset = mem_value - mem_region->start;
	int row = offset / mem_region->scanline_width;

	/*get the column*/
	uint64 col_offset = offset - row * mem_region->scanline_width;
	int column = col_offset / (mem_region->bytes_per_pixel);

	if (column >= mem_region->width) { column = -1; }
	//
	

	//DEBUG_PRINT(("get_mem_position - row - %d, col - %d, color - %d\n", row, column, color), 3);
	//
	//if (mem_region->layout == XY_LAYOUT){
	//	ASSERT_MSG((mem_region->height > row), ("ERROR: This memory location is out of bounds\n"));
	//	ASSERT_MSG((mem_region->width > column), ("ERROR: This memory location is out of bounds\n"));
	//	ASSERT_MSG((mem_region->colors > color), ("ERROR: This memory location is out of bounds\n"));
	//	
	//}
	//else if (mem_region->layout == YX_LAYOUT) {
	//	ASSERT_MSG((mem_region->width > row), ("ERROR: This memory location is out of bounds\n"));
	//	ASSERT_MSG((mem_region->height > column), ("ERROR: This memory location is out of bounds\n"));
	//	ASSERT_MSG((mem_region->colors > color), ("ERROR: This memory location is out of bounds\n"));
	//}

	//dims.push_back(row);
	//dims.push_back(column);
	//dims.push_back(color); /* will be 0 for only one color */

	return dims;


}

/* above functions need to be corrected */

/* one color assumption and column major assumption  - also assuming a paritcular mem layout */
uint64 get_random_mem_location(mem_regions_t *  region, uint seed){

	DEBUG_PRINT(("selecting a random output location now.....\n"), 2);

	srand(seed);

	uint32_t random_num = abs(rand());
	uint32_t x = random_num % region->width;
	uint32_t y = random_num % region->height;

	uint64_t mem_location = region->start + y * region->scanline_width + x;

	return mem_location;
}

mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions){

	DEBUG_PRINT(("selecting a random output region now.......\n"), 2);

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


mem_regions_t * get_mem_region(uint64 value, vector<mem_regions_t *> &mem_regions){

	for (int i = 0; i < mem_regions.size(); i++){
		if ((mem_regions[i]->start <= value) && (mem_regions[i]->end >= value)){
			return mem_regions[i];
		}
	}

	return NULL;

}

/* new functions */

/* we will need to customize for various image layouts in future (or we need to learn the image layouts - currently covers a specific layout) */
static mem_regions_t * locate_image(const char * filename, uint64_t * start, uint64_t * end, image_t * image){

	/* we will be searching for the first line of the image */
	struct stat results;

	ASSERT_MSG(stat(filename, &results) == 0, ("file stat collection unsuccessful\n"));

	char *  file_values;
	ifstream file(filename, ios::in | ios::binary);

	file_values = new char[results.st_size];
	file.read(file_values, results.st_size);

	bool image_found = false;
	int i;

	for (i = 0; i < results.st_size; i++){

		bool success = true;
		for (int j = 0; j < image->width; j++){
			if ( (file_values[i + j] & 0xff) != image->image_array[j]){ /* this may need to be changed */
				success = false;
				break;
			}
		}

		if (success){ /* verify the region actually has the image */
			vector<uint32_t> start_points;
			int start = 0;
			int last = 0;
			/* get the starting points of each image line */
			for (int j = i; j < results.st_size; j++){
				bool found = true;
				for (int k = 0; k < image->width; k++){
					if ( (file_values[j + k] & 0xff) != image->image_array[last + k]){
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
					region->scanline_width = image->width;
					region->padding[0] = 0;
					region->width = image->width;
					region->height = image->height;
					/*region start and the end */
					region->start = start_points[0];
					region->end = start_points[start_points.size() - 1] + region->scanline_width;

					return region;
				}
				else{
					uint found = true;
					uint comp_val = gaps[0];
					for (int j = 1; j < gaps.size(); j++){
						if (comp_val != gaps[j]){
							found = false; break;
						}
					}
					if (found){
						mem_regions_t * region = new mem_regions_t();
						region->scanline_width = gaps[0];
						region->padding[0] = gaps[0] - image->width;
						region->width = image->width;
						region->height = image->height;
						/* region start and the end */
						region->start = start_points[0];
						region->end = start_points[start_points.size() - 1] + region->scanline_width;
						return region;
					}
				}
			}
		}
	}

	return NULL;

}

vector<mem_regions_t *> get_image_regions_from_dump(vector<string> filenames, string in_image_filename, string out_image_filename){

	DEBUG_PRINT(("get_image_regions_from_dump....\n"), 2);

	image_t * in_image = populate_imageinfo(open_image(in_image_filename.c_str()));
	image_t * out_image = populate_imageinfo(open_image(out_image_filename.c_str()));
	vector<mem_regions_t *> regions;
	
	for (int i = 0; i < filenames.size(); i++){

		DEBUG_PRINT(("analyzing file - %s\n", filenames[i].c_str()), 2);

		vector<string> parts = split(filenames[i], '_');
		uint32_t index = parts.size() - 2;
		bool write = parts[index][0] - '0';
		uint32_t size = strtoull(parts[index - 1].c_str(), NULL, 10);
		uint64_t base_pc = strtoull(parts[index - 2].c_str(), NULL, 16);

		DEBUG_PRINT(("analyzing - base_pc %llx, size %x, write %d\n", base_pc, size, write), 2);

		uint64_t start;
		uint64_t end;

		mem_regions_t * mem = NULL;

		if (write){
			mem = locate_image(filenames[i].c_str(), &start, &end, out_image);
		}
		else{
			mem = locate_image(filenames[i].c_str(), &start, &end, in_image);
		}

		if (mem != NULL){
			mem->start += base_pc;
			mem->end += base_pc;
			DEBUG_PRINT(("region found(%u) - start %llx end %llx padding %u\n",write, mem->start, mem->end, mem->padding[0]), 2);
			regions.push_back(mem);
		}
	}

	/* remove duplicates */
	for (int i = 0; i < regions.size(); i++){
		for (int j = 0; j < i; j++){
			mem_regions_t * candidate = regions[j];
			mem_regions_t * check = regions[i];
			if (check->start == candidate->start && check->end == candidate->end){
				regions.erase(regions.begin() + i--);
				break;
			}
		}
	}

	DEBUG_PRINT(("no of mem regions found - %d\n", regions.size()), 2);
	DEBUG_PRINT(("get_image_regions_from_dump - done\n"), 2);

	return regions;

}

static mem_regions_t * create_region_from_info(mem_info_t * info){

}

/* this is the intersection of mem_regions */
vector<mem_regions_t *> merge_instrace_and_dump_regions(vector<mem_info_t *> mem_info, vector<mem_regions_t *> mem_regions){

	vector<mem_regions_t *> final_regions;

	DEBUG_PRINT(("merge_instrace_and_dump_regions...\n"), 2);

	bool * merged_mem_info = new bool[mem_info.size()];
	for (int i = 0; i < mem_info.size(); i++){
		merged_mem_info[i] = false;
	}

	/* mem regions are from dumps and check whether they overlap with instrace regions */
	for (int i = 0; i < mem_regions.size(); i++){
		for (int j = 0; j < mem_info.size(); j++){
			if (is_overlapped(mem_regions[i]->start,mem_regions[i]->end,mem_info[j]->start,mem_info[j]->end)){

				merged_mem_info[j] = true;
				mem_regions[i]->type = 0;
				if ( (mem_info[j]->direction  & MEM_INPUT) == MEM_INPUT){mem_regions[i]->type |= IMAGE_INPUT;}
				if ( (mem_info[j]->direction & MEM_OUTPUT) == MEM_OUTPUT){mem_regions[i]->type |= IMAGE_OUTPUT;}
				final_regions.push_back(mem_regions[i]);
				break;

			}
		}
	}

	/* create new mem_regions for the remaining mem_info which of type MEM_HEAP - postpone the implementation */
	

	DEBUG_PRINT((" no of mem regions after merging - %d\n", mem_regions.size()), 2);
	DEBUG_PRINT(("merge_instrace_and_dump_regions - done\n"), 2);

	return final_regions;
}

