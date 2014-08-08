 #include "moduleinfo.h"
#include <vector>
#include <fstream>
#include "defines.h"
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>

using namespace std;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

/* debug and helper functions */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

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

bbinfo_t * find_bb(funcinfo_t * func, uint32_t start_addr){

	for (int i = 0; i < func->bbs.size(); i++){
		if (func->bbs[i]->start_addr == start_addr){
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

void print_funcs(moduleinfo_t * module,const char * filename){

	FILE * file;
	file = fopen(filename, "w");

	assert(file != NULL);

	while (module != NULL){
		fprintf(file, "%s\n", module->name);
		for (int i = 0; i < module->funcs.size(); i++){
			fprintf(file, "%x-%d\n", module->funcs[i]->start_addr, module->funcs[i]->freq);
		}
		module = module->next;
	}

	fclose(file);
}

void print_moduleinfo(moduleinfo_t * module,const char * filename){

	FILE * file;
	file = fopen(filename, "w");

	assert(file != NULL);

	//get the number of modules
	moduleinfo_t * local = module;
	uint32_t number_modules = 0;
	while (local != NULL){
		number_modules++;
		local = local->next;
	}
	
	fprintf(file, "%d\n", number_modules);
	while (module != NULL){
		fprintf(file, "%s\n%x\n", module->name,module->start_addr);
		
		for (int i = 0; i < module->funcs.size(); i++){
			fprintf(file, "func-%x-%d-%d\n", module->funcs[i]->start_addr,module->funcs[i]->bbs.size(),module->funcs[i]->freq);
			funcinfo_t * func = module->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){
				fprintf(file, "%x,%d,%d,", func->bbs[j]->start_addr, func->bbs[j]->size, func->bbs[j]->freq);
				bbinfo_t * bb = func->bbs[j];
				
				fprintf(file, "%d,", bb->from_bbs.size());
				for (int k = 0; k < bb->from_bbs.size(); k++){
					fprintf(file, "%x,%d,", bb->from_bbs[k]->target, bb->from_bbs[k]->freq);
				}

				fprintf(file, "%d,", bb->to_bbs.size());
				for (int k = 0; k < bb->to_bbs.size(); k++){
					fprintf(file, "%x,%d,", bb->to_bbs[k]->target, bb->to_bbs[k]->freq);
				}

				fprintf(file, "%d,", bb->callees.size());
				for (int k = 0; k < bb->callees.size(); k++){
					fprintf(file, "%x,%d,", bb->callees[k]->target, bb->callees[k]->freq);
				}

				fprintf(file, "%d,", bb->callers.size());
				for (int k = 0; k < bb->callers.size(); k++){
					fprintf(file, "%x,%d,", bb->callers[k]->target, bb->callers[k]->freq);
				}

				fprintf(file, "\n");

			}
		}

		module = module->next;

	}

	fclose(file);

}

moduleinfo_t * populate_moduleinfo(const char* filename){

	moduleinfo_t * head = new moduleinfo_t();
	ifstream file;
	file.open(filename);

	assert(file.good());

	char string_val[MAX_STRING_LENGTH];

	//get the number of modules
	file.getline(string_val, MAX_STRING_LENGTH);
	int no_modules = atoi(string_val);


	
	moduleinfo_t * current_module = NULL;

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


		//module start addr;
		file.getline(string_val, MAX_STRING_LENGTH);
		current_module->start_addr = strtoull(string_val, NULL, 16);


		//number of bbs
		file.getline(string_val, MAX_STRING_LENGTH);
		int no_bbs = atoi(string_val);


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


			

		}
	}

	file.close();

	populate_func_freq(head);

	return head;

}














