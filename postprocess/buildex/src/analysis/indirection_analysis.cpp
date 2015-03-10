#include <vector>

#include "analysis/indirection_analysis.h"
#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"

#include "memory/memregions.h"

#include "trees/trees.h"
#include "trees/nodes.h"

using namespace std;


vector< vector<uint32_t> > find_dependant_statements_with_indirection(vec_cinstr &instrs, mem_regions_t * mem, std::vector<Static_Info *> static_info){

	Conc_Tree * indirect_tree = new Conc_Tree();
	Conc_Tree * direct_tree = new Conc_Tree();

	for (uint64_t i = mem->start; i < mem->end; i += mem->bytes_per_pixel){
		operand_t opnd = { MEM_HEAP_TYPE, mem->bytes_per_pixel, i };
		direct_tree->add_to_frontier(direct_tree->generate_hash(&opnd), new Conc_Node(&opnd));
		indirect_tree->add_to_frontier(indirect_tree->generate_hash(&opnd), new Conc_Node(&opnd));
	}

	int amount = 0;
	
	vector<uint32_t> direct_app_pc;
	vector<uint32_t> indirect_app_pc;


	for (int i = 0; i < instrs.size(); i++){

		cinstr_t * instr = instrs[i].first;
		rinstr_t * rinstr;
		string para = instrs[i].second->disassembly;

		rinstr = cinstr_to_rinstrs_eflags(instr, amount, para, i + 1);

		bool direct_dependant = false;
		bool indirect_dependant = false;

		for (int i = 0; i < amount; i++){
			if (direct_tree->update_dependancy_forward(&rinstr[i], instr->pc, para, i + 1)){
				direct_dependant = true;
			}
			if (indirect_tree->update_dependancy_forward_with_indirection(&rinstr[i], instr->pc, para, i + 1)){
				indirect_dependant = true;
			}
		}

		if (direct_dependant){
			if (find(direct_app_pc.begin(), direct_app_pc.end(), instr->pc) == direct_app_pc.end()){
				direct_app_pc.push_back(instr->pc);
			}
		}
		if (indirect_dependant){
			if (find(indirect_app_pc.begin(), indirect_app_pc.end(), instr->pc) == indirect_app_pc.end()){
				indirect_app_pc.push_back(instr->pc);
			}
		}


	}


	vector<vector<uint32_t> > ret;
	ret.push_back(direct_app_pc);
	ret.push_back(indirect_app_pc);

	return ret;

}

void populate_dependant_indireciton(vector<Static_Info *> &static_info, vector<uint32_t> dep){

	for (int i = 0; i < static_info.size(); i++){

		bool dependant = false;
		for (int j = 0; j < dep.size(); j++){
			if (static_info[i]->pc == dep[j]){
				dependant = true;
				break;
			}
		}

		if (dependant){
			static_info[i]->type = Static_Info::INPUT_DEPENDENT_INDIRECT;
		}

	}


}