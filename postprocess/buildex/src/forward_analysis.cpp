#include "forward_analysis.h"
#include <vector>
#include <stdint.h>
#include "canonicalize.h"
#include "expression_tree.h"
#include "memregions.h"
#include "fileparser.h"
#include "defines.h"
#include <string>

using namespace std;

vector<uint32_t> find_dependant_statements(vec_cinstr &instrs,mem_regions_t * mem, vector<disasm_t *> disasm_vec){

	Expression_tree * tree = new Expression_tree();
	for (uint64_t i = mem->start; i < mem->end; i += mem->bytes_per_pixel){
		operand_t opnd = {MEM_HEAP_TYPE,mem->bytes_per_pixel,i};
		tree->add_to_frontier(tree->generate_hash(&opnd), new Node(&opnd));
	}
	
	int amount = 0;
	vector<uint32_t> lines;
	vector<uint32_t> app_pc;
	for (int i = 0; i < instrs.size(); i++){

		cinstr_t * instr = instrs[i].first;
		rinstr_t * rinstr;
		vector<string> disasm = get_disasm_string(disasm_vec, instr->pc);

		string para;
		if (disasm.size()>0) para = disasm[0];
		else para = "uncaptured\n";


		rinstr = cinstr_to_rinstrs_eflags(instr, amount, para, i + 1);

		//if (instr->opcode == OP_cmp) cout << para << " amount: " << amount <<  endl;

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

void update_merge_points(vec_cinstr instrs, vector<jump_info_t *> &jumps){

	DEBUG_PRINT(("finding merge points\n"), 2);
	for (int i = 0; i < jumps.size(); i++){

		uint32_t true_line = jumps[i]->trueFalse[0];
		uint32_t false_line = jumps[i]->trueFalse[1];

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

vector<jump_info_t *> find_depedant_conditionals(vector<uint32_t> dep_instrs, vec_cinstr &instrs, vector<disasm_t *> disasm){


	vector<jump_info_t *> jumps;

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
						jump_info_t * jump_cinstr = NULL;
						for (int k = 0; k < jumps.size(); k++){
							if (jumps[k]->jump_pc == instr->pc){
								jump_found = true; 
								jump_cinstr = jumps[k]; break;
							}
						}
						
						if (!jump_found){
							jump_info_t * jump_instr = new jump_info_t;
							jump_instr->jump_pc = instr->pc;
							jump_instr->cond_pc = j_instr->pc; 
							if (is_branch_taken(instr->opcode, instr->eflags)){ /* jump is taken */
								jump_instr->target_pc = instrs[i + 1].first->pc;
								jump_instr->fall_pc = 0;
								jump_instr->trueFalse[0] = i;
								jump_instr->trueFalse[1] = 0;
							}
							else{
								jump_instr->fall_pc = instrs[i + 1].first->pc;
								jump_instr->target_pc = 0;
								jump_instr->trueFalse[1] = i;
								jump_instr->trueFalse[0] = 0;
							}
							jumps.push_back(jump_instr);
						}
						else{
							if (is_branch_taken(instr->opcode, instr->eflags)){
								if (jump_cinstr->target_pc == 0){
									jump_cinstr->target_pc = instrs[i + 1].first->pc;
									jump_cinstr->trueFalse[0] = i;
								}
								else{
									ASSERT_MSG((jump_cinstr->target_pc == instrs[i+1].first->pc), ("ERROR: inconsistency target %d\n", i+ 1));
								}
							}
							else{
								if (jump_cinstr->fall_pc == 0){
									jump_cinstr->fall_pc = instrs[i + 1].first->pc;
									jump_cinstr->trueFalse[1] = i;
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

instr_info_t * get_instruction_info(vector<instr_info_t *> instr, uint32_t pc){

	for (int i = 0; i < instr.size(); i++){
		if (pc == instr[i]->pc){
			return instr[i];
		}
	}
	return NULL;
}


vector<instr_info_t *> populate_conditional_instructions(vector<disasm_t *> disasm, vector<jump_info_t *> jumps){

	vector<instr_info_t *> instrs;

	for (int i = 0; i < disasm.size(); i++){
		for (int j = 0; j < disasm[i]->pc_disasm.size(); j++){
			instr_info_t * instr = new instr_info_t;
			instrs.push_back(instr);
			
			uint32_t pc = disasm[i]->pc_disasm[j].first;
			string disasm_string = disasm[i]->pc_disasm[j].second;
			instr->pc = pc;
			instr->disasm = disasm_string;

			for (int k = 0; k < jumps.size(); k++){

				//sbb, adc etc.
				//cout << pc << endl;
				if ( (pc == jumps[k]->jump_pc) && (jumps[k]->target_pc == jumps[k]->fall_pc) && (jumps[k]->target_pc == jumps[k]->merge_pc)){
					instr->conditions.push_back(make_pair(jumps[k], true));
					DEBUG_PRINT(("WARNING: added a non jump sbb/adc\n"), 2);
				}

				if (jumps[k]->target_pc < jumps[k]->fall_pc) continue; //this is a backward jump? we are only considering fwd jumps



				if (pc >= jumps[k]->fall_pc && pc < jumps[k]->target_pc){
					instr->conditions.push_back(make_pair(jumps[k], false));
				}
				else if (pc >= jumps[k]->target_pc && pc < jumps[k]->merge_pc){
					instr->conditions.push_back(make_pair(jumps[k], true));
				}

				

			}

		}
	}


	return instrs;

	
}


inverse_table_t get_inverse_jumptable(vector<jump_info_t *> jumps){

	inverse_table_t inverse_table;

	for (int i = 0; i < jumps.size(); i++){
		bool found = false;
		for (int j = 0; j < inverse_table.size(); j++){
			
			if (inverse_table[j].first == jumps[i]->target_pc){
				found = true;
				vector<jump_info_t *> jumps_rel = inverse_table[j].second;
				if (find(jumps_rel.begin(), jumps_rel.end(), jumps[i]) == jumps_rel.end()){
					inverse_table[j].second.push_back(jumps[i]);
				}
				break;
			}

		}

		if (!found){
			vector<jump_info_t *> jump_vec;
			jump_vec.push_back(jumps[i]);
			inverse_table.push_back(make_pair(jumps[i]->target_pc, jump_vec));
		}
	}

	return inverse_table;

}

vector<jump_info_t * > lookup_inverse_table(inverse_table_t jump_table, uint32_t pc){

	vector<jump_info_t *> dummy;


	for (int i = 0; i < jump_table.size(); i++){
		uint32_t target = jump_table[i].first;

		if (pc == target){
			return jump_table[i].second;
		}
	}

	return dummy;


}


void parse_for_indirect_accesses(){

}



void find_rdom_loops(){

}

void find_indirect_storage(){

}

void find_lookup_tables(){


}



