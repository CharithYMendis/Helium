#include "build_mem_dump.h"

#include <iostream>
#include <string>
#include "defines.h"
#include <stdlib.h>
#include "utilities.h"
#include <sys\stat.h>
#include "imageinfo.h"
#include "extract_memregions.h"


vector<mem_regions_t *> get_image_regions_from_instrace(vector<mem_info_t *> &mem, ifstream &config, image_t * in_image, image_t * out_image){
	
	ASSERT_MSG(false, ("ERROR: function not implemented\n"));

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

		/* read the file into a buffer */
		struct stat results;
		char *  file_values;

		ASSERT_MSG(stat(filenames[i].c_str(), &results) == 0, ("file stat collection unsuccessful\n"));
		ifstream file(filenames[i].c_str(), ios::in | ios::binary);
		file_values = new char[results.st_size];
		file.read(file_values, results.st_size);

		/* locate the image */
		if (write){
			mem = locate_image_CN2(file_values, results.st_size, &start, &end, out_image);
			//mem = locate_image_CN2_backward_write(file_values, results.st_size, &start, &end, out_image);
		}
		else{
			mem = locate_image_CN2(file_values, results.st_size, &start, &end, in_image);
			//mem = locate_image_CN2_backward(file_values, results.st_size, &start, &end, in_image);


		}

		if (mem != NULL){
			mem->start += base_pc;
			mem->end += base_pc;
			DEBUG_PRINT(("region found(%u) - start %llx end %llx padding %u\n",write, mem->start, mem->end, mem->padding[0]), 2);
			regions.push_back(mem);
		}

		delete file_values;
		file.close();
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

/* read the config file to fill in the information  */
static void populate_mem_region_from_config(ifstream &config, mem_regions_t * region){
	/*layout, padding*/
	while (!config.eof()){
		string line;
		getline(config, line);
		if (!line.empty()){
			vector<string> splitted = split(line, '=');
			ASSERT_MSG(splitted.size() == 2, ("expected a name / value pair\n"));

			string name = splitted[0];
			string value = splitted[1];

			/* fill the configurations for this mem region */

		}
	}
}

/* this is the intersection of mem_regions */
vector<mem_regions_t *> merge_instrace_and_dump_regions(vector<mem_regions_t *> &total_regions, 
	vector<mem_info_t *> mem_info, vector<mem_regions_t *> mem_regions){

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


				if ((mem_info[j]->direction  & MEM_INPUT) == MEM_INPUT){ cout << "image input" << endl;  mem_regions[i]->type |= IMAGE_INPUT; }
				if ((mem_info[j]->direction & MEM_OUTPUT) == MEM_OUTPUT){ cout << "image output" << endl;  mem_regions[i]->type |= IMAGE_OUTPUT; }

				/* debug information about the merging */
				if (debug && debug_level >= 2){
					cout << "mem region:" << endl;
					cout << "start : " << hex << mem_regions[i]->start << " end : " << mem_regions[i]->end << endl;
					cout << dec << "strides : ";
					for (int k = 0; k < mem_regions[i]->dimensions; k++) cout << mem_regions[i]->strides[k] << ",";
					cout << endl;
					cout << "mem instrace:" << endl;
					cout << "start : " << hex << mem_info[j]->start << " end : " << mem_info[j]->end << endl;

				}


				/* ok if the memory region is completely contained in the region constructed by meminfo */
				if ( (mem_regions[i]->start >= mem_info[j]->start) && (mem_regions[i]->end <= mem_info[j]->end) ){
				
					/* how much to the left of start is the meminfo spread? */
					uint64_t start = mem_regions[i]->start;
					vector<uint32_t> left_spread;
					for (int k = mem_regions[i]->dimensions - 1; k >= 0; k--){
						uint32_t spread = (start - mem_info[j]->start) / mem_regions[i]->strides[k];
						start = mem_regions[i]->start - spread * mem_regions[i]->strides[k];
						left_spread.push_back(spread);
					}

					/* printing out the spreads */
					cout << dec << "left spread: "; 
					for (int k = 0; k < left_spread.size(); k++){
						cout << left_spread[k] << ",";
					}
					cout << endl;

					/* how much to the right of the end of the meminfo are we spread? */
					uint64_t end = mem_regions[i]->end;
					vector<uint32_t> right_spread;
					for (int k = mem_regions[i]->dimensions - 1; k >= 0; k--){
						uint32_t spread = (mem_info[j]->end - end) / mem_regions[i]->strides[k];
						end = mem_regions[i]->end + spread * mem_regions[i]->strides[k];
						right_spread.push_back(spread);
					}

					/* printing out the spreads */
					cout << "right spread: ";
					for (int k = 0; k < right_spread.size(); k++){
						cout << right_spread[k] << ",";
					}
					cout << endl;

					cout << "--------------------------" << endl;

					mem_regions[i]->start = mem_info[j]->start;
					mem_regions[i]->end = mem_info[j]->end;

				}

				final_regions.push_back(mem_regions[i]);
				total_regions.push_back(mem_regions[i]);
				break;

			}
		}
	}

	/* create new mem_regions for the remaining mem_info which of type MEM_HEAP - postpone the implementation; these are intermediate nodes */
	for (int i = 0; i < mem_info.size(); i++){
		if (merged_mem_info[i] == false){ /* if not merged */
			if (mem_info[i]->type = MEM_HEAP_TYPE){
				mem_regions_t * mem = new mem_regions_t;

				mem->start = mem_info[i]->start;
				mem->end = mem_info[i]->end;
				mem->dimensions = get_number_dimensions(mem_info[i]); /* we don't know the dimensions of this yet */
				mem->bytes_per_pixel = mem_info[i]->prob_stride;
				for (int j = 1; j <= mem->dimensions; j++){
					mem->strides[j - 1] = get_stride(mem_info[i], j, mem->dimensions);
					mem->extents[j - 1] = get_extents(mem_info[i], j, mem->dimensions);
				}
				//mem->strides[0] = mem_info[i]->prob_stride;
				//mem->extents[0] = (mem->end - mem->start + 1)/ mem->strides[0];
				mem->padding_filled = 0;
				
				mem->type = 0;
				if ((mem_info[i]->direction  & MEM_INPUT) == MEM_INPUT){ mem->type |= IMAGE_INPUT; }
				if ((mem_info[i]->direction & MEM_OUTPUT) == MEM_OUTPUT){ mem->type |= IMAGE_OUTPUT; }

				total_regions.push_back(mem);
				
			}
		}
	}


	/* naming the memory regions */
	int inputs = 0;
	int intermediates = 0;
	int outputs = 0;

	for (int i = 0; i < total_regions.size(); i++){
		if (total_regions[i]->type == IMAGE_INPUT){
			total_regions[i]->name = "input_" + to_string(++inputs);
		}
		else if (total_regions[i]->type == IMAGE_OUTPUT){
			total_regions[i]->name = "output_" + to_string(++outputs);
		}
		else if (total_regions[i]->type == IMAGE_INTERMEDIATE){
			total_regions[i]->name = "inter_" + to_string(++intermediates);
		}
	}


	DEBUG_PRINT((" no of image mem regions after merging - %d\n", final_regions.size()), 2);
	DEBUG_PRINT((" total number of mem regions (from instrace) - %d\n", total_regions.size()), 2);

	DEBUG_PRINT((" merge_instrace_and_dump_regions - done\n"), 2);

	return final_regions;
}

