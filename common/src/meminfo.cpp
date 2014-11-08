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
	
	for (int i = 0; i < pc_mems.size(); i++){
		if (!pc_mems[i]->module.empty()){
			file << "module name - " << pc_mems[i]->module.c_str() << endl;
		}
		file << "app_pc - " << hex << pc_mems[i]->pc << " mem regions - " << pc_mems[i]->regions.size() << endl;
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

	DEBUG_PRINT(("print_mem_layout(mem_info) \n"), 4);

	for (int i = 0; i < mem_info.size(); i++){
		mem_info_t * info = mem_info[i];

		/* print out the type */
		if (info->type == MEM_HEAP_TYPE) file << "type - heap" << endl;
		if (info->type == MEM_STACK_TYPE) file << "type - stack" << endl;


		/* print out the direction */
		if ((info->direction & MEM_INPUT) == MEM_INPUT) file << "dir - input" << endl;
		if ((info->direction & MEM_OUTPUT) == MEM_OUTPUT) file << "dir - output\n" << endl;

		file << "start - " << hex << info->start << endl;
		file << "end - " << hex << info->end << endl;

		uint stride = get_most_probable_stride(info->stride_freqs);

		file << "stride (most) - " << dec << stride << endl;

		for (int i = 0; i < info->stride_freqs.size(); i++){
			file << "stride - " << dec << info->stride_freqs[i].first << ", freq - " << info->stride_freqs[i].second << endl;
		}

		file << "estimated size - " << dec << (info->end - info->start) / (stride) << endl;
		file << "--------------------------------------------------------------------------" << endl;

	}

	DEBUG_PRINT(("print_mem_layout(mem_info) - done\n"), 4);
}

bool compare_mem_regions(mem_info_t * first, mem_info_t * second){

	return (first->start <= second->start);

}

/*
* this is a greedy (non-optimal) coalescing of mem regions
*/

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


