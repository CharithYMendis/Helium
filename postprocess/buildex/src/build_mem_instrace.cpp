/* we will try to recreate the mem layout from the execution traces */
#include <fstream>
#include <iostream>
#include "build_mem_instrace.h"
#include <vector>
#include "fileparser.h"
#include "defines.h"


using namespace std;


void create_mem_layout(std::ifstream &in, vector<mem_info_t *> &mem_info){

	while (!in.eof()){
		cinstr_t * instr = get_next_from_ascii_file(in);
		mem_input_t * input = new mem_input_t;

		if (instr != NULL){

			for (int i = 0; i < instr->num_srcs; i++){
				if (instr->srcs[i].type == MEM_HEAP_TYPE || instr->srcs[i].type == MEM_STACK_TYPE){
					input->mem_addr = instr->srcs[i].value;
					input->stride = instr->srcs[i].width;
					input->write = false;
					input->type = instr->srcs[i].type;
					update_mem_regions(mem_info,input);
				}
			}
			for (int i = 0; i < instr->num_dsts; i++){
				if (instr->dsts[i].type == MEM_HEAP_TYPE || instr->dsts[i].type == MEM_STACK_TYPE){
					input->mem_addr = instr->dsts[i].value;
					input->stride = instr->dsts[i].width;
					input->write = true;
					update_mem_regions(mem_info, input);
				}
			}
			
		}

		delete instr;
		delete input;
	}

	postprocess_mem_regions(mem_info);


}


/* only app_pc based pc_mem_region recording is done here - if needed implement the module based recording */

void create_mem_layout(std::ifstream &in, vector<pc_mem_region_t *> &mem_info){

	while (!in.eof()){
		cinstr_t * instr = get_next_from_ascii_file(in);
		mem_input_t * input = new mem_input_t;

		if (instr != NULL){

			for (int i = 0; i < instr->num_srcs; i++){
				if (instr->srcs[i].type == MEM_HEAP_TYPE || instr->srcs[i].type == MEM_STACK_TYPE){
					input->pc = instr->pc;
					input->mem_addr = instr->srcs[i].value;
					input->stride = instr->srcs[i].width;
					input->write = false;
					input->type = instr->srcs[i].type;
					update_mem_regions(mem_info, input);
				}
			}
			for (int i = 0; i < instr->num_dsts; i++){
				if (instr->dsts[i].type == MEM_HEAP_TYPE || instr->dsts[i].type == MEM_STACK_TYPE){
					input->pc = instr->pc;
					input->mem_addr = instr->dsts[i].value;
					input->stride = instr->dsts[i].width;
					input->write = true;
					update_mem_regions(mem_info, input);
				}
			}

		}

		delete instr;
		delete input;
	}

	postprocess_mem_regions(mem_info);

}


void create_mem_layout(vector<cinstr_t * > &instrs, vector<mem_info_t *> &mem_info){

}


void create_mem_layout(vector<cinstr_t * > &instrs, vector<pc_mem_region_t *> &pc_mems){

}

