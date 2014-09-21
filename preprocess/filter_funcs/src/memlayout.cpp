#include "meminfo.h"
#include "moduleinfo.h"
#include <fstream>
#include <vector>
#include "utilities.h"
#include "memlayout.h"

using namespace std;


/* read the files and get the mem information recorded */

vector<pc_mem_region_t *> get_mem_regions_from_memtrace(vector<ifstream *> &memtrace, moduleinfo_t * head){

	vector<pc_mem_region_t *> pc_mems;

	for (int i = 0; i < memtrace.size(); i++){
		uint32_t count = 0;
		while (!(*memtrace[i]).eof()){

			string line;
			getline((*memtrace[i]), line);

			if (!line.empty()){
				vector<string> tokens = split(line, ',');

				mem_input_t * input = new mem_input_t;

				moduleinfo_t * module = find_module(head, strtoull(tokens[0].c_str(), NULL, 16));

				if (module != NULL){
					input->module = module->name;
					input->pc = strtoul(tokens[1].c_str(), NULL, 16);
					input->write = tokens[2][0] - '0';
					input->stride = atoi(tokens[3].c_str());
					input->type = MEM_HEAP_TYPE;
					input->mem_addr = strtoull(tokens[4].c_str(), NULL, 16);

					update_mem_regions(pc_mems, input);
				}
				else{
					DEBUG_PRINT(("WARNING: cannot find a module for %s address", tokens[0].c_str()), 1);
				}

				delete input;
			}
			print_progress(&count, 100000);
		}
		
		DEBUG_PRINT(("files %d/%d is read\n", i + 1, memtrace.size()), 5);

	}

	DEBUG_PRINT(("defragmenting and updating strides....\n"), 5);

	postprocess_mem_regions(pc_mems);

	DEBUG_PRINT(("defragmenting and updating strides done\n"), 5);

	return pc_mems;

}

/* now create the composition layout */
func_composition_t * find_func_composition(vector<func_composition_t *> &funcs, string module, uint32_t addr){
	for (int i = 0; i < funcs.size(); i++){
		if (funcs[i]->module_name.compare(module) == 0 && addr == funcs[i]->func_addr){
			return funcs[i];
		}
	}
	return NULL;
}


vector<func_composition_t *> create_func_composition_func(vector<pc_mem_region_t *> &regions, moduleinfo_t * head){

	vector<func_composition_t *>  funcs;

	for (int i = 0; i < regions.size(); i++){
		moduleinfo_t * module = find_module(head, regions[i]->module);
		if (module != NULL){
			funcinfo_t * func = find_func_app_pc(module, regions[i]->pc);
			if (func != NULL){
				func_composition_t * comp = find_func_composition(funcs, regions[i]->module, func->start_addr);
				if (comp == NULL){
					comp = new func_composition_t;
					comp->module_name = regions[i]->module;
					comp->func_addr = func->start_addr;
					funcs.push_back(comp);
				}

				comp->region.push_back(regions[i]);
			}
		}
	}

	return funcs;
}

vector<func_composition_t *> create_func_composition_wo_func(vector<pc_mem_region_t *> &regions, moduleinfo_t * head){
	
	vector<func_composition_t *> comp;

	for (int i = 0; i < regions.size(); i++){
		moduleinfo_t  * module = find_module(head, regions[i]->module);
		cout << hex << regions[i]->pc << endl;
		uint32_t start_addr =  get_probable_func(head, module, regions[i]->pc);
		cout << hex << start_addr << endl;
		func_composition_t * new_func = find_func_composition(comp, regions[i]->module, start_addr);
		if (new_func == NULL){
			new_func = new func_composition_t;
			new_func->func_addr = start_addr;
			new_func->module_name = regions[i]->module;
			comp.push_back(new_func);
		}
		new_func->region.push_back(regions[i]);
		
	}

	return comp;

}


void print_filter_file(ofstream &file, func_composition_t * &funcs){

}

void print_app_pc_info(ofstream &file, func_composition_t * &funcs){

	file << funcs->module_name << endl;
	file << funcs->func_addr << endl;

}

void print_app_pc_file(ofstream &file, vector<pc_mem_region_t *> &pc_mems){

	vector<string> modules;

	for (int i = 0; i < pc_mems.size(); i++){
		if (find(modules.begin(), modules.end(), pc_mems[i]->module) == modules.end()){
			modules.push_back(pc_mems[i]->module);
		}
	}

	file << modules.size() << endl;

	for (int i = 0; i < modules.size(); i++){

		file << "\"" << modules[i] << "\"" << endl;

		uint32_t amount = 0;
		for (int j = 0; j < pc_mems.size(); j++){
			if (pc_mems[j]->module == modules[i]){
				amount++;
			}
		}

		file << amount << endl;
		for (int j = 0; j < pc_mems.size(); j++){
			if (pc_mems[j]->module == modules[i]){
				file << dec << pc_mems[j]->pc << endl;
			}
		}

	}

}