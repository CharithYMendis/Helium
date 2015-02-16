#include <iostream>

#include "defines.h"
#include "common_defines.h"
#include "utilities.h"
#include "memory/memanalysis.h"


using namespace std;

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
			if (is_overlapped(mem_regions[i]->start, mem_regions[i]->end, mem_info[j]->start, mem_info[j]->end)){

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
				if ((mem_regions[i]->start >= mem_info[j]->start) && (mem_regions[i]->end <= mem_info[j]->end)){

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

vector<mem_regions_t *> get_image_regions_from_instrace(vector<mem_info_t *> &mem, ifstream &config, image_t * in_image, image_t * out_image){

	ASSERT_MSG(false, ("ERROR: function not implemented\n"));

}