 #include "moduleinfo.h"
#include <vector>
#include <fstream>
#include "defines.h"
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include "utilities.h"
#include <queue>

using namespace std;


void populate_func_freq(moduleinfo_t * head){

	while (head != NULL){

		for (int i = 0; i < head->funcs.size(); i++){
			funcinfo_t * func = head->funcs[i];
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
		head = head->next;

	}

}

/* parsing information from a file into the moduleinfo structure */
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
			uint32_t func_start = strtoul(tokens[index++].c_str(), NULL, 16);


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
			new_bb->is_call = strtoul(tokens[index++].c_str(), NULL, 10);
			new_bb->is_ret = strtoul(tokens[index++].c_str(), NULL, 10);
			new_bb->is_call_target = strtoul(tokens[index++].c_str(), NULL, 10);
			new_bb->func_addr = 0;

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

/* mining information from the module structure */
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

/* find the bb having the addr from a func 
* addr - any valid app_pc
*/
bbinfo_t * find_bb(funcinfo_t * func, uint32_t addr){

	for (int i = 0; i < func->bbs.size(); i++){
		if ((func->bbs[i]->start_addr >= addr) && (addr < func->bbs[i]->start_addr + func->bbs[i]->size)){
			return func->bbs[i];
		}
	}
	return NULL;

}

/* find the bb having the addr from a module 
* addr - any valid app_pc
*/
bbinfo_t * find_bb(moduleinfo_t * module, uint32_t addr){

	for (int i = 0; i < module->funcs.size(); i++){
		funcinfo_t * func = module->funcs[i];
		for (int j = 0; j < func->bbs.size(); j++){
			if ((func->bbs[j]->start_addr <= addr) && (addr < (func->bbs[j]->start_addr + func->bbs[j]->size))){
				return func->bbs[j];
			}
			
		}
	}
	return NULL;

}

bbinfo_t * find_bb_exact(moduleinfo_t * module, uint32_t addr){

	for (int i = 0; i < module->funcs.size(); i++){
		funcinfo_t * func = module->funcs[i];
		for (int j = 0; j < func->bbs.size(); j++){
			if (func->bbs[j]->start_addr == addr){
				return func->bbs[j];
			}

		}
	}
	return NULL;

}

/* tries to find the occurance in the current module or else find in another module 
* addr - bb start addr
*/
bbinfo_t * find_bb(moduleinfo_t * head, moduleinfo_t * current, uint32_t addr, moduleinfo_t ** from_module){

	/* find in the current module */
	for (int i = 0; i < current->funcs.size(); i++){
		funcinfo_t * func = current->funcs[i];
		for (int j = 0; j < func->bbs.size(); j++){
			if (func->bbs[j]->start_addr == addr){
				*from_module = current;
				return func->bbs[j];
			}
		}
	}

	*from_module = false;
	/* if not try to find from all the modules */
	while (head != NULL){
		for (int i = 0; i < head->funcs.size(); i++){
			funcinfo_t * func = head->funcs[i];
			for (int j= 0; j < func->bbs.size(); j++){
				if (func->bbs[j]->start_addr == addr){
					*from_module = head;
					return func->bbs[j];
				}
			}
		}
		head = head->next;
	}
	return NULL;

}

bool is_funcs_present(moduleinfo_t * head){

	while (head != NULL){
		if (!(head->funcs.size() == 1 && head->funcs[0]->start_addr == 0)){
			return true;
		}
		head = head->next;
	}
	return false;
}

moduleinfo_t * get_call_targets(moduleinfo_t * head){

	moduleinfo_t * new_head = NULL;
	moduleinfo_t * prev = NULL;
	moduleinfo_t * ret_head;


	while (head != NULL){

		ret_head = new moduleinfo_t;
		strncpy(ret_head->name, head->name, MAX_STRING_LENGTH);
		ret_head->start_addr = head->start_addr;

		if (prev != NULL) prev->next = ret_head;
		if (new_head == NULL) new_head = ret_head;

		funcinfo_t * func = new funcinfo_t;
		func->start_addr = 0;
		ret_head->funcs.push_back(func);

		for (int i = 0; i < head->funcs.size(); i++){
			for (int j = 0; j < head->funcs[i]->bbs.size(); j++){
				bbinfo_t * bb = head->funcs[i]->bbs[j];
				if (bb->is_call_target){
					func->bbs.push_back(bb);
				}
			}
		}

		prev = ret_head;
		ret_head = ret_head->next;

		head = head->next;
	}

	return new_head;


}

moduleinfo_t * get_probable_call_targets(moduleinfo_t * head){

	moduleinfo_t * new_head = NULL;
	moduleinfo_t * prev = NULL;
	moduleinfo_t * ret_head;
	

	while (head != NULL){

		ret_head = new moduleinfo_t;
		strncpy(ret_head->name, head->name, MAX_STRING_LENGTH);
		ret_head->start_addr = head->start_addr;

		if (prev != NULL) prev->next = ret_head;
		if (new_head == NULL) new_head = ret_head;

		funcinfo_t * func = new funcinfo_t;
		func->start_addr = 0;
		ret_head->funcs.push_back(func);

		for (int i = 0; i < head->funcs.size(); i++){
			for (int j = 0; j < head->funcs[i]->bbs.size(); j++){
				bbinfo_t * bb = head->funcs[i]->bbs[j];
				if (bb->callees.size() > 0){
					func->bbs.push_back(bb);
				}
			}
		}

		prev = ret_head;
		ret_head = ret_head->next;

		head = head->next;
	}

	return new_head;

}

moduleinfo_t * get_probable_callers(moduleinfo_t * head){

	moduleinfo_t * new_head = NULL;
	moduleinfo_t * prev = NULL;
	moduleinfo_t * ret_head;

	while (head != NULL){

		ret_head = new moduleinfo_t();
		strncpy(ret_head->name,head->name, MAX_STRING_LENGTH);
		ret_head->start_addr = head->start_addr;

		if (prev != NULL) prev->next = ret_head;

		if (new_head == NULL) new_head = ret_head;

		funcinfo_t * func = new funcinfo_t;
		func->start_addr = 0;
		ret_head->funcs.push_back(func);

		for (int i = 0; i < head->funcs.size(); i++){
			for (int j = 0; j < head->funcs[i]->bbs.size(); j++){
				bbinfo_t * bb = head->funcs[i]->bbs[j];
				if (bb->is_call){
					func->bbs.push_back(bb);
				}
			}
		}

		prev = ret_head;
		ret_head = ret_head->next;
		head = head->next;
	}

	return new_head;

}

uint32_t get_number_of_modules(moduleinfo_t * head){

	/* get the number of modules */
	moduleinfo_t * count_mod = head;
	uint32_t count = 0;

	while (count_mod != NULL){
		count++;
		count_mod = count_mod->next;
	}

	return count;

}

uint32_t get_number_of_bbs(moduleinfo_t * module){

	uint32_t count = 0;
	for (int i = 0; i < module->funcs.size(); i++){
		count += module->funcs[i]->bbs.size();
	}
	return count;

}


struct rec_struct{

	uint32_t addr;
	int ret;

};

#define MAX_RECURSE 200

/* assuming same module has the function entry point */
uint32_t get_probable_func_entrypoint(moduleinfo_t * current, queue<rec_struct> &bb_start, vector<uint32_t> &processed, uint32_t max_recurse){

	if (max_recurse > MAX_RECURSE){
		DEBUG_PRINT(("WARNING: max recursion limit reached\n"), 2);
		return 0;
	}

	if (bb_start.empty()) return 0;

	uint32_t addr = bb_start.front().addr;
	int ret = bb_start.front().ret;
	bb_start.pop();
	processed.push_back(addr);

	bbinfo_t * bbinfo = find_bb_exact(current, addr);
	if (bbinfo == NULL){
		return get_probable_func_entrypoint(current, bb_start, processed, max_recurse + 1);
	}

	
	if (bbinfo->is_call_target) ret--;
	if (ret < 0){
		return bbinfo->start_addr;
	}
	if (bbinfo->is_ret && max_recurse > 0) ret++; 

	DEBUG_PRINT(("%x, %d, %d\n", addr, ret, bbinfo->freq), 15);

	for (int i = 0; i < bbinfo->from_bbs.size(); i++){
		DEBUG_PRINT(("bbs all - %x\n", bbinfo->from_bbs[i]->target), 20);
		if (find(processed.begin(), processed.end(), bbinfo->from_bbs[i]->target) == processed.end()){
			DEBUG_PRINT(("bbs - %x\n", bbinfo->from_bbs[i]->target), 20);
			bb_start.push({ bbinfo->from_bbs[i]->target, ret });
		}
	}

	return get_probable_func_entrypoint(current, bb_start, processed, max_recurse + 1);

}

uint32_t get_probable_func(moduleinfo_t * head, moduleinfo_t * current, uint32_t start_addr){

	bbinfo_t * bb = find_bb(current, start_addr);
	if (bb != NULL){
		rec_struct rec = { bb->start_addr, 0 };
		queue<rec_struct> queue;
		queue.push(rec);

		vector<uint32_t> processed;

		return get_probable_func_entrypoint(current, queue, processed, 0);
	}
	else{
		return 0;
	}


}


/* printing and file reading functions */

/* prints the funcs with freq - not suitable for filter funcs */
void print_funcs(ofstream &file, moduleinfo_t * module){

	assert(file.good());

	while (module != NULL){
		file << module->name << endl;
		for (int i = 0; i < module->funcs.size(); i++){
			file << hex << module->funcs[i]->start_addr << "-" << dec << module->funcs[i]->freq << endl;
		}
		module = module->next;
	}
}

/* prints the full module information - not suitable for filter funcs */
void print_moduleinfo(ofstream &file, moduleinfo_t * module){

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

				bbinfo_t * bb = func->bbs[j];

				file << hex << bb->start_addr << "," << dec << bb->size << "," << bb->freq << ",";
				file << dec << bb->is_call << "," << bb->is_ret << ",";

				file << bb->from_bbs.size() << ",";
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

/* prints information for the bb - not suitable for filter funcs */
void print_bbinfo(ofstream &file, moduleinfo_t * module){


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
		file << "\"" << module->name << "\"" << endl;
		file << hex << module->start_addr << endl;

		for (int i = 0; i < module->funcs.size(); i++){

			funcinfo_t * func = module->funcs[i];
			file << dec << func->bbs.size() << endl;
			for (int j = 0; j < func->bbs.size(); j++){

				bbinfo_t * bb = func->bbs[j];

				file << hex << bb->start_addr << "," << dec << bb->freq << endl;
			}
		}

		module = module->next;

	}

}


/* funcs for creating the filter files */

/* prints func entry points from the func member */
void print_funcs_filter_file(ofstream &file, moduleinfo_t * head){

	file << get_number_of_modules(head) << endl;

	while (head != NULL){

		file << "\"" << head->name <<  "\""  << endl;
		file << head->funcs.size() << endl;
		for (int i = 0; i < head->funcs.size(); i++){
			file << head->funcs[i]->start_addr << endl;
		}


		head = head->next;
	}


}

/* prints func entry points for a single module and func_addr */
void print_funcs_filter_file(ofstream &file, const char * name, uint32_t func_addr){

	file << 1 << endl;
	file << "\"" << name << "\"" << endl;
	file << 1 << endl;
	file << dec << func_addr << endl;

}

/* prints out the bb start points for the module in the filter file format */
void print_bb_filter_file(ofstream &file, moduleinfo_t * head){
	
	file << get_number_of_modules(head) << endl;
	
	moduleinfo_t * local_head = head;

	while (local_head != NULL){

		file << "\"" << local_head->name << "\"" << endl;
		file << get_number_of_bbs(local_head) << endl;
		for (int i = 0; i < local_head->funcs.size(); i++){
			funcinfo_t * func = local_head->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){
				file << func->bbs[j]->start_addr << endl;
			}

		}

		local_head = local_head->next;
	}
	

}














