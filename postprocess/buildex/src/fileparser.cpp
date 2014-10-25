//file parser and other file operations are grouped in this file
#include "fileparser.h"
#include <vector>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include "canonicalize.h"
#include "defines.h"
#include <stdint.h>
#include <vector>
#include "utilities.h"
#include "print_common.h"
#include "common_defines.h"
#include <algorithm>

using namespace std;

void print_cinstr(cinstr_t * instr);
string * get_disasm_string_ref(vector<disasm_t *> &disasm, uint32_t app_pc);


/* main file parsing functions */
cinstr_t * get_next_from_ascii_file(ifstream &file){

	cinstr_t * instr;
	char string_ins[MAX_STRING_LENGTH];

	//we need to parse the file here - forward parsing and backward traversal
	file.getline(string_ins, MAX_STRING_LENGTH);

	string string_cpp(string_ins);

#ifdef DEBUG
#if DEBUG_LEVEL >= 5
	cout << string_cpp << endl;
#endif
#endif

	instr = NULL;

	if (string_cpp.size() > 0){

		instr = new cinstr_t;

		vector<string> tokens;
		tokens = split(string_cpp, ',');

		//now parse the string  - this is specific to the string being outputted
		instr->opcode = atoi(tokens[0].c_str());

		//get the number of destinations
		instr->num_dsts = atoi(tokens[1].c_str());

		int index_op = 0;
		int i;
		for (i = 2; i< 3 * instr->num_dsts + 2; i += 3){
			instr->dsts[index_op].type = atoi(tokens[i].c_str());

			instr->dsts[index_op].width = atoi(tokens[i + 1].c_str());

			if (instr->dsts[index_op].type == IMM_FLOAT_TYPE){
				instr->dsts[index_op++].float_value = atof(tokens[i + 2].c_str());

			}
			else{
#ifndef __GNUG__
				instr->dsts[index_op++].value = stoull(tokens[i + 2].c_str());
#else
				instr->dsts[index_op++].value = strtoull(tokens[i + 2].c_str(), NULL, 10);
#endif
			}
		}

		//get the number of sources
		instr->num_srcs = atoi(tokens[i++].c_str());

		index_op = 0;
		int current = i;

		for (; i< 3 * instr->num_srcs + current; i += 3){
			instr->srcs[index_op].type = atoi(tokens[i].c_str());
			instr->srcs[index_op].width = atoi(tokens[i + 1].c_str());
			if (instr->srcs[index_op].type == IMM_FLOAT_TYPE){
				instr->srcs[index_op++].float_value = atof(tokens[i + 2].c_str());
				//cout << tokens[i + 2] << endl;
			}
			else{
#ifndef __GNUG__
				instr->srcs[index_op++].value = stoull(tokens[i + 2].c_str());
#else
				instr->srcs[index_op++].value = strtoull(tokens[i + 2].c_str(), NULL, 10);
#endif
			}
		}

		instr->eflags = atoi(tokens[i++].c_str());
		instr->pc = atoi(tokens[i].c_str());

	}

#ifdef DEBUG
#if DEBUG_LEVEL >= 5
	if (instr != NULL)
		print_cinstr(instr);
#endif
#endif
	return instr;

}

cinstr_t * get_next_from_bin_file(ifstream &file){
	return NULL;
}


/* advanced instruction oriented file parsing functions */
vector<cinstr_t * > get_all_instructions(ifstream &file){

	vector<cinstr_t *> instrs;

	while (!file.eof()){
		cinstr_t * instr = get_next_from_ascii_file(file);
		if (instr != NULL){
			instrs.push_back(instr);
		}

	}

	return instrs;

}


vec_cinstr walk_file_and_get_instructions(ifstream &file, vector<disasm_t *> &disasm){

	cinstr_t * instr;
	string * dis_string;
	vector< pair<cinstr_t *, string *> > instrs;

	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		if (instr != NULL){
			dis_string = get_disasm_string_ref(disasm, instr->pc);
			instrs.push_back(make_pair(instr, dis_string));
		}
	}

	return instrs;



}

void walk_instructions(ifstream &file){

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;

	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		if (instr != NULL){
			rinstr = cinstr_to_rinstrs(instr, no_rinstrs,"",0);
			delete[] rinstr;
		}
		delete instr;
	}

}

uint32_t go_to_line_dest(ifstream &file, uint64_t dest, uint32_t stride){

	/* assume that the file is at the beginning*/
	uint32_t lineno = 0;
	cinstr_t * instr;


	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		lineno++;
		if (instr != NULL){
			for (int i = 0; i < instr->num_dsts; i++){
				if ( (instr->dsts[i].value == dest) && (instr->dsts[i].width == stride) ){
					DEBUG_PRINT(("found - pc %d\n", instr->pc), 4);
					return lineno;
				}
			}
		}
		delete instr;
	}

	return 0;

}
 

/* generic helper functions for file parsing */
void go_to_line(uint line_no, ifstream &file){

	file.seekg(file.beg);
	uint current_line = 0;

	char dummystr[MAX_STRING_LENGTH];

	while (current_line < line_no - 1){
		file.getline(dummystr, MAX_STRING_LENGTH);
		current_line++;
	}


}

void go_forward_line(ifstream &file){
	
	char array[MAX_STRING_LENGTH];
	file.getline(array,MAX_STRING_LENGTH);
	
 }
 
bool go_backward_line(ifstream &file){
 
	int value = '\0';
	unsigned int pos;

	file.seekg(-1,ios::cur);
	pos = file.tellg();

	
	while((value != '\n') && (pos != 0)){
		file.seekg(-1,ios::cur);
		value = file.peek();
		pos = file.tellg();
	}
	
	pos = file.tellg();

	if(pos == 0){
		return false;
	}
	
	
	file.seekg(1,ios::cur);  

	
	return true;
 
 }
 
void reverse_file(ofstream &out, ifstream &in){
	
	char value[MAX_STRING_LENGTH];
	in.seekg(-1,ios::end);
	in.clear();

	bool check = true;
	while(check){
		go_backward_line(in);
		in.getline(value,MAX_STRING_LENGTH);
		out << value << endl;
		//cout << value << endl;
		check = go_backward_line(in);
	}
	
 }


void print_cinstr(cinstr_t * instr){
	cout << instr->opcode << ",";
	cout << instr->num_dsts << ",";
	for (int i = 0; i<instr->num_dsts; i++){
		cout << instr->dsts[i].type << ",";
		cout << instr->dsts[i].width << ",";
		cout << instr->dsts[i].value << ",";
	}

	cout << instr->num_srcs << ",";
	for (int i = 0; i<instr->num_srcs; i++){
		cout << instr->srcs[i].type << ",";
		cout << instr->srcs[i].width << ",";
		cout << instr->srcs[i].value << ",";
	}
	cout << instr->eflags << ",";
	cout << instr->pc << endl;

	cout << dr_operation_to_string(instr->opcode) << endl;
}

string parse_line_disasm(string line, uint32_t * module, uint32_t * app_pc){

	int i = 0;
	string value = "";
	while (line[i] != ','){
		value += line[i];
		i++;
	}
	*module = atoi(value.c_str());
	i++; value = "";
	
	while (line[i] != ','){
		value += line[i];
		i++;
	}
	*app_pc = atoi(value.c_str());

	i++; value = "";
	for (; i < line.size(); i++){
		value += line[i];
	}
	
	return value;

}


bool compare_disasm(pair<uint32_t, string> first, pair<uint32_t, string> second){

	return first.first < second.first;

}

void print_disasm(vector<disasm_t *> &disasm){
	for (int i = 0; i < disasm.size(); i++){
		for (int j = 0; j < disasm[i]->pc_disasm.size(); j++){
			cout << disasm[i]->module_no << "," << disasm[i]->pc_disasm[j].first << "," << disasm[i]->pc_disasm[j].second << endl;
		}
	}
}


vector<disasm_t *> parse_debug_disasm(ifstream &file){


	vector<disasm_t *> disasm;

	while (!file.eof()){

		char string_ins[MAX_STRING_LENGTH];
		//we need to parse the file here - forward parsing and backward traversal
		file.getline(string_ins, MAX_STRING_LENGTH);
		string string_cpp(string_ins);

		if (string_cpp.size() > 0){

			uint32_t module_no;
			uint32_t app_pc;

			string disasm_string = parse_line_disasm(string_cpp, &module_no, &app_pc);

			disasm_t * dissasm;

			bool found = false;
			for (int i = 0; i < disasm.size(); i++){
				if (disasm[i]->module_no == module_no){
					dissasm = disasm[i];
					found = true;
					break;
				}
			}

			if (!found){
				dissasm = new disasm_t;
				dissasm->module_no = module_no;
				disasm.push_back(dissasm);
			}

			found = false;
			for (int i = 0; i < dissasm->pc_disasm.size(); i++){
				if (dissasm->pc_disasm[i].first == app_pc){
					found = true;
					break;
				}
			}

			if (!found){
				dissasm->pc_disasm.push_back(make_pair(app_pc,disasm_string));
			}
		}
		
	}

	for (int i = 0; i < disasm.size(); i++){
		sort(disasm[i]->pc_disasm.begin(), disasm[i]->pc_disasm.end(), compare_disasm);
	}

	return disasm;


}


string * get_disasm_string_ref(vector<disasm_t *> &disasm, uint32_t app_pc){

	for (int i = 0; i < disasm.size(); i++){
		for (int j = 0; j < disasm[i]->pc_disasm.size(); j++){
			if (disasm[i]->pc_disasm[j].first == app_pc){
				return &disasm[i]->pc_disasm[j].second;
			}
		}
	}

	return NULL;

}




vector<string> get_disasm_string(vector<disasm_t *> &disasm, uint32_t app_pc){

	vector<string> string_disasm;

	for (int i = 0; i < disasm.size(); i++){
		for (int j = 0; j < disasm[i]->pc_disasm.size(); j++){
			if (disasm[i]->pc_disasm[j].first == app_pc){
				string_disasm.push_back(disasm[i]->pc_disasm[j].second);
			}
		}
	}

	return string_disasm;
}


