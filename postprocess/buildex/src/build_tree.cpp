/*given a file and some other parameters this will build the expression tree*/
#include <fstream>
#include <iostream>
#include <vector>
#include "fileparser.h"
#include "build_tree.h"
#include "defines.h"
#include "expression_tree.h"

using namespace std;


void cinstr_convert_reg(cinstr_t * instr){

	for (int i = 0; i < instr->num_srcs; i++){
		reg_to_mem_range(&instr->srcs[i]);
	}

	for (int i = 0; i < instr->num_dsts; i++){
		reg_to_mem_range(&instr->dsts[i]);
	}

}


void build_tree(uint64 destination, int start_trace, int end_trace, ifstream &file, Expression_tree * tree){

	if (end_trace != FILE_END)
		ASSERT_MSG((end_trace >= start_trace), ("trace end should be greater than the trace start\n"));

	uint curpos = 0;
	
	if (start_trace != FILE_BEG){
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

	curpos++;
	rinstr = cinstr_to_rinstrs(instr, no_rinstrs);
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
		tree->update_frontier(&rinstr[i]);
	}


	//do the rest of expression tree building
	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		curpos++;
		//DEBUG_PRINT(("%d\n", curpos), 1);
		if (instr != NULL){
			cinstr_convert_reg(instr);
			rinstr = cinstr_to_rinstrs(instr, no_rinstrs);
			for (int i = no_rinstrs - 1; i >= 0; i--){
				tree->update_frontier(&rinstr[i]);
			}
		}

		if ( (end_trace != FILE_END) && (curpos == end_trace)){
			break;
		}

	}


}

/* yet to implement */
void build_tree(uint64 destination, int start_trace, int end_trace, vector<cinstr_t *> &instrs, Expression_tree * tree){

}

