 //main application - read a instrace in a file and then build an expression tree
 //you need to choose where to start and end the trace and other criteria in this file's implementation.
 #include <stdio.h>
 #include <iostream>
 #include <vector>
 #include <string.h>
 #include <sstream>
 #include "exbuild.h"
 #include "node.h"
 #include "canonicalize.h"
 #include "assert.h"
 #include "fileparser.h"
 #include <stdlib.h>
 #include <fstream>
 #include "defines.h"
#include "plainprint.h"

#define MAX_STRING_LENGTH 200
 
 using namespace std;

 int main(int argc, char ** argv){
	
	//remember backwards traversal
	//arguments - file name, file_type, {start line (instr) and end line (instr)} / {heap mem addr, its location in the instrace}
	
	
	//the 5 arguments are as follows.
	//1. name of the program
	//2. in file
	//3. out file
	//4. line to start tracking -> gives fexibility to start from a mid point in the execution trace
	//5. destination to start tracking (the value field)
	//6. line to end tracking

	ifstream in;
	ofstream out;

	int start_trace;
	unsigned long long dest_to_track;
	int end_trace;

	bool given_start = false;
	bool given_end = false;

	//argc should satisfy this assertion
	ASSERT_MSG((argc >= 2) && (argc != 4),("ERROR: incorrect number of arguments\n"));

	//fill up the variables from the arguments - prints out messages for information purposes
	if (argc >= 2){
		in.open(argv[1], std::ifstream::in);
		ASSERT_MSG(in.good(), ("%s file not existing?\n", argv[1]));
		DEBUG_PRINT(("exploring trace for file - %s\n",argv[1]),1);
	}

	if (argc >= 3){
		out.open(argv[2], std::ofstream::out);
		ASSERT_MSG(in.good(), ("%s file not existing?\n", argv[2]));
		DEBUG_PRINT(("output file - %s\n", argv[2]),1);
	}

	if (argc >= 5){
		start_trace = atoi(argv[3]);
		dest_to_track = stoull(argv[4]);
		DEBUG_PRINT(("starting trace from line %d for %llu destination\n", start_trace, dest_to_track),1);
		given_start = true;
	}

	if (argc >= 6){
		end_trace = atoi(argv[5]);
		DEBUG_PRINT(("ending trace at line %d\n", end_trace),1);
		given_end = true;
	}


	cinstr_t * instr;
	int no_rinstrs;
	rinstr_t * rinstr;
	Expression_tree * tree = new Expression_tree();


	int curpos = 0;

	//we need to scroll to the place from where we need to build the instrace
	if (given_start){

		
		char dummystr[MAX_STRING_LENGTH];

		while (curpos < start_trace - 1){
			in.getline(dummystr, MAX_STRING_LENGTH);
			curpos++;
			ASSERT_MSG(!(in.fail() || in.eof()),("ERROR: went past the end of the file\n"));
		}

		//now we need to read the next line and start from the correct destination
		bool dest_present = false;
		int index = -1;

		//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well.

		instr = get_next_from_ascii_file(in);
		curpos++;
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs);
		for (int i = no_rinstrs - 1; i >= 0; i--){
			if (rinstr[i].dst.value == dest_to_track){
				ASSERT_MSG((rinstr[i].dst.type != IMM_FLOAT_TYPE) && (rinstr[i].dst.type != IMM_INT_TYPE),("ERROR: dest cannot be an immediate\n"));
				index = i;
				dest_present = true;
				break;
			}
		}

		ASSERT_MSG((dest_present == true) && (index >= 0),("ERROR: couldn't find the dest to start trace\n")); //we should have found the destination

		//do the initial processing
		for (int i = index; i >= 0; i--){
			tree->update_frontier(&rinstr[i]);
		}

	}

	
	//do the rest of expression tree building
	while(!in.eof()){
		instr = get_next_from_ascii_file(in);
		curpos++;
		if (instr != NULL){
			rinstr = cinstr_to_rinstrs(instr, no_rinstrs);
			if (given_start){  // if a starting point and a destination is given build the tree; otherwise it is a walk through the instructions
				for (int i = no_rinstrs - 1; i >= 0; i--){
					tree->update_frontier(&rinstr[i]);
				}
			}
		}

		if (given_end && (curpos == end_trace)){  //if an end point is given then we break before EOF
			break;
		}

	}

	if (given_start){
		flatten_to_expression(tree->get_head(),out);
	}

	if (argc >= 2)
		in.close();
	if (argc >= 3)
		out.close();
	
	return 0;

 }


 