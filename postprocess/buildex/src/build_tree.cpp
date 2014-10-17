/*given a file and some other parameters this will build the expression tree*/
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "fileparser.h"
#include "build_tree.h"
#include "defines.h"
#include "expression_tree.h"
#include "canonicalize.h"

using namespace std;


void cinstr_convert_reg(cinstr_t * instr){

	for (int i = 0; i < instr->num_srcs; i++){
		if ((instr->srcs[i].type == REG_TYPE) && (instr->srcs[i].value > DR_REG_ST7)) instr->srcs[i].value += 8;
		reg_to_mem_range(&instr->srcs[i]);
	}

	for (int i = 0; i < instr->num_dsts; i++){
		if (instr->dsts[i].type == REG_TYPE && (instr->dsts[i].value > DR_REG_ST7)) instr->dsts[i].value += 8;
		reg_to_mem_range(&instr->dsts[i]);
	}

}


void print_vector(vector<pair<uint32_t,uint32_t> > lines, vector<disasm_t *> disasm){
	for (int i = 0; i < lines.size(); i++){
		vector<string> disasm_string = get_disasm_string(disasm,lines[i].second);
		
		cout << lines[i].first << " - " << lines[i].second;
		if (disasm_string.size()>0){
			cout << " " << disasm_string[0];
		}
		cout << endl;
 	}
}


void build_tree(uint64 destination, int start_trace, int end_trace, ifstream &file, Expression_tree * tree, vector<disasm_t *> disasm){

	DEBUG_PRINT(("build_tree(concrete)....\n"), 2);

	if (end_trace != FILE_ENDING)
		ASSERT_MSG((end_trace >= start_trace), ("trace end should be greater than the trace start\n"));

	uint curpos = 0;
	
	if (start_trace != FILE_BEGINNING){
		go_to_line(start_trace, file);
		curpos = start_trace - 1;
	}

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;


	//now we need to read the next line and start from the correct destination
	bool dest_present = false;
	int index = -1;

	//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well. can have an assert for this

	instr = get_next_from_ascii_file(file);
	

	ASSERT_MSG((instr != NULL), ("ERROR: you have given a line no beyond this file\n"));
	cinstr_convert_reg(instr);

	vector<string> string_disasm = get_disasm_string(disasm, instr->pc);
	if (debug && debug_level >= 4){
		for (int i = 0; i < string_disasm.size(); i++){
			cout << string_disasm[i] << endl;
		}
	}

	curpos++;
	if (string_disasm.size()>0){
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, string_disasm[0], curpos);
	}
	else{
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
	}

	

	if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs);}
	for (int i = no_rinstrs - 1; i >= 0; i--){
		if (rinstr[i].dst.value == destination){
			ASSERT_MSG((rinstr[i].dst.type != IMM_FLOAT_TYPE) && (rinstr[i].dst.type != IMM_INT_TYPE), ("ERROR: dest cannot be an immediate\n"));
			index = i;
			dest_present = true;
			break;
		}
	}

	ASSERT_MSG((dest_present == true) && (index >= 0), ("ERROR: couldn't find the dest to start trace\n")); //we should have found the destination

	//do the initial processing
	for (int i = index; i >= 0; i--){
		if (string_disasm.size() > 0){
			tree->update_frontier(&rinstr[i], instr->pc, string_disasm[0],curpos);
		}
		else{
			tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
		}
	}

	vector<pair<uint32_t,uint32_t> > lines;

	//do the rest of expression tree building
	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		rinstr = NULL;
		//print_cinstr(instr);
		curpos++;
		DEBUG_PRINT(("->line - %d\n", curpos), 3);
		if (instr != NULL){
			cinstr_convert_reg(instr);

			vector<string> string_disasm = get_disasm_string(disasm, instr->pc);
			if (debug && debug_level >= 4){
				for (int i = 0; i < string_disasm.size(); i++){
					cout << string_disasm[i] << endl;
				}
			}

			if (string_disasm.size() > 0){
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, string_disasm[0], curpos);
			}
			else{
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
			}
			
			if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs); }
			bool updated = false;
			for (int i = no_rinstrs - 1; i >= 0; i--){
				if (string_disasm.size() > 0){
					updated = tree->update_frontier(&rinstr[i], instr->pc,string_disasm[0], curpos);
				}
				else{
					updated = tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
				}
				if(updated) lines.push_back(make_pair(curpos,instr->pc));
			}
		}

		if ( (end_trace != FILE_ENDING) && (curpos == end_trace)){
			break;
		}

		delete instr;
		delete[] rinstr;

	}

	DEBUG_PRINT(("build_tree(concrete) - done\n"), 2);
	print_vector(lines,disasm);


}

/* yet to implement */
void build_tree(uint64 destination, int start_trace, int end_trace, vector<cinstr_t *> &instrs, Expression_tree * tree){

}

vector<uint32_t> get_instrace_startpoints(ifstream &file, uint32_t pc){

	vector<uint32_t> start_points;
	int line = 0;

	while (!file.eof()){
		cinstr_t * instr = get_next_from_ascii_file(file);
		line++;
		if (instr != NULL){
			if (instr->pc == pc){
				start_points.push_back(line);
			}
		}
		
	}

	return start_points;

}


bool are_conc_trees_similar(Node * first, Node * second){


	if ((first->symbol->type != second->symbol->type) || (first->operation != second->operation)){
		return false;
	}


	/*check whether all the nodes have same number of sources*/
	if (first->srcs.size() != second->srcs.size()){
		return false;
	}
	

	/* recursively check whether the src nodes are similar*/
	bool ret = true;
	for (int i = 0; i < first->srcs.size(); i++){
		if (!are_conc_trees_similar(first->srcs[i], second->srcs[i])){
			ret = false;
			break;
		}
	}

	return ret;

}
