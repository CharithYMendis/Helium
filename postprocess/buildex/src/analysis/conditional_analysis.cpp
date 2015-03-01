#include <vector>
#include <stdint.h>
#include <string>

#include "analysis/conditional_analysis.h"
#include "analysis/x86_analysis.h"
#include "trees/trees.h"
#include "trees/nodes.h"
#include "common_defines.h"

using namespace std;

/* here instrs are the forward instructions */
std::vector<uint32_t> find_dependant_statements(vec_cinstr &instrs, mem_regions_t * mem, std::vector<Static_Info *> static_info){

	Conc_Tree * tree = new Conc_Tree();

	for (uint64_t i = mem->start; i < mem->end; i += mem->bytes_per_pixel){
		operand_t opnd = {MEM_HEAP_TYPE,mem->bytes_per_pixel,i};
		tree->add_to_frontier(tree->generate_hash(&opnd), new Conc_Node(&opnd));
	}
	
	int amount = 0;
	vector<uint32_t> lines;
	vector<uint32_t> app_pc;
	for (int i = 0; i < instrs.size(); i++){

		cinstr_t * instr = instrs[i].first;
		rinstr_t * rinstr;
		string para = instrs[i].second->disassembly;

		rinstr = cinstr_to_rinstrs_eflags(instr, amount, para, i + 1);

		bool dependant = false;
		for (int i = 0; i < amount; i++){
			if (tree->update_dependancy_forward(&rinstr[i],instr->pc, para, i + 1)){
				dependant = true;
			}
		}

		if (dependant){
			if (find(app_pc.begin(), app_pc.end(), instr->pc) == app_pc.end()){
				app_pc.push_back(instr->pc);
			}
		}


	}

	return app_pc;

}

void update_merge_points(vec_cinstr &instrs, vector<Jump_Info *> &jumps){

	DEBUG_PRINT(("finding merge points\n"), 2);
	for (int i = 0; i < jumps.size(); i++){

		uint32_t true_line = jumps[i]->taken;
		uint32_t false_line = jumps[i]->not_taken;

		int j = 1;
		while (true){
			cinstr_t * true_c = instrs[true_line + j].first;
			cinstr_t * false_c = instrs[false_line + j].first;
			if (true_c->pc == false_c->pc){
				jumps[i]->merge_pc = true_c->pc;
				break;
			}
			j++;
		}
	}

}

vector<Jump_Info*> find_dependant_conditionals(vector<uint32_t> dep_instrs, vec_cinstr &instrs, vector<Static_Info *> &static_info){


	vector<Jump_Info *> jumps;

	for (int i = 0; i < instrs.size(); i++){

		cinstr_t * instr = instrs[i].first;

		if (is_conditional_jump_ins(instr->opcode)){

			int j = i - 1;
			cinstr_t * j_instr;
			bool found = false;
			while (!found){

				j_instr = instrs[j].first;
				uint32_t eflags = is_eflags_affected(j_instr->opcode);
				if (eflags && is_jmp_conditional_affected(instr->opcode,eflags)){


					//now check whether this instruction is in dep_instrs
					found = true;
					if (find(dep_instrs.begin(), dep_instrs.end(), j_instr->pc) != dep_instrs.end()){

						/* now check whether the instruction is already listed */
						bool jump_found = false;
						Jump_Info* jump_cinstr = NULL;
						for (int k = 0; k < jumps.size(); k++){
							if (jumps[k]->jump_pc == instr->pc){
								jump_found = true; 
								jump_cinstr = jumps[k]; break;
							}
						}
						
						if (!jump_found){
							Jump_Info* jump_instr = new Jump_Info;
							jump_instr->jump_pc = instr->pc;
							jump_instr->cond_pc = j_instr->pc; 
							if (is_branch_taken(instr->opcode, instr->eflags)){ /* jump is taken */
								jump_instr->target_pc = instrs[i + 1].first->pc;
								jump_instr->fall_pc = 0;
								jump_instr->taken = i;
								jump_instr->not_taken = 0;
							}
							else{
								jump_instr->fall_pc = instrs[i + 1].first->pc;
								jump_instr->target_pc = 0;
								jump_instr->not_taken = i;
								jump_instr->taken = 0;
							}
							jumps.push_back(jump_instr);
						}
						else{
							if (is_branch_taken(instr->opcode, instr->eflags)){
								if (jump_cinstr->target_pc == 0){
									jump_cinstr->target_pc = instrs[i + 1].first->pc;
									jump_cinstr->taken = i;
								}
								else{
									ASSERT_MSG((jump_cinstr->target_pc == instrs[i+1].first->pc), ("ERROR: inconsistency target %d\n", i+ 1));
								}
							}
							else{
								if (jump_cinstr->fall_pc == 0){
									jump_cinstr->fall_pc = instrs[i + 1].first->pc;
									jump_cinstr->not_taken = i;
								}
								else{
									ASSERT_MSG((jump_cinstr->fall_pc == instrs[i + 1].first->pc), ("ERROR: inconsistency fall %d\n", i + 1 ));
								}
							}

						}


					}
				}

				j--;
				ASSERT_MSG((j>=0), ("ERROR: couldn't find a ins which set eflags\n"));
			}

		}
	}

	update_merge_points(instrs, jumps);

	return jumps;


}

Static_Info * get_instruction_info(vector<Static_Info *> instr, uint32_t pc){

	for (int i = 0; i < instr.size(); i++){
		if (pc == instr[i]->pc){
			return instr[i];
		}
	}
	return NULL;
}

void populate_conditional_instructions(vector<Static_Info *> &static_info, vector<Jump_Info *> jumps){

	for (int i = 0; i < static_info.size(); i++){

		uint32_t pc = static_info[i]->pc;

		for (int k = 0; k < jumps.size(); k++){

			//sbb, adc etc.
			//cout << pc << endl;
			if ( (pc == jumps[k]->jump_pc) && (jumps[k]->target_pc == jumps[k]->fall_pc) && (jumps[k]->target_pc == jumps[k]->merge_pc)){
				static_info[i]->conditionals.push_back(make_pair(jumps[k], true));
				DEBUG_PRINT(("WARNING: added a non jump sbb/adc\n"), 2);
			}

			if (jumps[k]->target_pc < jumps[k]->fall_pc) continue; //this is a backward jump? we are only considering fwd jumps

			if (pc >= jumps[k]->fall_pc && pc < jumps[k]->target_pc){
				static_info[i]->conditionals.push_back(make_pair(jumps[k], false));
			}
			else if (pc >= jumps[k]->target_pc && pc < jumps[k]->merge_pc){
				static_info[i]->conditionals.push_back(make_pair(jumps[k], true));
			}
		}
	}	
}

