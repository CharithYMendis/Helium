/* we will try to recreate the mem layout from the execution traces */
#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include "common_defines.h"
#include "meminfo.h"
#include "utilities.h"
#include <algorithm>
#include <set>


using namespace std;


/* internal */
static int	get_most_probable_stride(vector<pair<uint, uint> > &strides);
static void defragment_regions(vector<mem_info_t *> &mem_info);


/* internal function implementation */

/* should we record the smallest stride? or the most frequent stride (width) as the data element width
*/

static int get_most_probable_stride(vector<pair<uint, uint> > &strides){

	int stride = -1;
	int max_freq = -1;

	for (int i = 0; i < strides.size(); i++){
		if (max_freq < (int)strides[i].second){
			max_freq = (int)strides[i].second;
			stride = (int)strides[i].first;
		}
	}

	return stride;

}

static void update_stride(vector<pair<uint, uint> > &strides, uint stride){

	bool updated = false;

	for (int i = 0; i < strides.size(); i++){
		if (strides[i].first == stride){
			strides[i].second++;
			updated = true;
			break;
		}
	}
	if (!updated){
		strides.push_back(make_pair(stride, 1));
	}
}

void sort_mem_info(vector<mem_info_t *> &mem_info){

	sort(mem_info.begin(), mem_info.end(), [](mem_info_t * first, mem_info_t *second)->bool{

		return first->start < second->start;

	});

}

static bool compare_pc_mem_regions(pc_mem_region_t * first, pc_mem_region_t * second){

	uint32_t highest_first = 0;
	uint32_t highest_second = 0;

	for (int i = 0; i < first->regions.size(); i++){
		uint32_t size = (first->regions[i]->start - first->regions[i]->end);
		if (highest_first < size){
			highest_first = size;
		}
	}

	for (int i = 0; i < second->regions.size(); i++){
		uint32_t size = (second->regions[i]->start - second->regions[i]->end);
		if (highest_second < size){
			highest_second = size;
		}
	}

	return highest_first >= highest_second;  /* we are going to get the reversed ranking */

}

void rank_pc_mems_from_highest(vector<pc_mem_region_t *> &pc_mems){

	sort(pc_mems.begin(), pc_mems.end(), compare_pc_mem_regions);

}

static void update_stride_from_vector(vector<pair<uint, uint> > &strides, vector<pair<uint, uint> > &old){
	for (int i = 0; i < old.size(); i++){

		uint old_stride = old[i].first;
		uint old_freq = old[i].second;
		bool updated = false;
		for (int j = 0; j < strides.size(); j++){
			if (strides[j].first == old_stride){
				strides[j].second += old_freq;
				updated = true;
				break;
			}
		}

		if (!updated){
			strides.push_back(old[i]);
		}
	}
}

void update_mem_regions(vector<mem_info_t *> &mem_info, mem_input_t * input){

		uint64 addr = input->mem_addr;
		uint stride = input->stride;
		bool merged = false;

		uint32_t direction = 0;
		vector<pair<uint, uint> > stride_acc;

		for (int i = 0; i < mem_info.size(); i++){
			mem_info_t * info = mem_info[i];
			if (info->type == input->type){

				/* is the address with in range?*/
				if ((addr >= info->start) && (addr + stride <= info->end)){
					info->direction |= input->write ? MEM_OUTPUT : MEM_INPUT;
					update_stride(info->stride_freqs, stride);
					merged = true;
				}
				/* delete the memory region that is contained */
				else if ((addr < info->start) && (addr + stride > info->end)){

					direction |= info->direction;
					update_stride_from_vector(stride_acc, info->stride_freqs);
					mem_info.erase(mem_info.begin() + i--);

				}
				/* can we prepend this to the start of the memory region? */
				else if ((addr + stride >= info->start) && (addr < info->start)){


					info->start = addr;
					info->direction |= input->write ? MEM_OUTPUT : MEM_INPUT;
					info->direction |= direction;
					update_stride(info->stride_freqs, stride);
					merged = true;
				}

				/* can we append this to the end of the memory region? */
				else if ((addr <= info->end) && (addr + stride > info->end)){

					info->end = addr + stride;
					info->direction |= input->write ? MEM_OUTPUT : MEM_INPUT;
					info->direction |= direction;
					update_stride(info->stride_freqs, stride);
					merged = true;

				}

			}
			else{
				/* see if there are any collisions and report as errors - implement later */
			}

			if (merged) break;

		}

		if (!merged){ /* if not merged to an exising mem_region then we need to create a new region */
			mem_info_t * new_region = new mem_info_t;
			new_region->start = addr;
			new_region->end = addr + stride;  /* actually this should be stride - 1 */
			new_region->direction = input->write ? MEM_OUTPUT : MEM_INPUT;
			new_region->direction |= direction;
			new_region->type = input->type;
			update_stride_from_vector(new_region->stride_freqs, stride_acc);
			update_stride(new_region->stride_freqs, stride);
			mem_info.push_back(new_region);
		}
}

/* this rountine is sound defragmentation */
static void defragment_regions(vector<mem_info_t *> &mem_info){

	/*this will try to merge individual chunks of memory units info defragmented larger chunks*/
	bool finished = false;

	while (!finished){

		int start_size = mem_info.size();

		for (int i = 0; i < mem_info.size(); i++){

			mem_info_t * candidate = mem_info[i];

			for (int j = i + 1; j < mem_info.size(); j++){

				mem_info_t * current = mem_info[j];
				if (current == candidate) continue;  /* this never happens? */

				/*check if they can be merged*/
				if (current->type == candidate->type){

					/*can we merge the two? we will always update the candidate and delete the current if this can be merged*/
					/* we cannot possibly have a mem region captured within a mem region if update mem regions has done its job*/

					bool merged = false;

					/*if the candidate is a subset of the current? remove candidate*/
					if ((candidate->start >= current->start) && (candidate->end <= current->end)){
						current->direction |= candidate->direction;
						update_stride_from_vector(current->stride_freqs, candidate->stride_freqs);
						delete mem_info[i];
						mem_info.erase(mem_info.begin() + i--);
						break;
					}
					/*if current is a subset of the candidate? remove current*/
					else if ((current->start >= candidate->start) && (current->end <= candidate->end)){
						merged = true;
					}
					/* prepend to the candidate?*/
					else if ((current->start < candidate->start) && (current->end >= candidate->start)){
						candidate->start = current->start;
						merged = true;
					}
					/* append to candidate?*/
					else if ((current->start <= candidate->end) && (current->end > candidate->end)){
						candidate->end = current->end;
						merged = true;
					}

					if (merged){
						candidate->direction |= current->direction;
						update_stride_from_vector(candidate->stride_freqs, current->stride_freqs);
						delete mem_info[j];
						mem_info.erase(mem_info.begin() + j--);
					}

				}

			}
		}

		int end_size = mem_info.size();

		finished = (start_size == end_size);

	}


}

/* update most prob stride information */
static void update_most_prob_stride(vector<mem_info_t *> &mem_info){

	for (int i = 0; i < mem_info.size(); i++){
		int stride = get_most_probable_stride(mem_info[i]->stride_freqs);
		mem_info[i]->prob_stride = stride;
	}

}

/* pc_mem_info_t related functions */

static pc_mem_region_t * get_pc_mem_region(vector<pc_mem_region_t *> &pc_mems, string module, uint32_t app_pc){

	/* first check whether we have the same app_pc */
	if (!module.empty()){ /* here the module information comparison is requested by the input */
		for (int i = 0; i < pc_mems.size(); i++){
			ASSERT_MSG(!pc_mems[i]->module.empty(),("module information is missing from mem_regions\n"));
			if (pc_mems[i]->module.compare(module) == 0){
				/* now check for the app_pc */
				if (pc_mems[i]->pc == app_pc){
					return pc_mems[i];
				}
			}
		}
	}
	else{  /* module information is disregarded; here we get the first match with app_pc we do not try to see whether there are more than one match*/
		for (int i = 0; i < pc_mems.size(); i++){
			if (pc_mems[i]->pc == app_pc){
				return pc_mems[i];
			}
		}
	}


	return NULL;


}


void update_mem_regions(vector<pc_mem_region_t *> &pc_mems, mem_input_t * input){

	pc_mem_region_t * mem_region = get_pc_mem_region(pc_mems, input->module, input->pc);
	if (mem_region != NULL){ /* we found a mem region */
		update_mem_regions(mem_region->regions, input);
	}
	else{
		mem_region = new pc_mem_region_t;
		mem_region->pc = input->pc;
		mem_region->module = input->module;
		update_mem_regions(mem_region->regions, input);
		pc_mems.push_back(mem_region);
	}
	
}

void merge_info_to_first(mem_info_t * first, mem_info_t * second){
	
	first->direction |= second->direction;
	update_stride_from_vector(first->stride_freqs, second->stride_freqs);
	first->prob_stride = get_most_probable_stride(first->stride_freqs);

}


vector<mem_info_t *> merge_mem_regions(vector<mem_info_t *> first_mem, vector<mem_info_t *> second_mem){

	/* try to merge as many from the second_mem as possible */
	for (int i = 0; i < first_mem.size(); i++){
		for (int j = 0; j < second_mem.size(); j++){
			/* check whether first is a subset of second */

			mem_info_t * first = first_mem[i];
			mem_info_t * second = second_mem[j];

			if (first->type == second->type){
				/* first is a subset of second */
				if (second->start <= first->start && second->end >= first->end){
					first_mem.erase(first_mem.begin() + i--);
					merge_info_to_first(second, first);
					break; /* as first is removed start from a new first */
				}

				/* second is a subset of first */
				else if (first->start <= second->start && first->end >= second->end){
					second_mem.erase(second_mem.begin() + j--);
					merge_info_to_first(first, second);
				}

				/* if there is an overlap then second is deleted and merged into the first */
				else if (first->start <= second->start && first->end >= second->start){
					first->end = second->end;
					second_mem.erase(second_mem.begin() + j--);
					merge_info_to_first(first, second);
				}
				else if (first->start >= second->start && first->start <= second->end){
					first->start = second->start;
					second_mem.erase(second_mem.begin() + j--);
					merge_info_to_first(first, second);
				}

				/* otherwise there is no overlap */

			}

		}
	}

	/* push back the remaining second_mem to first_mem */
	for (int i = 0; i < second_mem.size(); i++){
		first_mem.push_back(second_mem[i]);
	}
	
	return first_mem;

}

vector<mem_info_t *> extract_mem_regions(vector<pc_mem_region_t *> &pc_mems){
	/* reuse the existing functions */
	vector<mem_info_t *> ret;

	for (int i = 0; i < pc_mems.size(); i++){
		ret = merge_mem_regions(ret, pc_mems[i]->regions);
	}
	defragment_regions(ret);
	return ret;

}

void print_mem_layout(ostream &file, vector<pc_mem_region_t *> &pc_mems){
	
	//DEBUG_PRINT(("printing pc_meminfo...\n"), 2);

	for (int i = 0; i < pc_mems.size(); i++){
		if (!pc_mems[i]->module.empty()){
			file << "module name - " << pc_mems[i]->module.c_str() << endl;
		}
		file << "app_pc - " << dec << pc_mems[i]->pc << " mem regions - " << pc_mems[i]->regions.size() << endl;
		print_mem_layout(file,pc_mems[i]->regions); /* print mem layout has the seperation line */
	}

}

bool random_dest_select(vector<pc_mem_region_t *> &pc_mems, string module, uint32_t app_pc, uint64_t * dest, uint32_t * stride){

	pc_mem_region_t * region = get_pc_mem_region(pc_mems, module, app_pc);
	if (region == NULL){
		return false;
	}
	else{
		return random_dest_select(region->regions, dest, stride);
	}

}

void postprocess_mem_regions(vector<pc_mem_region_t *> &pc_mem){

	DEBUG_PRINT((" found %d pc mem regions for post processing \n", pc_mem.size()), 5);
	uint32_t count = 0;
	for (int i = 0; i < pc_mem.size(); i++){

		DEBUG_PRINT((" app_pc %x mem region size before %d \n",pc_mem[i]->pc, pc_mem[i]->regions.size()), 5);
		defragment_regions(pc_mem[i]->regions);
		update_most_prob_stride(pc_mem[i]->regions);
		DEBUG_PRINT((" mem region size after %d \n", pc_mem[i]->regions.size()), 5);
		print_progress(&count, 1);

	}
	
}


void link_mem_regions_dim(vector<pc_mem_region_t *> &pc_mems, uint32_t mode){

	for (int i = 0; i < pc_mems.size(); i++){

		if (mode == DYNAMIC_PROG){
			link_mem_regions(pc_mems[i]->regions, pc_mems[i]->pc);
		}
		else if (mode == GREEDY){
			link_mem_regions_greedy_dim(pc_mems[i]->regions, pc_mems[i]->pc);
		}

	}
}

void link_mem_regions(vector<pc_mem_region_t *> &pc_mems, uint32_t mode){

	for (int i = 0; i < pc_mems.size(); i++){

		if (mode == DYNAMIC_PROG){
			link_mem_regions(pc_mems[i]->regions, pc_mems[i]->pc);
		}
		else if(mode == GREEDY){
			link_mem_regions_greedy(pc_mems[i]->regions, pc_mems[i]->pc);
		}
		
	}
}



/* mem_info_t related functions */

void postprocess_mem_regions(vector<mem_info_t *> &mem){
	defragment_regions(mem);
	update_most_prob_stride(mem);
}


bool random_dest_select(vector<mem_info_t *> &mem_info, uint64_t * dest, uint32_t * stride){

	/*get heap regions*/
	uint size_heap = 0;
	for (int i = 0; i < mem_info.size(); i++){
		if ((mem_info[i]->type == MEM_HEAP_TYPE) && ((mem_info[i]->direction & MEM_OUTPUT) == MEM_OUTPUT)){
			size_heap++;
		}
	}

	uint current_heap = 0;

	/* select a random location to track using the heap regions (actually non stack regions) */
	for (int i = 0; i < mem_info.size(); i++){
		if ((mem_info[i]->type == MEM_HEAP_TYPE) && ((mem_info[i]->direction & MEM_OUTPUT) == MEM_OUTPUT)){

			current_heap++;
			if (current_heap < size_heap / 2) continue;

			uint32_t probable_stride = get_most_probable_stride(mem_info[i]->stride_freqs);

			uint32_t size = (mem_info[i]->end - mem_info[i]->start) / (probable_stride);

			uint32_t random = size / 2 + 10;

			/* here we are returning the first heap block we get; we can also randomize this */
			*dest = mem_info[i]->start + random * probable_stride;
			*stride = probable_stride;

			DEBUG_PRINT(("size, random, start - %d, %d, %llu\n", size, random, mem_info[i]->start), 1);

			return true;

		}
	}

	return false;

}

void print_mem_layout(ostream &file, vector<mem_info_t *> &mem_info){

	//DEBUG_PRINT(("print_mem_layout(mem_info) \n"), 2);

	for (int i = 0; i < mem_info.size(); i++){
		mem_info_t * info = mem_info[i];

		/* print out the type */
		if (info->type == MEM_HEAP_TYPE) file << "type - heap" << endl;
		if (info->type == MEM_STACK_TYPE) file << "type - stack" << endl;


		/* print out the direction */
		if ((info->direction & MEM_INPUT) == MEM_INPUT) file << "dir - input" << endl;
		if ((info->direction & MEM_OUTPUT) == MEM_OUTPUT) file << "dir - output\n" << endl;

		file << "start - " << dec << info->start << endl;
		file << "end - " << dec << info->end << endl;

		uint stride = get_most_probable_stride(info->stride_freqs);

		file << "stride (most) - " << dec << stride << endl;

		for (int j = 0; j < info->stride_freqs.size(); j++){
			file << "stride - " << dec << info->stride_freqs[j].first << ", freq - " << info->stride_freqs[j].second << endl;
		}

		file << "estimated size - " << dec << (info->end - info->start) / (stride) << endl;

		uint32_t dims = get_number_dimensions(info);
		file << "dims - " << dims << endl;
		//cout << "dims - " << dims << endl;
		for (int j = 1; j <= dims; j++){
			file << "dim_no - " << j << endl;
			file << "stride - " << get_stride(info, j, dims) << endl;
			file << "extent - " << get_extents(info, j, dims) << endl;
		}

		file << "--------------------------------------------------------------------------" << endl;

	}

}

bool compare_mem_regions(mem_info_t * first, mem_info_t * second){

	return (first->start <= second->start);

}

/*
* this is a greedy (non-optimal) coalescing of mem regions
*/



uint32_t get_stride(mem_info_t * mem, uint32_t dim, uint32_t total_dims){
	
	vector<mem_info_t * > local_mems;
	local_mems.push_back(mem);

	mem_info_t * local = mem;

	while (local->mem_infos.size() > 0){
		local_mems.push_back(local->mem_infos[0]);
		local = local->mem_infos[0];
	}

	mem_info_t * wanted = local_mems[total_dims - dim];
	if (wanted->mem_infos.size() > 0){
		return wanted->mem_infos[1]->start - wanted->mem_infos[0]->start;
	}
	else{
		return wanted->prob_stride;
	}

}

uint32_t get_extents(mem_info_t * mem, uint32_t dim, uint32_t total_dims){

	vector<mem_info_t * > local_mems;
	local_mems.push_back(mem);

	mem_info_t * local = mem;

	while (local->mem_infos.size() > 0){
		local_mems.push_back(local->mem_infos[0]);
		local = local->mem_infos[0];
	}

	mem_info_t * wanted = local_mems[total_dims - dim];
	if (wanted->mem_infos.size() > 0){
		return wanted->mem_infos.size();
	}
	else{
		return (wanted->end - wanted->start) / wanted->prob_stride;
	}

}

uint32_t get_number_dimensions(mem_info_t * mem){

	uint32_t dim = 1;
	mem_info_t * local_mem = mem;
	while (local_mem->mem_infos.size() > 0){
		dim++;
		local_mem = local_mem->mem_infos[0];
	}
	return dim;
}

/*
Here, we check whether a particular pc accesses two coalesced memory regions -> if yes we can merge these two memory regions.
*/
vector< vector< mem_info_t * > > get_merge_opportunities(vector<mem_info_t *> mem_info, vector<pc_mem_region_t *> pc_mems){

	vector< vector<mem_info_t *> > ret;
	sort_mem_info(mem_info);

	DEBUG_PRINT(("getting merge opportunities...\n"), 2);
	LOG(log_file, "merge opportunities..." << endl);

	for (int i = 0; i < pc_mems.size(); i++){

		vector<mem_info_t *> overlapped;

		vector<mem_info_t *> pc_mem_info = pc_mems[i]->regions;

		for (int j = 0; j < mem_info.size(); j++){
			for (int k = 0; k < pc_mem_info.size(); k++){
				if (is_overlapped(pc_mem_info[k]->start, pc_mem_info[k]->end, mem_info[j]->start, mem_info[j]->end)){
					overlapped.push_back(mem_info[j]);
					break;
				}
			}
		}

		LOG(log_file, pc_mems[i]->pc << " - " << endl);
		LOG(log_file, "overlapped - ");
		for (int j = 0; j < overlapped.size(); j++){
			LOG(log_file, overlapped[j]->start << ",");
		}
		LOG(log_file, endl);

		vector< vector< mem_info_t *> > regions;
		vector<mem_info_t *> temp_info;
		/* find sequences which are fairly close to each other */
		for (int j = 1; j < overlapped.size(); j++){
			uint32_t first_dim = get_number_dimensions(overlapped[j - 1]);
			uint32_t first_extent = get_extents(overlapped[j - 1], first_dim, first_dim);
			uint32_t first_stride = get_stride(overlapped[j - 1], first_dim, first_dim);
			uint32_t right_first = overlapped[j - 1]->end + first_extent*first_stride;

			uint32_t second_dim = get_number_dimensions(overlapped[j]);
			uint32_t second_extent = get_extents(overlapped[j], second_dim, second_dim);
			uint32_t second_stride = get_stride(overlapped[j], second_dim, second_dim);
			uint32_t left_second = overlapped[j]->start - second_extent*second_stride;

			temp_info.push_back(overlapped[j - 1]);
			if (right_first < left_second){
				vector<mem_info_t *> temp(temp_info);
				regions.push_back(temp);
				temp_info.clear();
			}
		}

		temp_info.push_back(overlapped[overlapped.size() - 1]);
		regions.push_back(temp_info);

		LOG(log_file, "regions" << endl);
		for (int j = 0; j < regions.size(); j++){
			for (int k = 0; k < regions[j].size(); k++){
				LOG(log_file, regions[j][k]->start << ",");
			}
			LOG(log_file,endl);
		}

		for (int k = 0; k < regions.size(); k++){

			sort(regions[k].begin(), regions[k].end());

			if (regions[k].size() >= 2){

				bool added = false;

				for (int j = 0; j < ret.size(); j++){

					/* 1000 is just a heuristic value of the set size() -> make it dynamic for more resilience*/
					vector<mem_info_t *> temp(1000);
					vector<mem_info_t *>::iterator it;

					it = set_intersection(regions[k].begin(), regions[k].end(), ret[j].begin(), ret[j].end(), temp.begin());
					if (it != temp.begin()){
						it = set_union(regions[k].begin(), regions[k].end(), ret[j].begin(), ret[j].end(), temp.begin());
						temp.resize(it - temp.begin());
						ret[j] = temp;
						added = true;
						break;
					}

				}

				if (!added){
					ret.push_back(regions[k]);
				}
			}
		}
	}
	

	LOG(log_file, "final mergable regions..." << endl);
	DEBUG_PRINT(("final mergable regions....\n"), 2);
	for (int i = 0; i < ret.size(); i++){
		for (int j = 0; j < ret[i].size(); j++){
			DEBUG_PRINT(("%d,", ret[i][j]->start), 2);
			LOG(log_file,ret[i][j]->start << ",");
		}
		DEBUG_PRINT(("\n"),2);
		LOG(log_file,endl);
	}


	return ret;

}


/* this is for merging regions which may have not coalesced in buffer structure reconstruction; but found through pc analysis */
void merge_mem_regions_pc(vector < vector< mem_info_t *> > mergable, vector<mem_info_t *> total_regions){

	DEBUG_PRINT(("merging meminfo regions....\n"), 2);
	LOG(log_file, "merging information" << endl);

	/* take the biggest region and expand - assume that the ghost zones would be minimal compared to the actual buffer sizes accessed */
	for (int i = 0; i < mergable.size(); i++){

		vector< mem_info_t *> region_set = mergable[i];
		sort_mem_info(region_set);
		uint32_t max_val = 0; 
		uint32_t index = 0;

		for (int j = 0; j < region_set.size(); j++){
			if ( (region_set[j]->end - region_set[j]->start) > max_val ){
				max_val = region_set[j]->end - region_set[j]->start;
				index = j;
			}
		}


		int dims = get_number_dimensions(region_set[index]);
		mem_info_t * merged = region_set[index];

		/* check neighbours and if faraway do not merge; for now if not input do not merge */
		if (merged->direction == MEM_OUTPUT) continue;

		LOG(log_file, "merge_region before: " << merged->start << " " << merged->end << endl);
		for (int j = 1; j <= dims; j++){
			uint32_t stride = get_stride(merged, j, dims);
			uint32_t extents = get_extents(merged, j, dims);
			LOG(log_file, "dim " << j << " extent " << extents << " strides " << stride << endl);
		}

		if (index != 0){

			if (dims == 1){
				merged->start = region_set[0]->start;
			}
			else{

				int stride = get_stride(merged, dims, dims);
				while (merged->start > region_set[0]->start){
					mem_info_t * new_region = new mem_info_t;
					new_region->type = merged->type;
					new_region->direction = merged->direction;
					new_region->start = merged->start - stride;
					new_region->end = new_region->start + get_extents(merged, dims - 1, dims);
					new_region->prob_stride = merged->mem_infos[0]->prob_stride;
					merged->mem_infos.insert(merged->mem_infos.begin(), new_region);
					merged->start -= stride;
				}
			}

		}

		if (index != region_set.size() - 1){

			if (dims == 1){
				merged->end = region_set[region_set.size() - 1]->end;
			}
			else{

				int stride = get_stride(merged, dims, dims);
				while (merged->end < region_set[region_set.size() - 1]->end){
					mem_info_t * new_region = new mem_info_t;
					new_region->type = merged->type;
					new_region->direction = merged->direction;
					new_region->end = merged->end + get_extents(merged, dims - 1, dims);
					new_region->start = merged->end;
					new_region->prob_stride = merged->mem_infos[0]->prob_stride;
					merged->mem_infos.push_back(new_region);
					merged->end += stride;
				}
			}

		}

		LOG(log_file, "merge_region after: " << merged->start << " " << merged->end << endl);
		for (int j = 1; j <= dims; j++){
			LOG(log_file, "dim " << j << " extent " << get_extents(merged, j, dims) << " strides " << get_stride(merged, j, dims) << endl);
		}



	}

	

}



/* app_pc is needed when you are merging the regions of a particular pc_mem_region value*/
bool link_mem_regions_greedy_dim(vector<mem_info_t *> &mem, uint32_t app_pc){

	DEBUG_PRINT(("link_mem_regions_greedy...\n"), 2);
	LOG(log_file, "link_mem_regions_greedy....\n");

	sort(mem.begin(), mem.end(), compare_mem_regions);

	bool ret = true;

	while (ret){

		ret = false;
		for (int i = 0; i < mem.size(); i++){

			if (i + 2 >= mem.size()) continue; //at least three regions should be connected
			int32_t gap_first = mem[i + 1]->start - mem[i]->end;
			int32_t gap_second = mem[i + 2]->start - mem[i + 1]->end;

			int32_t size_first = mem[i]->end - mem[i]->start;
			int32_t size_middle = mem[i + 1]->end - mem[i + 1]->start;
			int32_t size_last = mem[i + 2]->end - mem[i + 2]->start;

			bool size_match = (size_first == size_middle && size_middle == size_last);

			if (gap_first == gap_second && size_match){ /* ok we can now merge the regions */

				mem_info_t * new_mem_info = new mem_info_t;

				int32_t gap = gap_first;
				//cout << mem[i+1]->start << " "<< mem[i]-> end<< endl;
				//mem[i]->end = mem[i + 2]->end;
				//merge_info_to_first(mem[i], mem[i + 1]);
				//merge_info_to_first(mem[i], mem[i + 2]);

				new_mem_info->direction = mem[i]->direction;
				new_mem_info->prob_stride = mem[i]->prob_stride;
				new_mem_info->type = mem[i]->type;
				new_mem_info->stride_freqs = mem[i]->stride_freqs;

				new_mem_info->start = mem[i]->start;
				new_mem_info->end = mem[i + 2]->end;
				new_mem_info->mem_infos.push_back(mem[i]);
				new_mem_info->mem_infos.push_back(mem[i+1]);
				new_mem_info->mem_infos.push_back(mem[i+2]);

				merge_info_to_first(new_mem_info, mem[i + 1]);
				merge_info_to_first(new_mem_info, mem[i + 2]);

				uint32_t index = i + 2;
				for (int j = i + 3; j < mem.size(); j++){
					int32_t gap_now = mem[j]->start - new_mem_info->end;
					int32_t size_now = mem[j]->end - mem[j]->start;
					//cout << gap << " " << gap_now << endl;
					if (mem[j]->start == 0x80067a08){
						cout << "gap:" << gap_now << " g : " << gap << endl;
						cout << hex << app_pc << dec << endl;
					}
					if (gap_now == gap && size_now == size_last){
						//mem[i]->end = mem[j]->end;
						//merge_info_to_first(mem[i], mem[j]);
						new_mem_info->mem_infos.push_back(mem[j]);
						new_mem_info->end = mem[j]->end;
						merge_info_to_first(new_mem_info, mem[j]);
						index = j;
					}
					else{
						
						break;
					}
				}

				new_mem_info->end += gap;

				DEBUG_PRINT(("app_pc %x merged indexes from %d to %d\n", app_pc, i, index), 5);
				for (int j = i; j <= index; j++){
					//delete mem[i + 1];
					mem.erase(mem.begin() + i);
				}

				mem.insert(mem.begin() + i, new_mem_info);


				new_mem_info->order = INT_MAX;
				for (int j = 0; j < new_mem_info->mem_infos.size(); j++){
					if (new_mem_info->order > new_mem_info->mem_infos[j]->order){
						new_mem_info->order = new_mem_info->mem_infos[j]->order;
					}
				}
				
				LOG(log_file, "linked infos" << endl);
				LOG(log_file, new_mem_info->start << "  " << new_mem_info->end << endl);
				LOG(log_file,"dims : " << get_number_dimensions(new_mem_info) << endl);
				LOG(log_file,"merged amount : " << new_mem_info->mem_infos.size() << endl);
				ret = true;


			}
		}
	}

	DEBUG_PRINT(("link_mem_regions_greedy - done\n"), 2);

	return ret;
}


bool link_mem_regions_greedy(vector<mem_info_t *> &mem, uint32_t app_pc){
	
	DEBUG_PRINT(("link_mem_regions_greedy...\n"), 4);

	sort(mem.begin(), mem.end(), compare_mem_regions);

	bool ret = true;

	while (ret){

		ret = false;
		for (int i = 0; i < mem.size(); i++){

			if (i + 2 >= mem.size()) continue; //at least three regions should be connected
			int32_t gap_first = mem[i + 1]->start - mem[i]->end;
			int32_t gap_second = mem[i + 2]->start - mem[i + 1]->end;

			if (gap_first == gap_second){ /* ok we can now merge the regions */
				int32_t gap = gap_first;
				//cout << mem[i+1]->start << " "<< mem[i]-> end<< endl;
				mem[i]->end = mem[i + 2]->end;
				merge_info_to_first(mem[i], mem[i + 1]);
				merge_info_to_first(mem[i], mem[i + 2]);
				uint32_t index = i + 2;
				for (int j = i + 3; j < mem.size(); j++){
					int32_t gap_now = mem[j]->start - mem[i]->end;
					//cout << gap << " " << gap_now << endl;
					if (gap_now == gap){
						mem[i]->end = mem[j]->end;
						merge_info_to_first(mem[i], mem[j]);
						index = j;
					}
					else{
						break;
					}
				}

				DEBUG_PRINT(("app_pc %x merged indexes from %d to %d\n", app_pc, i, index), 5);
				for (int j = i + 1; j <= index; j++){
					delete mem[i + 1];
					mem.erase(mem.begin() + i + 1);
				}

				ret = true;


			}
		}
	}

	DEBUG_PRINT(("link_mem_regions_greedy - done\n"), 4);

	return ret;
}

/* this is to coalesce regions into big input output chuncks; this is inference and may be wrong
* this is kind of unsound defragmentation
*/
bool link_mem_regions(vector<mem_info_t *> &mem, uint32_t app_pc){

	struct dp_info{
		uint32_t amount;
		uint32_t gap;
		uint32_t connection;

	};

	sort(mem.begin(), mem.end(), compare_mem_regions);

	/*can use a dynamic programming algo with optimizing for the most number of connected regions*/

	/* following is a suboptimal greedy solution */

	if (mem.size() > 1){


		dp_info ** dp = new dp_info *[mem.size()];
		for (int i = 0; i < mem.size(); i++){
			dp[i] = new dp_info[mem.size()];
		}

		/* init */
		for (int i = 0; i < mem.size(); i++){
			for (int j = 0; j < mem.size(); j++){
				dp[i][j].amount = 0;
			}
		}
		
		/* get the connectivity */
		for (int i = 0; i < mem.size(); i++){
			for (int j = 0; j <= i; j++){
				if (i == j){
					dp[i][j].gap = 0;
					dp[i][j].amount = 1;
					dp[i][j].connection = j;
				}
				else{
					int32_t gap = mem[i]->start - mem[j]->end;
					ASSERT_MSG(gap>0, ("gap should be > 0\n"));
					int index = -1;

					for (int k = 0; k <= j; k++){
						int32_t j_gap = dp[j][k].gap;
						if ((gap == j_gap)){
							index = k; break;
						}
					}

					if (index == -1) index = j;

					ASSERT_MSG(index != -1, ("check logic\n"));
					dp[i][j].amount = dp[j][index].amount + 1;
					dp[i][j].gap = gap;
					dp[i][j].connection = index;
				}
			}
		}


		

		int max_i = -1;
		int max_j = -1;
		int max_val = 0;

		/* now get the highest */
		for (int i = 0; i < mem.size(); i++){
			for (int j = 0; j < mem.size(); j++){
				if (dp[i][j].amount > max_val){
					max_val = dp[i][j].amount;
					max_i = i;
					max_j = j;
				}
			}
		}

		

		if (max_val > 2){
			/* we can now merge the regions */
			/*cout << "merging " << dec << max_val << " app_pc " << hex << app_pc <<  endl;
			cout << "gap " << dec << dp[max_i][max_j].gap << endl; */

			/*for (int i = 0; i < mem.size(); i++){
				for (int j = 0; j < mem.size(); j++){
					cout << dp[i][j].amount << "," << dp[i][j].gap << "," << dp[i][j].connection << " ";
				}
				cout << endl;
			}*/
			

			int i = max_i;
			int j = max_j;
			int temp = 0;

			vector<int> indexes;
			while (true){
				indexes.push_back(i);
				if (i == j) break;
				temp = i;
				i = j;
				j = dp[temp][j].connection;
			}

			sort(indexes.begin(), indexes.end());

			mem[indexes[0]]->end = mem[indexes[indexes.size() - 1]]->end;

			for (int i = 1; i < indexes.size(); i++){
				merge_info_to_first(mem[indexes[0]], mem[indexes[i]]);
			}
			defragment_regions(mem);

			//print_mem_layout(mem);

			return true;

		}

		return false;


	}

	return false;

}

static bool check_overlap(mem_info_t * first, mem_info_t * second){

	if (first->type == second->type){
		return  (first->end >= second->start && first->start <= second->start) ||
			(second->end >= first->start && second->start <= first->start);
	}
	else{
		return false;
	}

}

static void populate_overlapped_regions(pc_mem_region_t * pc_mem, vector<pc_mem_region_t *> &pc_mems){

	for (int k = 0; k < pc_mem->regions.size(); k++){

		mem_info_t * mem = pc_mem->regions[k];

		if (mem->direction == MEM_OUTPUT){

			for (int i = 0; i < pc_mems.size(); i++){

				if (pc_mem == pc_mems[i]) continue;

				for (int j = 0; j < pc_mems[i]->regions.size(); j++){
					
					mem_info_t * now = pc_mems[i]->regions[j];
					if ( (now->direction == MEM_INPUT) && check_overlap(mem, now) ){
						pc_mem->to_regions.push_back(pc_mems[i]);
						break;
					}

				}
			}

		}

		if (mem->direction == MEM_INPUT){

			for (int i = 0; i < pc_mems.size(); i++){

				if (pc_mem == pc_mems[i]) continue;

				for (int j = 0; j < pc_mems[i]->regions.size(); j++){

					mem_info_t * now = pc_mems[i]->regions[j];
					if ( (now->direction == MEM_OUTPUT) && check_overlap(mem, now) ){
						pc_mem->from_regions.push_back(pc_mems[i]);
						break;
					}

				}
			}

		}
	}
}


void populate_memory_dependancies(vector<pc_mem_region_t *> &regions){

	
	for (int i = 0; i < regions.size(); i++){
		populate_overlapped_regions(regions[i], regions);
	}


}


pc_mem_region_t* get_pc_mem_region(vector<pc_mem_region_t *> regions, uint32_t pc){

	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->pc == pc) return regions[i];
	}
	return NULL;

}


