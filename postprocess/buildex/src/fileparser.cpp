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

#define MAX_STRING_LENGTH 100

using namespace std;

void print_cinstr(cinstr_t * instr);
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);


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

void walk_instructions(ifstream &file){

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;

	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		if (instr != NULL){
			rinstr = cinstr_to_rinstrs(instr, no_rinstrs);
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
					return lineno;
				}
			}
		}
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
}


