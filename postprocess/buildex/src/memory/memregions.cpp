
#include <iostream>
#include <string>

#include "memory/memregions.h"
#include "analysis/x86_analysis.h"
#include "common_defines.h"

#include "meminfo.h"
#include "utilities.h"
#include "utility\defines.h"

using namespace std;

/* extracting random locations from the mem regions */
vector<uint64_t> get_nbd_of_random_points(vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride){

	/*ok we need find a set of random locations */
	mem_regions_t * random_mem_region = get_random_output_region(image_regions);
	uint64_t mem_location = get_random_mem_location(random_mem_region, seed);
	DEBUG_PRINT(("random mem location we got - %llx\n", mem_location), 1);
	*stride = random_mem_region->bytes_per_pixel;

	vector<uint64_t> nbd_locations;
	vector<int> base = get_mem_position(random_mem_region, mem_location);
	nbd_locations.push_back(mem_location);

	cout << "base : " << endl;
	for (int j = 0; j < base.size(); j++){
		cout << base[j] << ",";
	}
	cout << endl;

	//get a nbd of locations - diagonally choose pixels
	int boundary = (int)ceil((double)(random_mem_region->dimensions + 2) / 2.0);
	DEBUG_PRINT(("boundary : %d\n", boundary), 1);
	int count = 0;
	for (int i = -boundary; i <= boundary; i++){

		if (i == 0) continue;
		vector<int> offset;
		uint32_t affected = count % random_mem_region->dimensions;
		for (int j = 0; j < base.size(); j++){
			if (j == affected){
				if (base[j] + i < 0 || base[j] + i >= random_mem_region->extents[j]) offset.push_back(0);
				else offset.push_back(i);
				//offset.push_back(i);
			}
			else offset.push_back(0);
		}

		cout << "offset" << endl;
		for (int j = 0; j < offset.size(); j++){
			cout << offset[j] << ",";
		}
		cout << endl;

		bool success;
		mem_location = get_mem_location(base, offset, random_mem_region, &success);
		cout << hex << "dest - " << mem_location << dec << endl;
		ASSERT_MSG(success, ("ERROR: memory location out of bounds\n"));

		nbd_locations.push_back(mem_location);
		count++;
	}

	return nbd_locations;


}

vector<uint64_t> get_nbd_of_random_points_2(vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride){

	mem_regions_t * random_mem_region = get_random_output_region(image_regions);
	cout << hex << random_mem_region->start << endl;
	uint64_t mem_location = get_random_mem_location(random_mem_region, seed);
	DEBUG_PRINT(("random mem location we got - %llx\n", mem_location), 1);
	*stride = random_mem_region->bytes_per_pixel;

	vector<uint64_t> nbd_locations;
	vector<int> base = get_mem_position(random_mem_region, mem_location);
	nbd_locations.push_back(mem_location);

	cout << "base : " << endl;
	for (int j = 0; j < base.size(); j++){
		cout << base[j] << ",";
	}
	cout << endl;

	//get a nbd of locations - diagonally choose pixels
	int boundary = (int)ceil((double)(random_mem_region->dimensions + 2) / 2.0);
	DEBUG_PRINT(("boundary : %d\n", boundary), 1);
	int count = 0;

	int * val = new int[random_mem_region->dimensions];
	for (int i = 0; i < random_mem_region->dimensions; i++){
		val[i] = 0;
	}

	for (int i = -boundary; i <= boundary; i++){

		//if (i == 0) continue;
		vector<int> offset;
		uint32_t affected = count % random_mem_region->dimensions;
		if (count == 4){
			cout << "hello" << endl;
			count++; 
			val[affected]++;
			continue;
		}
		for (int j = 0; j < base.size(); j++){
			if (j == affected){
				val[j]++;
				if (val[j] + base[j] < random_mem_region->extents[j] && val[j] + base[j] > 0){
					offset.push_back(val[j]);
				}
				else{
					offset.push_back(0);
				}
				
			}
			else offset.push_back(0);
		}

		cout << "offset" << endl;
		for (int j = 0; j < offset.size(); j++){
			cout << offset[j] << ",";
		}
		cout << endl;

		bool success;
		mem_location = get_mem_location(base, offset, random_mem_region, &success);
		cout << hex << "dest - " << mem_location << dec << endl;
		ASSERT_MSG(success, ("ERROR: memory location out of bounds\n"));

		nbd_locations.push_back(mem_location);
		count++;
	}

	return nbd_locations;



}


uint32_t get_region_size(mem_regions_t * region){

	uint32_t size = 1;
	for (int i = 0; i < region->dimensions; i++){
		size *= region->extents[i];
	}
	return size;

}

uint64_t get_random_mem_location(mem_regions_t *  region, uint32_t seed){

	DEBUG_PRINT(("selecting a random output location now.....\n"), 2);

	srand(seed);

	uint32_t random_num = abs(rand());

	vector<int> base;
	vector<int> offset(region->dimensions, 0);

	for (int i = 0; i < region->dimensions; i++){
		base.push_back(random_num % region->extents[i]);
		cout << dec << base[i] << endl;
		cout << "reg extents : " << region->extents[i] << endl;
	}

	for (int i = 0; i < region->dimensions; i++){
		cout << "strides: " << region->strides[i] << endl;
		cout << "extent: " << region->extents[i] << endl;
	}

	bool success;
	uint64_t mem_location = get_mem_location(base, offset, region, &success);

	ASSERT_MSG(success, ("ERROR: random memory location out of bounds\n"));

	return mem_location;
}

mem_regions_t* get_random_output_region(vector<mem_regions_t *> regions){

	DEBUG_PRINT(("selecting a random output region now.......\n"), 2);

	/*get the number of intermediate and output regions*/
	uint32_t no_regions = 0;
	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->direction == MEM_INTERMEDIATE || regions[i]->direction == MEM_OUTPUT){
			no_regions++;
		}
	}


	uint32_t random = rand() % no_regions;
	no_regions = 0;

	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->direction == MEM_INTERMEDIATE || regions[i]->direction == MEM_OUTPUT){
			if (no_regions == random){
				DEBUG_PRINT(("random output region seleted\n"), 1);
				return regions[i];
			}
			no_regions++;
		}
	}

	return NULL; /*should not reach this point*/


}

/* abstracting memory locations from mem_regions */
uint64_t get_mem_location(vector<int> base, vector<int> offset, mem_regions_t * mem_region, bool * success){

	ASSERT_MSG((base.size() == mem_region->dimensions), ("ERROR: dimensions dont match up\n"));

	for (int i = 0; i < base.size(); i++){
		base[i] += offset[i];
	}

	for (int i = 0; i < base.size(); i++){
		if (base[i] >= mem_region->extents[i]){
			*success = false;
			return 0;
		}
	}

	*success = true;


	uint64_t ret_addr;
	if (mem_region->start < mem_region->end){
		ret_addr = mem_region->start;
		for (int i = 0; i < base.size(); i++){
			ret_addr += mem_region->strides[i] * base[i];
		}
	}
	else{
		ret_addr = mem_region->start;
		for (int i = 0; i < base.size(); i++){
			ret_addr -= mem_region->strides[i] * base[i];
		}
	}

	return ret_addr;


}

vector<int> get_mem_position(mem_regions_t * mem_region, uint64_t mem_value){

	vector<int> pos;
	vector<int> r_pos;

	/* dimensions would always be width dir(x), height dir(y) */

	/*get the row */

	uint64_t offset;

	if (mem_region->start < mem_region->end){
		offset = mem_value - mem_region->start;
	}
	else{
		offset = mem_region->start - mem_value;
	}
	//uint64_t offset = mem_region->end - mem_value;

	//cout << "mem position: " << dec << offset << " start " << mem_region->start << " end " << mem_region->end << " value " << mem_value << endl;
 

	for (int i = mem_region->dimensions - 1; i >= 0; i--){
		int point_offset = offset / mem_region->strides[i];
		if (point_offset >= mem_region->extents[i]){ point_offset = -1; }
		r_pos.push_back(point_offset);
		offset -= point_offset * mem_region->strides[i];
		//cout << offset << endl;
	}

	pos.assign(r_pos.rbegin(), r_pos.rend());

	return pos;

}

mem_regions_t * get_mem_region(uint64_t value, vector<mem_regions_t *> &mem_regions){

	for (int i = 0; i < mem_regions.size(); i++){
		if (mem_regions[i]->start < mem_regions[i]->end){
			if ((mem_regions[i]->start <= value) && (mem_regions[i]->end >= value)){
				return mem_regions[i];
			}
		}
		else{
			if ((mem_regions[i]->start >= value) && (mem_regions[i]->end <= value)){
				return mem_regions[i];
			}
		}
	}

	return NULL;

}

bool is_within_mem_region(mem_regions_t* mem, uint64_t value){

	if (mem->start < mem->end){
		return (value >= mem->start) && (value <= mem->end);
	}
	else{
		return (value >= mem->end) && (value <= mem->start);
	}
}

/* printing functions - debug */
void print_mem_regions(ostream &file, mem_regions_t * region){

	file << "bytes per color = " << region->bytes_per_pixel << endl;


	file << "start = " << region->start << endl;
	file << "end = " << region->end << endl;


	file << "type = ";
	switch (region->direction){
	case MEM_INPUT:  file << "image input" << endl; break;
	case MEM_OUTPUT: file << "image output" << endl; break;
	case MEM_INTERMEDIATE: file << "image intermediate" << endl; break;
	}


	file << "extents : " << endl;
	for (int i = 0; i < region->dimensions; i++){
		file << region->extents[i] << endl;
	}

	file << "strides : " << endl;
	for (int i = 0; i < region->dimensions; i++){
		file << region->strides[i] << endl;
	}

	file << "padding : " << endl;
	for (int i = 0; i < region->padding_filled; i++){
		file << region->padding[i] << endl;
	}

	file << "name = " << region->name << endl;

	file << "-------------------------------------------------------------------" << endl;


}

void print_mem_regions(ostream &file, vector<mem_regions_t *> regions){

	file << "------------------------------MEM REGIONS ----------------------------" << endl;

	for (int i = 0; i < regions.size(); i++){
		print_mem_regions(file,regions[i]);
	}

}


void remove_possible_stack_frames(vector<pc_mem_region_t *> &pc_mem, vector<mem_info_t *> &mem, vector<Static_Info *> &info, vec_cinstr &instrs){

	DEBUG_PRINT(("removing stack frames and out of scope mem infos\n"), 2);
	DEBUG_PRINT(("mem info size - %d\n", mem.size()), 2);

	for (int i = 0; i < mem.size(); i++){

		bool found = false;

		for (int j = 0; j < pc_mem.size(); j++){
			vector<mem_info_t *> mem_info = pc_mem[j]->regions;
			for (int k = 0; k < mem_info.size(); k++){
				if (is_overlapped(mem[i]->start, mem[i]->end, mem_info[k]->start, mem_info[k]->end)){
					Static_Info * static_info = get_static_info(info, pc_mem[j]->pc);
					if (static_info->example_line == -1) continue;
					cinstr_t * instr = instrs[static_info->example_line].first;
				
					if ((mem_info[k]->direction & MEM_OUTPUT) == MEM_OUTPUT){
						for (int dsts = 0; dsts < instr->num_dsts; dsts++){
							operand_t opnd = instr->dsts[dsts];
							if (opnd.type == MEM_HEAP_TYPE || opnd.type == MEM_STACK_TYPE){

								for (int addr = 0; addr < 2; addr++){
									if (opnd.addr[addr].value != 0){
										if (mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RBP &&
											mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RSP){
											found = true; break;
										}
									}
								}
							}
						}
					}

					if ((mem_info[k]->direction & MEM_INPUT) == MEM_INPUT){
						for (int srcs = 0; srcs < instr->num_srcs; srcs++){
							operand_t opnd = instr->srcs[srcs];
							if (opnd.type == MEM_HEAP_TYPE || opnd.type == MEM_STACK_TYPE){

								for (int addr = 0; addr < 2; addr++){
									if (opnd.addr[addr].value != 0){
										if (mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RBP &&
											mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RSP){
											found = true; break;
										}
									}
								}

							}
						}
					}


				}
				if (found) break;
			}
			if (found) break;
		}

		if (!found){
			mem.erase(mem.begin() + i--);
		}

	}

	DEBUG_PRINT(("mem info size after - %d\n", mem.size()), 2);

}

void mark_possible_buffers(vector<pc_mem_region_t *> &pc_mem, vector<mem_regions_t *> &mem_regions, vector<Static_Info *> &info, vec_cinstr &instrs){

	for (int i = 0; i < mem_regions.size(); i++){

		bool found = false;

		for (int j = 0; j < pc_mem.size(); j++){
			bool printed = true;
			vector<mem_info_t *> mem_info = pc_mem[j]->regions;
			for (int k = 0; k < mem_info.size(); k++){
				if (is_overlapped(mem_regions[i]->start, mem_regions[i]->end, mem_info[k]->start, mem_info[k]->end)){
					Static_Info * ind = get_static_info(info, pc_mem[j]->pc);
					if (ind->example_line == -1) continue;
					cinstr_t * instr = instrs[ind->example_line].first;
					if (!printed){
						cout << i << " " << mem_regions[i]->start << " " << get_static_info(info, pc_mem[j]->pc)->disassembly << endl;
						print_cinstr(instr);
						cout << pc_mem[j]->pc << endl;
						printed = true;

					}

					if ( (mem_info[k]->direction & MEM_OUTPUT) == MEM_OUTPUT){
						for (int dsts = 0; dsts < instr->num_dsts; dsts++){
							operand_t opnd = instr->dsts[dsts];
							if (opnd.type == MEM_HEAP_TYPE || opnd.type == MEM_STACK_TYPE){
								
								for (int addr = 0; addr < 2; addr++){
									if (opnd.addr[addr].value != 0){
										if (mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RBP &&
											mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RSP){
												found = true; break;
										}
									}
								}
							}
						}
					}

					if ( (mem_info[k]->direction & MEM_INPUT) == MEM_INPUT){
						for (int srcs = 0; srcs < instr->num_srcs; srcs++){
							operand_t opnd = instr->srcs[srcs];
							if (opnd.type == MEM_HEAP_TYPE || opnd.type == MEM_STACK_TYPE){

								for (int addr = 0; addr < 2; addr++){
									if (opnd.addr[addr].value != 0){
										if (mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RBP &&
											mem_range_to_reg(&opnd.addr[addr]) != DR_REG_RSP){
											found = true; break;
										}
									}
								}

							}
						}
					}


				}
				if (found) break;
			}
			if (found) break;
		}

		if (!found){
			LOG(log_file, "region not found to be a buffer" << endl);
			LOG(log_file, "start : " << mem_regions[i]->start << " end : " << mem_regions[i]->end);
		}

		ASSERT_MSG(found, ("ERROR: all buffers should are captured\n"));

	}

}

uint64_t get_farthest_mem_access_point(vector<mem_regions_t *> &regions){

	uint64_t max_addr = 0;

	for (int i = 0; i < regions.size(); i++){
		if (regions[i]->start < regions[i]->end && max_addr < regions[i]->end){
			max_addr = regions[i]->end;
		}
		else if (regions[i]->start > regions[i]->end && max_addr < regions[i]->start){
			max_addr = regions[i]->start;
		}
	}

	return max_addr + 16;

}


uint32_t get_actual_size(mem_regions_t * region){

	uint32_t dims = region->dimensions;
	return region->extents[dims - 1] * region->strides[dims - 1];

}


