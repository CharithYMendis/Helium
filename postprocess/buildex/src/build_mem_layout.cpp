/* we will try to recreate the mem layout from the execution traces */
#include <fstream>
#include <iostream>
#include "build_mem_layout.h"
#include <vector>
#include "expression_tree.h"
#include "fileparser.h"
#include "defines.h"
#include "stdio.h"


struct cregions{

	cregions * prev;
	mem_info_t * current;
	cregions * next;

	cregions(){
		prev = NULL;
		next = NULL;
	}

};



using namespace std;

void update_mem_regions(operand_t * opnd, vector<mem_info_t *> &mem_info, bool write);
void defragment_regions(vector<mem_info_t *> &mem_info);

void create_mem_layout(std::ifstream &in, vector<mem_info_t *> &mem_info){

	while (!in.eof()){
		cinstr_t * instr = get_next_from_ascii_file(in);

		if (instr != NULL){
			for (int i = 0; i < instr->num_srcs; i++){
				update_mem_regions(&instr->srcs[i], mem_info, false);
			}
			for (int i = 0; i < instr->num_dsts; i++){
				update_mem_regions(&instr->dsts[i], mem_info, true);
			}
		}

		delete instr;

		//defragment_regions(mem_info); /* this could be lifted out? */
	}

	defragment_regions(mem_info);


}


void create_mem_layout(vector<cinstr_t * > &instrs, vector<mem_info_t *> &mem_info){

}

/* should we record the smallest stride? or the most frequent stride (width) as the data element width
*/

int get_most_probable_stride(vector<pair<uint, uint> > &strides){
	
	int stride = -1;
	int max_freq = -1;

	for (int i = 0; i < strides.size(); i++){
		if (max_freq < strides[i].second){
			max_freq = strides[i].second;
			stride = strides[i].first;
		}
	}

	return stride;

}

void update_stride(vector<pair<uint, uint> > &strides, uint stride){

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

void update_stride_from_vector(vector<pair<uint, uint> > &strides, vector<pair<uint, uint> > &old){
	for (int i = 0; i < old.size(); i++){
		
		uint old_stride = old[i].first;
		uint old_freq = old[i].second;
		bool updated = false;
		for (int j = 0; j < strides.size(); j++){
			if (strides[i].first == old_stride){
				strides[i].second += old_freq;
				updated = true;
				break;
			}
		}

		if (!updated){
			strides.push_back(old[i]);
		}
	}
}


void update_mem_regions(operand_t * opnd, vector<mem_info_t *> &mem_info, bool write){

	if ((opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE)){
		uint64 addr = opnd->value;
		uint stride = opnd->width;
		bool merged = false;

		uint32_t direction = 0;
		vector<pair<uint, uint> > stride_acc;

		for (int i = 0; i < mem_info.size(); i++){
			mem_info_t * info = mem_info[i];
			if (info->type == opnd->type){

				/* is the address with in range?*/
				if ((addr >= info->start) && (addr + stride <= info->end)){
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
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
				else if ( (addr + stride >= info->start) && (addr < info->start) ){
					

					info->start = addr;
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
					info->direction |= direction;
					update_stride(info->stride_freqs, stride);
					merged = true;
				}

				/* can we append this to the end of the memory region? */
				else if ( (addr <= info->end) && (addr + stride > info->end) ){
					
					info->end = addr + stride;
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
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
			new_region->direction = write ? MEM_OUTPUT : MEM_INPUT;
			new_region->direction |= direction;
			new_region->type = opnd->type;
			update_stride_from_vector(new_region->stride_freqs, stride_acc);
			update_stride(new_region->stride_freqs, stride);			
			mem_info.push_back(new_region);
		}
	}

}

/* this rountine is sound defragmentation */
void defragment_regions(vector<mem_info_t *> &mem_info){

	/*this will try to merge individual chunks of memory units info defragmented larger chunks*/
	bool finished = false;

	while (!finished){

		int start_size = mem_info.size();

		for (int i = 0; i < mem_info.size(); i++){
			
			mem_info_t * candidate = mem_info[i];
			
			for (int j = i+1; j < mem_info.size(); j++){

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
						mem_info.erase(mem_info.begin() + j--);
					}

				}

			}
		}

		int end_size = mem_info.size();

		finished = (start_size == end_size);

	}
	

}

/* this is to coalesce regions into big input output chuncks; this is inference and may be wrong 
 * this is kind of unsound defragmentation
*/
void infer_connected_regions(vector<mem_info_t * > &mem_info){

	/*
	algortihm - first find regions which are seperated with constant gaps; this may be due to various bounds in the algorithm
	*/

	vector<cregions * > heads; /* this will carry the heads of all supposedly connected areas */

	/* yet another n^2 algorithm - I am not trying to be fancy like trying for dynamic programming etc. */




}


void random_dest_select(vector<mem_info_t *> &mem_info, uint64_t * dest, uint32_t * stride){

	/* select a random location to track using the heap regions (actually non stack regions) */
	for (int i = 0; i < mem_info.size(); i++){
		if ( (mem_info[i]->type == MEM_HEAP_TYPE) && ( (mem_info[i]->direction & MEM_OUTPUT) == MEM_OUTPUT) ){


			uint32_t probable_stride = get_most_probable_stride(mem_info[i]->stride_freqs);

			uint32_t size = (mem_info[i]->end - mem_info[i]->start) / (probable_stride);

			uint32_t random = size/2 + 10;

			/* here we are returning the first heap block we get; we can also randomize this */
			*dest = mem_info[i]->start + random * probable_stride;
			*stride = probable_stride;

			DEBUG_PRINT(("size, random, start - %d, %d, %llu\n", size, random, mem_info[i]->start), 3);

			return;

		}
	}

}

void print_mem_layout(vector<mem_info_t *> &mem_info){
	for (int i = 0; i < mem_info.size(); i++){
		mem_info_t * info = mem_info[i];

		/* print out the type */
		IF_PRINT((info->type == MEM_HEAP_TYPE), ("type - heap\n"));
		IF_PRINT((info->type == MEM_STACK_TYPE), ("type - stack\n"));

		/* print out the direction */
		IF_PRINT((info->direction & MEM_INPUT) == MEM_INPUT, ("dir - input\n"));
		IF_PRINT((info->direction & MEM_OUTPUT) == MEM_OUTPUT, ("dir - output\n"));

		printf("start - %llu\n", info->start);
		printf("end - %llu\n", info->end);

		uint stride = get_most_probable_stride(info->stride_freqs);

		printf("stride (most) - %d\n", stride);

		for (int i = 0; i < info->stride_freqs.size(); i++){
			printf("stride - %d, freq - %d\n", info->stride_freqs[i].first, info->stride_freqs[i].second);
		}

		printf("estimated size - %d\n", (info->end - info->start) / (stride));
		printf("-------------------------------------\n");

	}
}

