#include <iostream>
#include <map>
#include <algorithm>

#include "utility/defines.h"
#include "common_defines.h"
#include "utilities.h"
#include "memory/memanalysis.h"

#include "meminfo.h"
#include "trees/trees.h"



using namespace std;

mem_info_t * get_the_deepest_enclosing(mem_regions_t * region, mem_info_t * info){

	uint64_t region_start = region->start > region->end ? region->end : region->start; 
	uint64_t region_end = region->start > region->end ? region->start : region->end;
	

	mem_info_t * mem = NULL;
	if (info->start <= region_start && info->end >= region_end){
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

		uint64_t region_start = 0;
		uint64_t region_end = 0;
		if (mem_regions[i]->start > mem_regions[i]->end){
			region_start = mem_regions[i]->end;
			region_end = mem_regions[i]->start;
			DEBUG_PRINT(("region start is greater than end\n"), 4);
		}
		else{
			region_start = mem_regions[i]->start;
			region_end = mem_regions[i]->end;
			DEBUG_PRINT(("region start is lesser than end\n"), 4);
		}

		for (int j = 0; j < mem_info.size(); j++){


			if (is_overlapped(region_start, region_end - 1, mem_info[j]->start, mem_info[j]->end - 1)){

				merged_mem_info[j] = true;
				mem_regions[i]->direction = mem_info[j]->direction;
				
				/* debug information about the merging */
				if (debug && debug_level >= 2){
					LOG(log_file, "overlap for mem region" << endl);
					LOG(log_file, "mem region:" << endl);
					LOG(log_file, "start : " << hex << mem_regions[i]->start << " end : " << mem_regions[i]->end << endl);
					LOG(log_file, dec << "strides : ");
					for (int k = 0; k < mem_regions[i]->dimensions; k++) LOG(log_file, mem_regions[i]->strides[k] << ",");
					LOG(log_file, dec << "extents : ");
					for (int k = 0; k < mem_regions[i]->dimensions; k++) LOG(log_file, mem_regions[i]->extents[k] << ",");
					LOG(log_file,endl);
					LOG(log_file,"mem instrace:" << endl);
					LOG(log_file,"start : " << hex << mem_info[j]->start << " end : " << mem_info[j]->end << dec << endl);

				}

				mem_info_t * info = get_the_deepest_enclosing(mem_regions[i], mem_info[j]);
				/* ok if the memory region is completely contained in the region constructed by meminfo */
				if (info != NULL){
					/* how much to the left of start is the meminfo spread? */
					LOG(log_file, "found deepest enclosing: start " << info->start << " end " << info->end << endl);
					
					if (mem_regions[i]->start < mem_regions[i]->end){
						uint64_t start = mem_regions[i]->start;
						vector<uint32_t> left_spread;
						for (int k = mem_regions[i]->dimensions - 1; k >= 0; k--){
							uint32_t spread = (start - info->start) / mem_regions[i]->strides[k];
							start = mem_regions[i]->start - spread * mem_regions[i]->strides[k];
							left_spread.push_back(spread);
						}

						/* printing out the spreads */
						LOG(log_file, dec << "left spread: ");
						for (int k = 0; k < left_spread.size(); k++){
							LOG(log_file, left_spread[k] << ",");
						}
						LOG(log_file, endl);

						/* how much to the right of the end of the meminfo are we spread? */
						uint64_t end = mem_regions[i]->end;
						vector<uint32_t> right_spread;
						for (int k = mem_regions[i]->dimensions - 1; k >= 0; k--){
							uint32_t spread = (info->end - end) / mem_regions[i]->strides[k];
							end = mem_regions[i]->end + spread * mem_regions[i]->strides[k];
							right_spread.push_back(spread);
						}

						/* printing out the spreads */
						LOG(log_file, "right spread: ");
						for (int k = 0; k < right_spread.size(); k++){
							LOG(log_file, right_spread[k] << ",");
						}
						LOG(log_file, endl);

						LOG(log_file, "--------------------------" << endl);
					}

					if (mem_regions[i]->start < mem_regions[i]->end){
						mem_regions[i]->start = info->start;
						mem_regions[i]->end = info->end;
					}
					else{
						mem_regions[i]->start = info->end;
						mem_regions[i]->end = info->start;
					}

					LOG(log_file,"dims : " << get_number_dimensions(info) << endl);
					LOG(log_file, "new start : " << mem_info[j]->start << endl);
					LOG(log_file, "new end : " <<mem_info[j]->end << endl);
 

					if (mem_regions[i]->dimensions == get_number_dimensions(info)){

						/* if we get the dimensionality correct on our memory analysis we should
						   use it instead of the memory dump information.
						*/
						for (int k = 0; k < mem_regions[i]->dimensions; k++){
							mem_regions[i]->extents[k] = get_extents(info, k + 1, get_number_dimensions(info));
						}

						LOG(log_file, "new extents recorded" << endl);
						for (int k = 0; k < mem_regions[i]->dimensions; k++) LOG(log_file, mem_regions[i]->extents[k] << ",");

					}

					/* added - for invert */
					if (mem_regions[i]->bytes_per_pixel != info->prob_stride){
						uint32_t factor = info->prob_stride / mem_regions[i]->bytes_per_pixel;
						mem_regions[i]->bytes_per_pixel = info->prob_stride;
						mem_regions[i]->strides[0] = info->prob_stride;
						mem_regions[i]->extents[0] /= factor;
					
					}

					final_regions.push_back(mem_regions[i]);
					total_regions.push_back(mem_regions[i]);
					mem_regions[i]->order = info->order;
					break;

				}
				else{ /* FIXME: */
					DEBUG_PRINT(("WARNING: region is greater than what is accessed; may be not whole image accessed\n"), 2);
					LOG(log_file,"WARNING: region is greater than what is accessed; may be not whole image accessed\n");
					final_regions.push_back(mem_regions[i]);
					total_regions.push_back(mem_regions[i]);
					mem_regions[i]->order = mem_info[j]->order;
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

				mem->direction = mem_info[i]->direction;
				mem->order = mem_info[i]->order;
				total_regions.push_back(mem);

			}
		}
	}


	/* naming the memory regions */
	int inputs = 0;
	int intermediates = 0;
	int outputs = 0;

	for (int i = 0; i < total_regions.size(); i++){
		if (total_regions[i]->direction == MEM_INPUT){
			total_regions[i]->name = "input_" + to_string(++inputs);
		}
		else if (total_regions[i]->direction == MEM_OUTPUT){
			total_regions[i]->name = "output_" + to_string(++outputs);
		}
		else if (total_regions[i]->direction == MEM_INTERMEDIATE){
			total_regions[i]->name = "inter_" + to_string(++intermediates);
		}
	}


	DEBUG_PRINT((" no of image mem regions after merging - %d\n", final_regions.size()), 2);
	DEBUG_PRINT((" total number of mem regions (from instrace) - %d\n", total_regions.size()), 2);

	DEBUG_PRINT(("merge_instrace_and_dump_regions - done\n"), 2);

	cout << dec;

	return final_regions;
}

vector<mem_regions_t *> get_image_regions_from_instrace(vector<mem_info_t *> &mem, ifstream &config, image_t * in_image, image_t * out_image){

	ASSERT_MSG(false, ("ERROR: function not implemented\n"));

}


void  mark_regions_type(vector<mem_regions_t *> regions, vector<mem_regions_t *> input, vec_cinstr &instrs, 
	map< uint32_t, vector<mem_regions_t *> > maps, uint32_t start, uint32_t end){

	Conc_Tree * tree = new Conc_Tree();

	bool input_given = input.size() > 0;
	
	for (int j = 0; j < input.size(); j++){
		mem_regions_t * mem = input[j];
		for (uint64_t i = mem->start; i < mem->end; i += mem->bytes_per_pixel){
			operand_t opnd = { MEM_HEAP_TYPE, mem->bytes_per_pixel, i };
			tree->add_to_frontier(tree->generate_hash(&opnd), new Conc_Node(&opnd));
		}
	}
	
	int amount = 0;
	bool * srcs_dep = new bool[2];
	srcs_dep[0] = false; srcs_dep[1] = false;


#define WRITTEN	2
#define READ	1

	map<uint64_t, uint32_t> memory_map;

	for (uint32_t i = start; i < end; i++){

		if (i % 10000 == 0) DEBUG_PRINT(("."),2);

		cinstr_t * instr = instrs[i].first;
		rinstr_t * rinstr;
		string para = instrs[i].second->disassembly;

		rinstr = cinstr_to_rinstrs_eflags(instr, amount, para, i + 1);

		vector<mem_regions_t *> regions = maps[instr->pc];

		if (!input_given){
			for (int j = 0; j < instr->num_srcs; j++){
				if ((instr->srcs[j].type == MEM_HEAP_TYPE || instr->srcs[j].type == MEM_STACK_TYPE) && ((memory_map[instr->srcs[j].value] & WRITTEN) != WRITTEN)){
					for (int k = 0; k < regions.size(); k++){
						if (is_overlapped(regions[k]->start, regions[k]->end, instr->srcs[j].value, instr->srcs[j].value)){ 
							regions[k]->type |= INPUT_BUFFER;
							memory_map[instr->srcs[j].value] |= READ;
							if (tree->search_node(&instr->srcs[j]) == NULL){
								tree->add_to_frontier(tree->generate_hash(&instr->srcs[j]), new Conc_Node(&instr->srcs[j]));
							}
						}
					}
				}
			}
		}



		for (int j = 0; j < amount; j++){
			if (tree->update_depandancy_forward_with_src(&rinstr[j], instr->pc, instrs[i].second->disassembly, i + 1, srcs_dep)){

				for (int k = 0; k < rinstr[j].num_srcs; k++){
					if ((rinstr[j].srcs[k].type == MEM_STACK_TYPE || rinstr[j].srcs[k].type == MEM_HEAP_TYPE) && srcs_dep[k]){
						for (int m = 0; m < regions.size(); m++){
							if (is_overlapped(regions[m]->start, regions[m]->end, rinstr->srcs[k].value, rinstr->srcs[k].value)){ 
								memory_map[instr->srcs[k].value] |= READ;
								if ( (regions[m]->type & OUTPUT_BUFFER) == OUTPUT_BUFFER ) regions[m]->type |= INTERMEDIATE_BUFFER;
								else regions[m]->type |= INPUT_BUFFER;
								regions[m]->dependant = true;
							}
						}
					}
				}

				if ((rinstr[j].dst.type == MEM_STACK_TYPE || rinstr[j].dst.type == MEM_HEAP_TYPE)){
					for (int m = 0; m < regions.size(); m++){
						if (is_overlapped(regions[m]->start, regions[m]->end, rinstr->dst.value, rinstr->dst.value)){ 
							memory_map[rinstr->dst.value] |= WRITTEN;
							regions[m]->type |= OUTPUT_BUFFER;
							regions[m]->dependant = true;
						}
					}
				}
			}
		}

	}

	DEBUG_PRINT(("\n"), 2);

}


vector<mem_regions_t *> get_input_output_regions(vector<mem_regions_t *> &image_regions, vector<mem_regions_t *> &total_regions,
	vector<pc_mem_region_t* > &pc_mems, vector<uint32_t> app_pc, vec_cinstr &instrs, vector<uint32_t> start_points){

	DEBUG_PRINT(("getting input output region for further analysis\n"), 2);

	vector<mem_regions_t *> ret;
	mem_regions_t * input_mem_region = NULL;
	mem_regions_t * output_mem_region = NULL;

	for (int i = 0; i < image_regions.size(); i++){
		if (image_regions[i]->dump_type == INPUT_BUFFER){
			input_mem_region = image_regions[i]; break;
		}
	}


	for (int i = 0; i < image_regions.size(); i++){
		if (image_regions[i]->dump_type == OUTPUT_BUFFER){
			output_mem_region = image_regions[i]; break;
		}
	}

	if (input_mem_region != NULL && output_mem_region != NULL){
		DEBUG_PRINT(("Both input and output regions are found from dump\n"), 2);
		ret.push_back(input_mem_region);
		ret.push_back(output_mem_region);
		return ret;
	}


	if (input_mem_region == NULL) DEBUG_PRINT(("input region missing\n"), 2);
	if (output_mem_region == NULL) DEBUG_PRINT(("output region missing\n"), 2);

	/* mapping - pc, regions */
	map< uint32_t, vector<mem_regions_t *> > region_map;
	for (int i = 0; i < pc_mems.size(); i++){
		for (int k = 0; k < total_regions.size(); k++){
			for (int j = 0; j < pc_mems[i]->regions.size(); j++){
				if (is_overlapped(pc_mems[i]->regions[j]->start, pc_mems[i]->regions[j]->end, total_regions[k]->start, total_regions[k]->end)){
					region_map[pc_mems[i]->pc].push_back(total_regions[k]);
					break;
				}
			}
		}
	}

	vector<mem_regions_t *> input_regions;
	if (input_mem_region != NULL){
		input_regions.push_back(input_mem_region);

		DEBUG_PRINT((" marking dependant regions using the input image\n"), 2);

		uint32_t start = 0;
		uint32_t end = 0;

		for (int i = 0; i < start_points.size(); i++){
			if (i != start_points.size() - 1) end = start_points[i + 1];
			else end = instrs.size();
			mark_regions_type(total_regions, input_regions, instrs, region_map, start, end);
			start = end;
		}
	}

	if (output_mem_region == NULL && input_mem_region == NULL){

		ASSERT_MSG(false, ("ERROR: not yet implemented\n"));

	}
	else if (output_mem_region == NULL && input_mem_region != NULL){
		for (int i = 0; i < total_regions.size(); i++){
			if ((total_regions[i]->type & OUTPUT_BUFFER) == OUTPUT_BUFFER){
				output_mem_region = total_regions[i];
				DEBUG_PRINT(("new output region found\n"), 2);
				break;
			}
		}

		

	}
	else if (output_mem_region != NULL && input_mem_region == NULL){

		ASSERT_MSG(false, ("ERROR: not yet implemented\n"));

	}

	ret.push_back(input_mem_region);
	ret.push_back(output_mem_region);

	return ret;


}


bool compare_mem_region(mem_regions_t * first, mem_regions_t * second){
	return first->order < second->order;
}

vector<mem_regions_t *> get_input_regions(vector<mem_regions_t *> total_regions, vector<pc_mem_region_t *> &pc_mems,
	vector<uint32_t> start_points, vec_cinstr &instrs){

	sort(total_regions.begin(), total_regions.end(), compare_mem_region);

	/*log_file << "***************sorted**************" << endl;
	print_mem_regions(log_file, total_regions);
	log_file << "****end*****" << endl;*/

	vector<mem_regions_t *> inputs;

	/* mapping - pc, regions */
	map< uint32_t, vector<mem_regions_t *> > region_map;
	for (int i = 0; i < pc_mems.size(); i++){
		for (int k = 0; k < total_regions.size(); k++){
			for (int j = 0; j < pc_mems[i]->regions.size(); j++){
				if (is_overlapped(pc_mems[i]->regions[j]->start, pc_mems[i]->regions[j]->end, total_regions[k]->start, total_regions[k]->end)){
					region_map[pc_mems[i]->pc].push_back(total_regions[k]);
					break;
				}
			}
		}
	}

	for (int i = 0; i < total_regions.size(); i++){

		if ((!total_regions[i]->dependant) && ( (total_regions[i]->direction & MEM_INPUT) == MEM_INPUT) ){

			inputs.push_back(total_regions[i]);

			uint32_t start = 0;
			uint32_t end = 0;

			vector<mem_regions_t *> current;
			current.push_back(total_regions[i]);

			for (int i = 0; i < start_points.size(); i++){
				if (i != start_points.size() - 1) end = start_points[i + 1];
				else end = instrs.size();
				mark_regions_type(total_regions,current, instrs, region_map, start, end);
				start = end;
			}


		}


	}

	return inputs;


}


