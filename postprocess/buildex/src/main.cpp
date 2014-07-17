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
#include "print_common.h"
#include "build_tree.h"
#include "build_mem_instrace.h"
#include "build_mem_dump.h"
#include "print_dot.h"
#include "tree_transformations.h"
#include "build_abs_tree.h"

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
	ofstream halide_out;

	int start_trace;
	unsigned long long dest_to_track;
	int end_trace;

	bool given_start = false;
	bool given_end = false;

	//argc should satisfy this assertion
	ASSERT_MSG((argc >= 2) && (argc != 5),("ERROR: incorrect number of arguments\n"));

	//fill up the variables from the arguments - prints out messages for information purposes
	if (argc >= 2){
		in.open(argv[1], std::ifstream::in);
		ASSERT_MSG(in.good(), ("%s file not existing?\n", argv[1]));
		DEBUG_PRINT(("exploring trace for file - %s\n",argv[1]), 1);
	}

	if (argc >= 3){
		out.open(argv[2], std::ofstream::out);
		ASSERT_MSG(out.good(), ("%s file not existing?\n", argv[2]));
		DEBUG_PRINT(("output file - %s\n", argv[2]), 1);
	}

	if (argc >= 4){
		halide_out.open(argv[3], std::ofstream::out);
		ASSERT_MSG(halide_out.good(), ("%s file not existing?\n", argv[3]));
		DEBUG_PRINT(("halide output file - %s\n", argv[3]), 1);
	}

	if (argc >= 6){
		start_trace = atoi(argv[4]);
		dest_to_track = stoull(argv[5]);
		DEBUG_PRINT(("starting trace from line %d for %llu destination\n", start_trace, dest_to_track),1);
		given_start = true;
	}

	if (argc >= 7){
		end_trace = atoi(argv[6]);
		DEBUG_PRINT(("ending trace at line %d\n", end_trace),1);
		given_end = true;
	}


	


	if (given_start){

		Expression_tree * tree = new Expression_tree();

		end_trace = given_end ? end_trace : FILE_END;

		/* create the mem_layout */
		/*vector<mem_info_t *> mem_info;
		create_mem_layout(in, mem_info);
		print_mem_layout(mem_info);

		in.clear();
		in.seekg(in.beg);*/
	
		build_tree(dest_to_track, start_trace, end_trace, in, tree);
		//flatten_to_expression(tree->get_head(), out);

		uint nodes = number_expression_tree(tree->get_head());

		do_remove_signex(tree->get_head(), tree->get_head());

		cout << nodes << endl;

		print_to_dotfile(out, tree->get_head(), nodes, 0);


	}
	else{
		/* this is for experimentation */
		//walk_instructions(in);
		vector<mem_info_t *> mem_info;
		vector<mem_regions_t *> mem_regions;

		create_mem_layout(in, mem_info);
		print_mem_layout(mem_info);

		get_input_output_mem_regions(mem_info, mem_regions);
		print_mem_regions(mem_regions);

		bool ok; 
		mem_regions_t * mem_region = get_random_output_region(mem_regions);
		uint64 mem_location = get_random_mem_location(mem_region, 1, 10, &ok);
		vector<uint> base = get_mem_position(mem_region, mem_location);
		uint color = base[2];

		if (ok){
			DEBUG_PRINT(("random memory location - %llu\n",mem_location), 3);
		}

		vector<int> offset(DIMENSIONS, 0);

		vector<uint64> nbd_locations;
		nbd_locations.push_back(mem_location);


		/*get the mem locations of points surrounding the random memory location */
		for (int i = -1; i <= 1; i++){
			for (int j = -1; j <= 1; j++){
					
				if (i == 0 && j == 0) continue;
					
				bool success;
				offset[0] = i; offset[1] = j; offset[2] = 0;
				uint64 offset_mem_location = get_mem_location(base, offset, mem_region, &success);

				if (success){
					DEBUG_PRINT(("memory location - %llu\n", offset_mem_location), 3);
					nbd_locations.push_back(offset_mem_location);
				}


			}
		}


		vector<Abs_node *> abs_nodes; 

		for (int i = 0; i < nbd_locations.size(); i++){


			Expression_tree * tree = new Expression_tree();

			in.clear();
			in.seekg(in.beg);

			DEBUG_PRINT(("track info - dest %llu stride %d\n", nbd_locations[i], mem_region->stride), 1);
			uint lineno = go_to_line_dest(in, nbd_locations[i], mem_region->stride);
			DEBUG_PRINT(("line no - %d\n", lineno), 1);

			ASSERT_MSG((lineno != 0), ("ERROR: the selected destination does not exist\n"));
			in.clear();
			in.seekg(in.beg);

			build_tree(nbd_locations[i], lineno, FILE_END, in, tree);

			
			do_remove_signex(tree->get_head(), tree->get_head());

			/*uint nodes = number_expression_tree(tree->get_head());
			print_to_dotfile(out, tree->get_head(), nodes, i);*/

			Abs_tree  * abs_tree = new Abs_tree();
			abs_tree->build_abs_tree(NULL, tree->get_head(), mem_regions);
			abs_nodes.push_back(abs_tree->head);

			

		}





		/*in.clear();
		in.seekg(in.beg);

		uint64_t dest;
		uint32_t stride;
		uint32_t lineno;

		random_dest_select(mem_info, &dest, &stride);

		DEBUG_PRINT(("track info - dest %llu stride %d\n", dest, stride), 1);

		lineno = go_to_line_dest(in, dest, stride);

		DEBUG_PRINT(("line no - %d\n", lineno), 1);

		ASSERT_MSG((lineno != 0), ("ERROR: the selected destination does not exist\n"));

		in.clear();
		in.seekg(in.beg);



		build_tree(dest, lineno, FILE_END, in, tree);

		//flatten_to_expression(tree->get_head(), out);

		uint nodes = number_tree(tree->get_head());

		do_remove_signex(tree->get_head(), tree->get_head());

		print_to_dotfile(out, tree->get_head(), nodes);

		//walk_instructions(in);*/

	}


	if (argc >= 2)
		in.close();
	if (argc >= 3)
		out.close();
	if (argc >= 4)
		halide_out.close();
	
	return 0;

 }


 /*
 
 //testing

	 double A1[] = { 0, 1, 1 };
	 double A2[] = { 2, 4, -2 };
	 double A3[] = { 0, 3, 15 };

	 vector<double> A1_vec(A1, A1 + 3);
	 vector<double> A2_vec(A2, A2 + 3);
	 vector<double> A3_vec(A3, A3 + 3);

	 vector<vector<double> > A;
	 A.push_back(A1_vec);
	 A.push_back(A2_vec);
	 A.push_back(A3_vec);

	 double b1[] = { 4, 2, 36 };
	 vector<double> b(b1, b1 + 3);

	 vector<double> results = solve_linear_eq(A, b);

	 for (int i = 0; i < results.size(); i++){
		 cout << results[i] << endl;
	 }

	 exit(0);
 
 */

 