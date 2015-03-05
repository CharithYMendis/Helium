//file parser and other file operations are grouped in this file

#include <vector>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <algorithm>

#include "utility\fileparser.h"
#include "utility\defines.h"
#include "analysis\staticinfo.h"

#include "utilities.h"
#include "common_defines.h"

using namespace std;

void go_forward_line(std::ifstream &file);
bool go_backward_line(std::ifstream &file);
void go_to_line(uint32_t line_no, std::ifstream &file);
uint32_t go_to_line_dest(std::ifstream &file, uint64_t dest, uint32_t stride);

uint32_t fill_operand(operand_t * operand, vector<string> &tokens, uint32_t start, uint32_t version){

	uint32_t i = start;

	operand->type = atoi(tokens[i++].c_str());
	operand->width = atoi(tokens[i++].c_str());
	if (operand->type == IMM_FLOAT_TYPE){
		operand->float_value = atof(tokens[i++].c_str());

	}
	else{
#ifndef __GNUG__
		operand->value = stoull(tokens[i++].c_str());
#else
		operand->value = strtoull(tokens[i++].c_str(), NULL, 10);
#endif
	}

	if (version == VER_WITH_ADDR_OPND){
		if (operand->type == MEM_STACK_TYPE || operand->type == MEM_HEAP_TYPE){
			/* we need to collect the addr operands */
			operand->addr = new operand_t[4];
			for (int j = 0; j < 4; j++){
				operand->addr[j].type == atoi(tokens[i++].c_str());
				operand->addr[j].width = atoi(tokens[i++].c_str());
#ifndef __GNUG__
				operand->addr[j].value = stoull(tokens[i++].c_str());
#else
				operand->addr[j].value = strtoull(tokens[i++].c_str(), NULL, 10);
#endif
			}

		}
	}
	else if(version == VER_NO_ADDR_OPND){
		operand->addr = NULL;
	}

	return i;

}

/* main file parsing functions */
cinstr_t * get_next_from_ascii_file(ifstream &file, uint32_t version){

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

		int index = 2;
		for (int i = 0; i < instr->num_dsts; i++){
			index = fill_operand(&instr->dsts[i], tokens, index, version);
		}

		//get the number of sources
		instr->num_srcs = atoi(tokens[index++].c_str());

		for (int i = 0; i < instr->num_srcs; i++){
			index = fill_operand(&instr->srcs[i], tokens, index, version);
		}

		instr->eflags = (uint32_t)stoull(tokens[index++].c_str());
		instr->pc = atoi(tokens[index++].c_str());

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

/* parsing the disasm file */
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

bool compare_static_info(Static_Info * first, Static_Info * second){
	if (first->module_no == second->module_no){
		return first->pc < second->pc;
	}
	else{
		return first->module_no < second->module_no;
	}
}

void parse_debug_disasm(vector<Static_Info *> &static_info, ifstream &file){

	while (!file.eof()){

		char string_ins[MAX_STRING_LENGTH];
		//we need to parse the file here - forward parsing and backward traversal
		file.getline(string_ins, MAX_STRING_LENGTH);
		string string_cpp(string_ins);

		if (string_cpp.size() > 0){

			uint32_t module_no;
			uint32_t app_pc;

			string disasm_string = parse_line_disasm(string_cpp, &module_no, &app_pc);

			Static_Info * disasm;

			bool found = false;
			for (int i = 0; i < static_info.size(); i++){
				if (static_info[i]->module_no == module_no && static_info[i]->pc == app_pc){
					disasm = static_info[i];
					found = true;
					break;
				}
			}

			if (!found){
				disasm = new Static_Info;
				disasm->module_no = module_no;
				disasm->pc = app_pc;
				disasm->disassembly = disasm_string;
				static_info.push_back(disasm);
			}

			
		}

	}


	sort(static_info.begin(), static_info.end(), compare_static_info);

}

/* advanced instruction oriented file parsing functions */
vector<cinstr_t * > get_all_instructions(ifstream &file, uint32_t version){

	vector<cinstr_t *> instrs;

	while (!file.eof()){
		cinstr_t * instr = get_next_from_ascii_file(file, version);
		if (instr != NULL){
			instrs.push_back(instr);
		}

	}

	return instrs;

}


vec_cinstr walk_file_and_get_instructions(ifstream &file, vector<Static_Info *> &static_info, uint32_t version){

	cinstr_t * instr;
	Static_Info * info;
	vec_cinstr instrs;

	while (!file.eof()){
		instr = get_next_from_ascii_file(file, version);
		
		if (instr != NULL){
			info = get_static_info(static_info, instr->pc);
			instrs.push_back(make_pair(instr, info));
		}
	}

	return instrs;

}

void reverse_file(ofstream &out, ifstream &in){

	char value[MAX_STRING_LENGTH];
	in.seekg(-1, ios::end);
	in.clear();

	bool check = true;
	while (check){
		go_backward_line(in);
		in.getline(value, MAX_STRING_LENGTH);
		out << value << endl;
		//cout << value << endl;
		check = go_backward_line(in);
	}

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
 
uint32_t go_to_line_dest(ifstream &file, uint64_t dest, uint32_t stride, uint32_t version){

	/* assume that the file is at the beginning*/
	uint32_t lineno = 0;
	cinstr_t * instr;


	while (!file.eof()){
		instr = get_next_from_ascii_file(file, version);
		lineno++;
		if (instr != NULL){
			for (int i = 0; i < instr->num_dsts; i++){
				if ((instr->dsts[i].value == dest) && (instr->dsts[i].width == stride)){
					DEBUG_PRINT(("found - pc %d\n", instr->pc), 4);
					return lineno;
				}
			}
		}
		delete instr;
	}

	return 0;

}

/* debug routines */
void walk_instructions(ifstream &file, uint32_t version){

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;

	while (!file.eof()){
		instr = get_next_from_ascii_file(file, version);
		if (instr != NULL){
			rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "", 0);
			delete[] rinstr;
		}
		delete instr;
	}

}

void print_disasm(vector<Static_Info *> &static_info){
	for (int i = 0; i < static_info.size(); i++){
		cout << static_info[i]->module_no << "," << static_info[i]->pc << "," << static_info[i]->disassembly << endl;
	}
}

string get_disasm_string(vector<Static_Info *> &static_info, uint32_t app_pc){

	for (int i = 0; i < static_info.size(); i++){
		if (static_info[i]->pc == app_pc){
			return static_info[i]->disassembly;
		}
	}
	
	return "";

}


