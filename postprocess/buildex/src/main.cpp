 //main application - read a instrace in a file and then build an expression tree
 //you need to choose where to start and end the trace and other criteria in this file's implementation.
 #include <stdio.h>
 #include <iostream>
 #include <vector>
 #include <string.h>
 #include <sstream>
 #include "expression_tree.h"
 #include "node.h"
 #include "canonicalize.h"
 #include "assert.h"
 #include "fileparser.h"
 #include <stdlib.h>
 #include <fstream>
 #include "defines.h"
#include "print.h"
#include "build_tree.h"
#include "build_mem_layout.h"

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


	Expression_tree * tree = new Expression_tree();


	if (given_start){

		end_trace = given_end ? end_trace : FILE_END;

		/* create the mem_layout */
		vector<mem_info_t *> mem_info;
		create_mem_layout(in, mem_info);
		print_mem_layout(mem_info);

		in.clear();
		in.seekg(in.beg);
	
		build_tree(dest_to_track, start_trace, end_trace, in, tree);
		flatten_to_expression(tree->get_head(), out);

	}
	else{
		/* this is for experimentation */
		//walk_instructions(in);
		vector<mem_info_t *> mem_info;
		create_mem_layout(in, mem_info);
		print_mem_layout(mem_info);

		in.clear();
		in.seekg(in.beg);

		uint64_t dest;
		uint32_t stride;
		uint32_t lineno;

		random_dest_select(mem_info, &dest, &stride);

		DEBUG_PRINT(("track info - dest %llu stride %d\n", dest, stride), 3);

		lineno = go_to_line_dest(in, dest, stride);

		DEBUG_PRINT(("line no - %d\n", lineno), 3);

		ASSERT_MSG((lineno != 0), ("ERROR: the selected destination does not exist\n"));

		in.clear();
		in.seekg(in.beg);

		build_tree(dest, lineno, FILE_END, in, tree);

		flatten_to_expression(tree->get_head(), out);

		//walk_instructions(in);

	}

	delete tree;

	if (argc >= 2)
		in.close();
	if (argc >= 3)
		out.close();
	
	return 0;

 }


 