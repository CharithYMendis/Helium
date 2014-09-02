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
		}
		else{
			mem = locate_image_CN2(file_values, results.st_size, &start, &end, in_image);
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

	/* create new mem_regions for the remaining mem_info which of type MEM_HEAP - postpone the implementation; these are intermediate nodes */
	


	/* naming the memory regions */
	int inputs = 0;
	int intermediates = 0;
	int outputs = 0;

	for (int i = 0; i < final_regions.size(); i++){
		if (final_regions[i]->type == IMAGE_INPUT){
			final_regions[i]->name = "input_" + to_string(++inputs);
		}
		else if (final_regions[i]->type == IMAGE_OUTPUT){
			final_regions[i]->name = "output_" + to_string(++outputs);
		}
		else if (final_regions[i]->type == IMAGE_INTERMEDIATE){
			final_regions[i]->name = "inter_" + to_string(++intermediates);
		}
	}


	DEBUG_PRINT((" no of mem regions after merging - %d\n", mem_regions.size()), 2);
	DEBUG_PRINT(("merge_instrace_and_dump_regions - done\n"), 2);

	return final_regions;
}

