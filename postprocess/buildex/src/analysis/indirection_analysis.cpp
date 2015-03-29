#include <vector>

#include "analysis/indirection_analysis.h"
#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"

#include "memory/memregions.h"

#include "trees/trees.h"
#include "trees/nodes.h"

#include "common_defines.h"

using namespace std;


vector< vector<uint32_t> > find_dependant_statements_with_indirection(vec_cinstr &instrs, vector<mem_regions_t *> mem, std::vector<Static_Info *> static_info, vector<uint32_t> start_points){

	DEBUG_PRINT(("finding dependant statements direct and indirect\n"), 2);

	vector<uint32_t> direct_app_pc;
	vector<uint32_t> indirect_app_pc;

	uint32_t start = 0;
	uint32_t end = 0;

	for (int i = 0; i < start_points.size(); i++){

		if(i != start_points.size() - 1) end = start_points[i + 1];
		else end = instrs.size();

		Conc_Tree * indirect_tree = new Conc_Tree();
		Conc_Tree * direct_tree = new Conc_Tree();

		DEBUG_PRINT(("start - %d  end - %d\n", start, end), 2);

		for (int j = 0; j < mem.size(); j++){
			for (uint64_t i = mem[j]->start; i < mem[j]->end; i += mem[j]->bytes_per_pixel){
				operand_t opnd = { MEM_HEAP_TYPE, mem[j]->bytes_per_pixel, i };
				direct_tree->add_to_frontier(direct_tree->generate_hash(&opnd), new Conc_Node(&opnd));
				indirect_tree->add_to_frontier(indirect_tree->generate_hash(&opnd), new Conc_Node(&opnd));
			}
		}

		int amount = 0;

		for (int i = start; i < end; i++){

			cinstr_t * instr = instrs[i].first;
			rinstr_t * rinstr;
			string para = instrs[i].second->disassembly;

			rinstr = cinstr_to_rinstrs_eflags(instr, amount, para, i + 1);

			bool direct_dependant = false;
			bool indirect_dependant = false;

			int value = 0;

			for (int j = 0; j < amount; j++){
				if (direct_tree->update_dependancy_forward(&rinstr[j], instr->pc, para, i + 1)){
					direct_dependant = true;
					value = j;
				}
				if (indirect_tree->update_dependancy_forward_with_indirection(&rinstr[j], instr->pc, para, i + 1)){
					indirect_dependant = true;
					value = j;
					//print_cinstr(instr);
					//print_rinstrs(&rinstr[j], 1);
				}
			}

			if (direct_dependant){
				if (find(direct_app_pc.begin(), direct_app_pc.end(), instr->pc) == direct_app_pc.end()){
					direct_app_pc.push_back(instr->pc);
					print_rinstrs(log_file,&rinstr[value], 1);
					LOG(log_file, "direct " << instr->pc << endl);
				}
			}
			if (indirect_dependant){
				if (find(indirect_app_pc.begin(), indirect_app_pc.end(), instr->pc) == indirect_app_pc.end()){
					indirect_app_pc.push_back(instr->pc);
					print_rinstrs(log_file, &rinstr[value], 1);
					LOG(log_file, "indirect " << instr->pc << endl);
				}
			}


		}

		start = end;

	}


	vector<vector<uint32_t> > ret;
	ret.push_back(direct_app_pc);
	ret.push_back(indirect_app_pc);

	return ret;

}

void populate_dependant_indireciton(vector<Static_Info *> &static_info, vector<uint32_t> dep){

	DEBUG_PRINT(("populating indirection instructions\n"), 2);
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