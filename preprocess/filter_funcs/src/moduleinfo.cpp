 #include "moduleinfo.h"
#include <vector>
#include <fstream>
#include "defines.h"
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include "utilities.h"

using namespace std;


void populate_func_freq(moduleinfo_t * module){

	while (module != NULL){
		
		for (int i = 0; i < module->funcs.size(); i++){
			funcinfo_t * func = module->funcs[i];
			func->freq = 0;
			for (int j = 0; j < func->bbs.size(); j++){
				if (func->bbs[j]->callees.size() > 0){
					uint32_t num_calls = 0;
					for (int k = 0; k < func->bbs[j]->callees.size(); k++){
						num_calls += func->bbs[j]->callees[k]->freq;
					}
					func->freq = num_calls;
				}
			}

			if (func->freq == 0){
				func->freq = func->bbs[0]->freq;
			}


		}
		module = module->next;

	}

}

moduleinfo_t * find_module(moduleinfo_t * head,  uint64_t start_addr){
	while (head != NULL){
		if (head->start_addr == start_addr){
			return head;
		}
		head = head->next;
	}
	return NULL;
}

moduleinfo_t * find_module(moduleinfo_t * head, string name){
	while (head != NULL){
		if (strcmp(head->name,name.c_str()) == 0){
			return head;
		}
		head = head->next;
	}
	return NULL;
}

bbinfo_t * find_bb(funcinfo_t * func, uint32_t addr){

	for (int i = 0; i < func->bbs.size(); i++){
		if ( (func->bbs[i]->start_addr >= addr) && (addr < func->bbs[i]->start_addr + func->bbs[i]->size) ){
			return func->bbs[i];
		}
	}
	return NULL;

}

funcinfo_t * find_func(moduleinfo_t * module,uint32_t start_addr){
	for (int i = 0; i < module->funcs.size(); i++){
		if (start_addr == module->funcs[i]->start_addr){
			return module->funcs[i];
		}
	}
	return NULL;
}

funcinfo_t * find_func_app_pc(moduleinfo_t * module, uint32_t app_pc){
	for (int i = 0; i < module->funcs.size(); i++){
		if (find_bb(module->funcs[i], app_pc) != NULL){
			return module->funcs[i];
		}
	}
	return NULL;
}

void print_funcs(moduleinfo_t * module,ofstream &file){

	assert(file.good());

	while (module != NULL){
		file << module->name << endl;
		for (int i = 0; i < module->funcs.size(); i++){
			file << hex << module->funcs[i]->start_addr << "-" << dec << module->funcs[i]->freq << endl;
		}
		module = module->next;
	}
}

void print_moduleinfo(moduleinfo_t * module,ofstream &file){

	assert(file.good());

	//get the number of modules
	moduleinfo_t * local = module;
	uint32_t number_modules = 0;
	while (local != NULL){
		number_modules++;
		local = local->next;
	}
	
	file << number_modules << endl;
	while (module != NULL){
		file << module->name << endl;
		file << hex << module->start_addr << endl;
		
		for (int i = 0; i < module->funcs.size(); i++){

			file << "func-" << hex << module->funcs[i]->start_addr << dec << "-" << module->funcs[i]->bbs.size() << "-" << module->funcs[i]->freq << endl;

			funcinfo_t * func = module->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){

				file << hex << func->bbs[j]->start_addr << "," << dec << func->bbs[j]->size << "," << func->bbs[j]->freq << ",";
				bbinfo_t * bb = func->bbs[j];
				
				file << bb->from_bbs.size() << "," ;
				for (int k = 0; k < bb->from_bbs.size(); k++){
					file << hex << bb->from_bbs[k]->target << "," << dec << bb->from_bbs[k]->freq << ",";
				}

				file << bb->to_bbs.size() << ",";
				for (int k = 0; k < bb->to_bbs.size(); k++){
					file << hex << bb->to_bbs[k]->target << "," << dec << bb->to_bbs[k]->freq << ",";
				}

				file << bb->callees.size() << ",";
				for (int k = 0; k < bb->callees.size(); k++){
					file << hex << bb->callees[k]->target << "," << dec << bb->callees[k]->freq << ",";
				}

				file << bb->callers.size() << ",";
				for (int k = 0; k < bb->callers.size(); k++){
					file << hex << bb->callers[k]->target << "," << dec << bb->callers[k]->freq << ",";
				}

				file << endl;

			}
		}

		module = module->next;

	}


}

moduleinfo_t * populate_moduleinfo(ifstream &file){

	moduleinfo_t * head = new moduleinfo_t();

	assert(file.good());

	char string_val[MAX_STRING_LENGTH];

	//get the number of modules
	file.getline(string_val, MAX_STRING_LENGTH);
	int no_modules = atoi(string_val);
	
	moduleinfo_t * current_module = NULL;

	uint32_t count = 0;

	for (int i = 0; i < no_modules; i++){

		if (current_module == NULL){
			current_module = head;
		}
		else{
			current_module->next = new moduleinfo_t();
			current_module = current_module->next;
		}

		//module name
		file.getline(current_module->name, MAX_STRING_LENGTH);
		count++;

		//module start addr;
		file.getline(string_val, MAX_STRING_LENGTH);
		current_module->start_addr = strtoull(string_val, NULL, 16);
		count++;

		//number of bbs
		file.getline(string_val, MAX_STRING_LENGTH);
		int no_bbs = atoi(string_val);
		count++;

		for (int j = 0; j < no_bbs; j++){

			

			/*func, bb_addr, size, freq, <from_bbs_count>, from_bb, freq, .., <caller_count>, caller, freq, ..., <to_bbs_count>, to_bbs, freq, ..., <callee_count>, callee, freq*/

			file.getline(string_val, MAX_STRING_LENGTH);
			string string_cpp(string_val);
			vector<string> tokens = split(string_cpp, ',');

			//func start
			uint32_t index = 0;
			uint32_t func_start = strtoul(tokens[index++].c_str(),NULL,16);

			
			funcinfo_t * func = find_func(current_module, func_start);
			if (func == NULL){
				func = new funcinfo_t();
				current_module->funcs.push_back(func);
			}
			
			func->start_addr = func_start;
			bbinfo_t * new_bb = new bbinfo_t();
			func->bbs.push_back(new_bb);

			//populate bb
			new_bb->start_addr = strtoul(tokens[index++].c_str(), NULL, 16);

			new_bb->size = strtoul(tokens[index++].c_str(), NULL, 10);
			new_bb->freq = strtoul(tokens[index++].c_str(), NULL, 10);

			uint32_t from_bbs = atoi(tokens[index++].c_str());
			for (int k = 0; k < from_bbs; k++){
				targetinfo_t * info = new targetinfo_t();
				info->target = strtoul(tokens[index++].c_str(), NULL, 16);
				info->freq = strtoul(tokens[index++].c_str(), NULL, 10);				
				new_bb->from_bbs.push_back(info);
			}

			uint32_t to_bbs = atoi(tokens[index++].c_str());
			for (int k = 0; k < to_bbs; k++){
				targetinfo_t * info = new targetinfo_t();
				info->target = strtoul(tokens[index++].c_str(), NULL, 16);
				info->freq = strtoul(tokens[index++].c_str(), NULL, 10);
				new_bb->to_bbs.push_back(info);
			}

			uint32_t called_from = atoi(tokens[index++].c_str());
			for (int k = 0; k < called_from; k++){
				targetinfo_t * info = new targetinfo_t();
				info->target = strtoul(tokens[index++].c_str(), NULL, 16);
				info->freq = strtoul(tokens[index++].c_str(), NULL, 10);
				new_bb->callees.push_back(info);
			}

			uint32_t called_to = atoi(tokens[index++].c_str());
			for (int k = 0; k < called_to; k++){
				targetinfo_t * info = new targetinfo_t();
				info->target = strtoul(tokens[index++].c_str(), NULL, 16);
				info->freq = strtoul(tokens[index++].c_str(), NULL, 10);
				new_bb->callers.push_back(info);
			}

			count++;
			
			

		}
	}

	populate_func_freq(head);

	return head;

}

void print_filter_file(ofstream &file, moduleinfo_t * head){

	/* get the number of modules */
	moduleinfo_t * count_mod = head;
	uint32_t count = 0;

	while (count_mod != NULL){
		count++;
		count_mod = count_mod->next;
	}

	file << count << endl;

	while (head != NULL){

		file << head->name << endl;
		file << head->funcs.size() << endl;
		for (int i = 0; i < head->funcs.size(); i++){
			file << head->funcs[i]->start_addr << endl;
		}


		head = head->next;
	}


}














