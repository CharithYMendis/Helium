/* we will try to recreate the mem layout from the execution traces */
#include <fstream>
#include <iostream>
#include "build_mem_layout.h"
#include <vector>
#include "expression_tree.h"
#include "fileparser.h"
#include "defines.h"
#include "stdio.h"




using namespace std;

void update_mem_regions(operand_t * opnd, vector<mem_info_t *> &mem_info, bool write);
void defragment_regions(vector<mem_info_t *> &mem_info);

void create_mem_layout(std::ifstream &in, vector<mem_info_t *> &mem_info){

	while (!in.eof()){
		cinstr_t * instr = get_next_from_ascii_file(in);
		//print_cinstr(instr);

		if (instr != NULL){
			for (int i = 0; i < instr->num_srcs; i++){
				update_mem_regions(&instr->srcs[i], mem_info, false);
			}
			for (int i = 0; i < instr->num_dsts; i++){
				update_mem_regions(&instr->dsts[i], mem_info, true);
			}
		}
		//print_mem_layout(mem_info);

		defragment_regions(mem_info);
	}


}


void create_mem_layout(vector<cinstr_t * > &instrs, vector<mem_info_t *> &mem_info){

}

/* should we record the smallest stride? or the most frequent stride (width) as the data element width
*/


void update_mem_regions(operand_t * opnd, vector<mem_info_t *> &mem_info, bool write){

	if ((opnd->type == MEM_HEAP_TYPE) || (opnd->type == MEM_STACK_TYPE)){
		uint64 addr = opnd->value;
		uint stride = opnd->width;
		bool merged = false;

		for (int i = 0; i < mem_info.size(); i++){
			mem_info_t * info = mem_info[i];
			if (info->type == opnd->type){

				/* is the address with in range?*/
				if ((addr >= info->start) && (addr + stride <= info->end)){
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
					merged = true;
				}

				/* can we prepend this to the start of the memory region? */
				else if ( (addr + stride >= info->start) && (addr < info->start) ){
					
					ASSERT_MSG((addr + stride <= info->end), ("ERROR: accessing a huge chunk of memory compared to the initial stride\n"));
					/* some sanity warnings which should be addressed */
					if ((addr + stride >  info->start) && (addr < info->start))
						DEBUG_PRINT(("WARNING: possible unaligned access\n"), 3);
					if (stride != info->stride)
						DEBUG_PRINT(("WARNING: accessing at a different stride than before\n"), 3);

					info->start = addr;
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
					merged = true;
				}

				/* can we append this to the end of the memory region? */
				else if ( (addr <= info->end) && (addr + stride > info->end) ){

					ASSERT_MSG((addr >= info->start), ("ERROR: accessing a huge chunk of memory compared to the initial stride\n"));
					/* some sanity warnings which should be addressed */
					if ((addr <  info->end) && (addr + stride > info->end))
						DEBUG_PRINT(("WARNING: possible unaligned access\n"), 3);
					if (stride != info->stride)
						DEBUG_PRINT(("WARNING: accessing at a different stride than before\n"), 3);


					info->end = addr + stride;
					info->direction |= write ? MEM_OUTPUT : MEM_INPUT;
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
			new_region->type = opnd->type;
			new_region->stride = stride;
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

					ASSERT_MSG(!((candidate->start <= current->start) && (candidate->end >= current->end)), ("ERROR: please check update mem regions function - subset relation detected\n"));
					ASSERT_MSG(!((current->start <= candidate->start) && (current->end >= candidate->end)), ("ERROR: please check update mem regions function - subset relation detected\n"));
					
					bool merged = false;

					/* prepend to the candidate?*/
					if ((current->start < candidate->start) && (current->end >= candidate->start)){
						if (current->stride != candidate->stride)
							DEBUG_PRINT(("WARNING: accessing at a different stride in current and candidate\n"), 3);
						candidate->start = current->start;
						candidate->direction |= current->direction;
						merged = true;
					}
					/* append to candidate?*/
					else if ((current->start <= candidate->end) && (current->end > candidate->end)){
						if (current->stride != candidate->stride)
							DEBUG_PRINT(("WARNING: accessing at a different stride in current and candidate\n"), 3);
						candidate->end = current->end;
						candidate->direction |= current->direction;
						merged = true;

					}

					if (merged){
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
		printf("stride - %d\n", info->stride);

		printf("estimated size - %d\n", (info->end - info->start) / (info->stride));
		printf("-------------------------------------\n");

	}
}

