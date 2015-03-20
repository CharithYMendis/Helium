#include "localize.h"
#include "moduleinfo.h"
#include "meminfo.h"

using namespace std;

vector<funcinfo_t *> get_all_valid_functions(moduleinfo_t * module){


	vector<funcinfo_t *> funcs;

	while (module != NULL){
		for (int i = 0; i < module->funcs.size(); i++){
			if (module->funcs[i]->start_addr != 0){
				funcs.push_back(module->funcs[i]);
			}
		}
		module = module->next;
	}


	return funcs;


}


vector<mem_info_t *> get_function_pc_mem_regions(funcinfo_t * func, vector<pc_mem_region_t *> &regions){


	vector<pc_mem_region_t *> func_regions;

	for (int i = 0; i < func->bbs.size(); i++){
		bbinfo_t * bbinfo = func->bbs[i];
		for (int j = 0; j < regions.size(); j++){
			if ( (regions[j]->pc >= bbinfo->start_addr) && (regions[j]->pc <= bbinfo->start_addr + bbinfo->size) ){
				func_regions.push_back(regions[j]);
				break;
			}
		}
	}

	link_mem_regions(func_regions, GREEDY);
	return extract_mem_regions(func_regions);

}


void get_mem_dependant_functions(vector<funcinfo_t *> &funcs, funcinfo_t * src_func, vector<pc_mem_region_t *> &regions, vector<mem_input_t *> &mem_trace){
	
	/* need to look at the memtrace to get the dependancy information */



}