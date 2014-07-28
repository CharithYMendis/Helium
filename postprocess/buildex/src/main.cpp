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
#include "print_halide.h"

#define MAX_STRING_LENGTH 200
 
 using namespace std;


 string get_filename(string folder, string filename, int number ,string extension){

	 if (number == -1){
		 return folder + "\\" + filename + "." + extension;
	 }
	 else{
		 return folder + "\\" + filename + "_" + to_string(number) + "." + extension;
	 }
 }

 int main(int argc, char ** argv){

	ifstream in;
	ofstream out;
	ofstream halide_out;


	string folder;
	string out_prefix; 

	int start_trace;
	unsigned long long dest_to_track;
	int end_trace;

	bool given_start = false;
	bool given_end = false;

	//argc should satisfy this assertion
	ASSERT_MSG((argc >= 3) && (argc != 6),("ERROR: incorrect number of arguments\n"));

	/* argument processing */
	
	/* folder */
	if (argc >= 2){
		folder.assign(argv[1]);
	}

	/* input file - instrace */
	if (argc >= 3){
		string name = get_filename(folder, argv[2], -1, "txt");
		in.open(name, std::ifstream::in);
		ASSERT_MSG(in.good(), ("%s file not existing?\n", name));
		DEBUG_PRINT(("exploring trace for file - %s\n",name.c_str()), 1);
	}

	/* expression output filename prefix */
	if (argc >= 4){
		out_prefix.assign(argv[3]);
	}

	/* halide output */
	if (argc >= 5){
		string name = get_filename(folder, argv[4], -1, "cpp");
		halide_out.open(name, std::ofstream::out);
		ASSERT_MSG(halide_out.good(), ("%s file not existing?\n", name));
		DEBUG_PRINT(("halide output file - %s\n", name.c_str()), 1);
	}

	/* starting line no and destination */
	if (argc >= 7){
		start_trace = atoi(argv[5]);
		dest_to_track = stoull(argv[6]);
		DEBUG_PRINT(("starting trace from line %d for %llu destination\n", start_trace, dest_to_track),1);
		given_start = true;
	}

	/* ending line number */
	if (argc >= 8){
		end_trace = atoi(argv[7]);
		DEBUG_PRINT(("ending trace at line %d\n", end_trace),1);
		given_end = true;
	}


	
	/* main application logic starts here */

	if (given_start){

		/* build the tree */
		Expression_tree * tree = new Expression_tree();
		end_trace = given_end ? end_trace : FILE_END;
		build_tree(dest_to_track, start_trace, end_trace, in, tree);


		/*print out the tree*/
		out.open(get_filename(folder, out_prefix, 0, "txt")); 
		print_node_tree(tree->get_head(), out);
		out.close();

		/* output to a dot file */
		out.open(get_filename(folder, out_prefix + "_dot", 0, "txt"));

		uint nodes = number_tree_nodes(tree->get_head());
		do_remove_signex(tree->get_head(), tree->get_head());
		print_to_dotfile(out, tree->get_head(), nodes, 0);
		
		out.close();

	}
	else{
		

		vector<mem_info_t *> mem_info;
		vector<mem_regions_t *> mem_regions;

		/* create the mem layout from the instrace */
		create_mem_layout(in, mem_info);
		print_mem_layout(mem_info);

		/* get input output mem regions and detailed information from the memory layout  - should ideally come from the dump */
		get_input_output_mem_regions(mem_info, mem_regions);
		print_mem_regions(mem_regions);

		bool ok; 

		/* get a random memory output location - this should be changed so that the location will always be from the output image */
		mem_regions_t * mem_region = get_random_output_region(mem_regions);
		uint64 mem_location = get_random_mem_location(mem_region, 1, 10, &ok);

		/* get the image location */
		vector<uint> base = get_mem_position(mem_region, mem_location);
		uint color = base[2];

		if (ok){
			DEBUG_PRINT(("random memory location - %llu\n",mem_location), 3);
		}

		vector<int> offset(DIMENSIONS, 0);
		vector<uint64> nbd_locations;

		nbd_locations.push_back(mem_location);

		uint rand_color = 0;


		/*get the mem locations of points surrounding the random memory location */
		for (int i = -1; i <= 1; i++){
			for (int j = -1; j <= 1; j++){
					
				if (i == 0 && j == 0) continue;

				rand_color++;
				rand_color %= 3;
					
				bool success;
				offset[0] = i; offset[1] = j; offset[2] = (int)rand_color - (int) color;
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
			remove_full_overlap_nodes_aggressive(tree->get_head(), tree->get_head(), 0);

			/*print the node dot file*/

			/*out.open(get_filename(folder, out_prefix + "_node", i, "txt"));
			uint nodes = number_tree_nodes(tree->get_head());
			print_node_tree(tree->get_head(), out);
			print_to_dotfile(out, tree->get_head(), nodes, i);
			out.close();*/

			/*build the abstract and  print the abs_node dot file */
			//out.open(get_filename(folder, out_prefix + "_abs_node", i, "txt"));
			Abs_tree  * abs_tree = new Abs_tree();
			abs_tree->build_abs_tree(NULL, tree->get_head(), mem_regions);
			//nodes = number_tree_nodes(abs_tree->head);
			//print_to_dotfile(out, abs_tree->head, nodes, i);
			//out.close();
			
			abs_nodes.push_back(abs_tree->head);

		}


		bool similar = Abs_tree::are_abs_trees_similar(abs_nodes);
		if (similar){

			cout << "similar" << endl;
			out.open(get_filename(folder, out_prefix + "_comp_node", 0, "txt"));
			Comp_Abs_tree * comp_tree = new Comp_Abs_tree();
			comp_tree->build_compound_tree(comp_tree->head, abs_nodes);
			uint nodes = number_tree_nodes(comp_tree->head);
			print_to_dotfile(out, comp_tree->head, nodes, 0);
			out.close();

			Comp_Abs_tree::abstract_buffer_indexes(comp_tree->head);

			Abs_node * final_tree = comp_tree->compound_to_abs_tree();
			out.open(get_filename(folder, out_prefix + "_node_final", 0, "txt"));
			nodes = number_tree_nodes(final_tree);
			print_to_dotfile(out, final_tree, nodes, 0, true);


			Halide_program * program = new Halide_program(final_tree);

			vector<Abs_node *> stack;
			program->seperate_to_Funcs(program->head, stack);
			program->print_seperated_funcs();
			program->print_halide(halide_out);


		}

	}


	if (argc >= 2)
		in.close();
	if (argc >= 4)
		halide_out.close();
	
	return 0;

 }



 