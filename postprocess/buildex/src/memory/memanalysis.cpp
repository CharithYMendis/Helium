#include <iostream>

#include "utility/defines.h"
#include "common_defines.h"
#include "utilities.h"
#include "memory/memanalysis.h"

#include "meminfo.h"


using namespace std;

mem_info_t * get_the_deepest_enclosing(mem_regions_t * region, mem_info_t * info){


	mem_info_t * mem = NULL;
	if (info->start <= region->start && info->end >= region->end){
		mem = info;
	}

	for (int i = 0; i < info->mem_infos.size(); i++){
		mem_info_t * ret = get_the_deepest_enclosing(region, info->mem_infos[i]);
		if (ret != NULL) mem = ret;
	}

	return mem;


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

				mem_info_t * info = get_the_deepest_enclosing(mem_regions[i], mem_info[j]);

				/* ok if the memory region is completely contained in the region constructed by meminfo */
				if (info != NULL){
					/* how much to the left of start is the meminfo spread? */
					uint64_t start = mem_regions[i]->start;
					vector<uint32_t> left_spread;
					for (int k = mem_regions[i]->dimensions - 1; k >= 0; k--){
						uint32_t spread = (start - info->start) / mem_regions[i]->strides[k];
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
						uint32_t spread = (info->end - end) / mem_regions[i]->strides[k];
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

					mem_regions[i]->start = info->start;
					mem_regions[i]->end = info->end;

					cout << "dims : " << get_number_dimensions(info) << endl;
					cout << mem_info[j]->start << endl;
					cout << mem_info[j]->end << endl;
 

					if (mem_regions[i]->dimensions == get_number_dimensions(info)){

						/* if we get the dimensionality correct on our memory analysis we should
						   use it instead of the memory dump information.
						*/
						for (int k = 0; k < mem_regions[i]->dimensions; k++){
							mem_regions[i]->extents[k] = get_extents(info, k + 1, get_number_dimensions(info));
						}

					}

					final_regions.push_back(mem_regions[i]);
					total_regions.push_back(mem_regions[i]);
					break;

				}

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

void  mark_regions_input_output(vector<mem_regions_t *> regions,vector<mem_regions_t *> input){

}

/* we should be concerned with */

vector<mem_regions_t *> get_input_output_regions(vector<mem_regions_t *> &image_regions, vector<mem_regions_t *> &total_regions, 
	vector<pc_mem_region_t* > &pc_mems, vector<uint32_t> app_pc){

	vector<mem_regions_t *> ret;
	mem_regions_t * input_mem_region = NULL;
	mem_regions_t * output_mem_region = NULL;

	for (int i = 0; i < image_regions.size(); i++){
		if (image_regions[i]->type == IMAGE_INPUT){
			input_mem_region = image_regions[i]; break;
		}
	}


	for (int i = 0; i < image_regions.size(); i++){
		if (image_regions[i]->type == IMAGE_OUTPUT){
			output_mem_region = image_regions[i]; break;
		}
	}

	/* filter based on candidate instructions */
	vector<pc_mem_region_t *> candidate_pc_mem;
	for (int i = 0; i < app_pc.size(); i++){
		candidate_pc_mem.push_back(get_pc_mem_region(pc_mems, app_pc[i]));
	}


	/* select candidate output and input locations if dump fails for output and input */
	vector<mem_regions_t *> candidate_output;
	vector<mem_regions_t *> candidate_input;

	if (output_mem_region == NULL && input_mem_region == NULL){

		ASSERT_MSG((app_pc.size() != 0), ("ERROR: cannot find output/input from dump; please pass in the app_pc file\n"));

		for (int i = 0; i < total_regions.size(); i++){
			bool found = false;
			if (total_regions[i]->buffer){
				for (int j = 0; j < candidate_pc_mem.size(); j++){
					for (int k = 0; k < candidate_pc_mem[j]->regions.size(); j++){
						mem_info_t * info = candidate_pc_mem[j]->regions[k];
						if (is_overlapped(info->start, info->end, total_regions[i]->start, total_regions[i]->end)){
							if ((total_regions[i]->type & IMAGE_OUTPUT) == IMAGE_OUTPUT){
								candidate_output.push_back(total_regions[i]);
							}
							if ((total_regions[i]->type & IMAGE_INPUT) == IMAGE_INPUT){
								candidate_input.push_back(total_regions[i]);
							}
							found = true; break;
						}
					}
					if (found) break;
				}
			}
		}

	}

	/* input there; output can't find
	Then - use forward dependancy to find a heap dependancy; assign the last heap dependancy as the output location
	*/
	if (output_mem_region == NULL && input_mem_region != NULL){

	}
	/* output there; input can't find; build a tree for output
	*/
	else if (output_mem_region != NULL && input_mem_region == NULL){

	}

	ret.push_back(output_mem_region);
	ret.push_back(input_mem_region);

	return ret;


}