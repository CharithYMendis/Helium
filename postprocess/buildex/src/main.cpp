 //main application - read a instrace in a file and then build an expression tree
 //you need to choose where to start and end the trace and other criteria in this file's implementation.

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "expression_tree.h"
#include "node.h"
#include "canonicalize.h"
#include "assert.h"
#include "fileparser.h"
#include "defines.h"
#include "tree_transformations.h"

#include "build_tree.h"
#include "build_mem_instrace.h"
#include "build_mem_dump.h"
#include "build_abs_tree.h"

#include "print_common.h"
#include "print_dot.h"
#include "print_halide.h"

#include "utilities.h"
#include "meminfo.h"
#include "imageinfo.h"

 using namespace std;

 bool debug = false;
 uint32_t debug_level = 0;
 ofstream log_file;

 void test_linear_solver();


 void print_usage(){
	 printf("usage - format -<name> <value>\n");
	 printf("\t exec - the executable which DR analyzed (without exe) \n");
	 printf("\t thread_id - the thread id that the instrace file belongs to\n");
	 printf("\t start_line - tracing start line\n");
	 printf("\t dest - the destination to be tracked\n");
	 printf("\t end_line - tracing end line\n");
	 printf("\t in_image - the in_image filename with ext \n");
	 printf("\t out_image - the out_image filename with ext \n");
	 printf("\t config - the middle name of the config file config_<name>.log\n");
	 printf("\t debug - 1,0 which turns debug mode on/off \n");
	 printf("\t debug_level - the level of debugging (higher means more debug info) \n");
 }

 int main(int argc, char ** argv){

	 /* setting up the files and other inputs and outputs for the program */

	 string process_name(argv[0]);
	 string exec;
	 string in_image;
	 string out_image;
	 string config;
	
	 int32_t start_trace = FILE_BEGINNING;
	 uint64_t dest = 0;
	 uint32_t stride = 0;
	 int32_t end_trace = FILE_ENDING;
	 int32_t thread_id = -1;


	 /***************************** command line args processing ************************/
	 vector<cmd_args_t *> args = get_command_line_args(argc, argv);

	 if (args.size() == 0){
		 print_usage();
		 exit(0);
	 }
	 
	 for (int i = 0; i < args.size(); i++){
		 cout << args[i]->name << " " << args[i]->value << endl;
		 if (args[i]->name.compare("-exec") == 0){
			 exec = args[i]->value;
		 }
		 else if (args[i]->name.compare("-thread_id") == 0){
			 thread_id = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-start_line") == 0){
			 start_trace = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-dest") == 0){
			 dest = strtoull(args[i]->value.c_str(), NULL, 10);
		 }
		 else if (args[i]->name.compare("-end_line") == 0){
			 end_trace = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-in_image") == 0){
			 in_image = args[i]->value;
		 }
		 else if (args[i]->name.compare("-out_image") == 0){
			 out_image = args[i]->value;
		 }
		 else if (args[i]->name.compare("-config") == 0){
			 config = args[i]->value;
		 }
		 else if (args[i]->name.compare("-debug") == 0){
			 debug = args[i]->value[0] - '0';
		 }
		 else if (args[i]->name.compare("-debug_level") == 0){
			 debug_level = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-stride") == 0){
			 stride = atoi(args[i]->value.c_str());
		 }
	 }

	 ASSERT_MSG(!exec.empty(), ("exec must be specified\n"));
	 ASSERT_MSG((!in_image.empty()) && (!out_image.empty()), ("image must be specified\n"));
	 
	 /********************************open the files************************************/

	 /*get all the files in output folder*/
	 string output_folder = get_standard_folder("output");
	 vector<string> files = get_all_files_in_folder(output_folder);
	 
	 /* inputs */
	 ifstream		  instrace_file;
	 ifstream		  app_pc_file;
	 ifstream		  config_file;
	 ifstream		  disasm_file;

	 string			  disasm_filename;
	 string			  instrace_filename;
	 string			  app_pc_filename;
	 string			  config_filename;
	 string			  in_image_filename;
	 string			  out_image_filename;
	 vector<string>	  memdump_files;
	 

	 if (thread_id != -1){ /* here we can get a specific instrace file*/
		 instrace_filename = get_standard_folder("output") + "\\instrace_" + exec + "_" + to_string(thread_id) + ".log";
		 instrace_file.open(instrace_filename, ifstream::in);
	 }
	 else{ /* get the instrace file with the largest size */
		 struct _stat buf;
		 int64_t max_size = -1;
		 /* get the instrace files for this exec */
		 for (int i = 0; i < files.size(); i++){
			 if (is_prefix(files[i], "instrace_" + exec)){
				 /*open the file*/
				 string file = output_folder + "\\" + files[i];
				 _stat(file.c_str(), &buf);
				 if (max_size < buf.st_size){
					 max_size = buf.st_size;
					 instrace_filename = file;
				 }

			 }
		 }
		 ASSERT_MSG((!instrace_filename.empty()), ("suitable instrace file cannot be located; please specify manually\n"));
		 instrace_file.open(instrace_filename, ifstream::in);
	 }
	 ASSERT_MSG(instrace_file.good(), ("instrace file cannot be opened\n"));

	 if (debug){
		 struct _stat buf;
		 int64_t max_size = -1;
		 /* get the instrace files for this exec */
		 for (int i = 0; i < files.size(); i++){
			 if (is_prefix(files[i], "instrace_" + exec + ".exe_" + "low.png" + "_asm_instr")){
				 /*open the file*/
				 string file = output_folder + "\\" + files[i];
				 _stat(file.c_str(), &buf);
				 if (max_size < buf.st_size){
					 max_size = buf.st_size;
					 disasm_filename = file;
				 }

			 }
		 }
		 ASSERT_MSG((!disasm_filename.empty()), ("suitable disasm file cannot be located; please specify manually\n"));
		 disasm_file.open(disasm_filename, ifstream::in);
		 ASSERT_MSG(disasm_file.good(), ("disasm file cannot be opened\n"));
	 }
	 

	 for (int i = 0; i < files.size(); i++){
		 if (is_prefix(files[i], "memdump_" + exec)){
			 memdump_files.push_back(output_folder + "\\" + files[i]);
		 }
	 }

	 /* get the images */
	 in_image_filename = get_standard_folder("image") + "\\" + in_image;
	 out_image_filename = get_standard_folder("image") + "\\" + out_image;
	 /* get the app_pcs to track files */
	 app_pc_filename = output_folder + "\\filter_funcs_" + exec + ".exe_app_pc.log";
	 app_pc_file.open(app_pc_filename, ifstream::in);
	 /* get the image mem config files - these have common configs for a given image processing program like Photoshop (hardcoded)  */
	 config_filename = get_standard_folder("filter") + "\\config_" + config + ".log";
	 config_file.open(config_filename, ifstream::in);

	 /* outputs */
	 ofstream concrete_tree_file;
	 ofstream compound_tree_file;
	 ofstream abs_tree_file;
	 ofstream expression_file;
	 ofstream halide_file;

	 /*check whether process name has .exe or not*/
	 size_t find = process_name.find(".exe");
	 if (find != string::npos){
		 process_name = process_name.substr(0, find);
	 }

	 string file_substr("\\" + process_name + "_" + exec);

	 if (debug){
		 /* get the log file */
		 log_file.open(get_standard_folder("log") + file_substr + ".log", ofstream::out);
		 /* get the concrete tree file - dot file */
		 concrete_tree_file.open(output_folder + file_substr + "_conctree.dot", ofstream::out);
		 /* get the compound tree file */
		 compound_tree_file.open(output_folder + file_substr + "_comptree.dot", ofstream::out);
		 /* get the abstract tree file */
		 abs_tree_file.open(output_folder + file_substr + "_abstree.dot", ofstream::out);
	     /* get the pseudo expression file */
		 expression_file.open(output_folder + file_substr + "_expression.log", ofstream::out);
	 }

	 /* create halide output file */
	 halide_file.open(get_standard_folder("halide") + file_substr + "_halide.cpp", ofstream::out);

	
	 /**************************algorithm********************************************/

	 /* main application logic starts here */
	 /* debugging printfs */
	 printf("instrace file - %s\n", instrace_filename.c_str());
	 printf("config file - %s\n", config_filename.c_str());
	 printf("app_pc file - %s\n", app_pc_filename.c_str());
	 printf("in image file - %s\n", in_image_filename.c_str());
	 printf("out image file - %s\n", out_image_filename.c_str());
	 if (debug){
		 printf("disasm file - %s\n", disasm_filename.c_str());
	 }
	 printf("memdump files - \n");
	 for (int i = 0; i < memdump_files.size(); i++){
		 printf("%s\n", memdump_files[i].c_str());
	 }

	 ULONG_PTR token = initialize_image_subsystem();

	 DEBUG_PRINT(("getting disassembly trace\n"), 1);

	 vector<disasm_t *> disasm;
	 if (debug){
		 disasm = parse_debug_disasm(disasm_file);
	 }

	 DEBUG_PRINT(("analyzing mem dumps\n"), 1);

	 vector<mem_regions_t *> regions = get_image_regions_from_dump(memdump_files, in_image_filename, out_image_filename);

	 DEBUG_PRINT(("analyzing instrace file - %s\n", instrace_filename.c_str()), 1);

	 vector<mem_info_t *> mem_info;
	 create_mem_layout(instrace_file, mem_info);
	 link_mem_regions_greedy(mem_info, 0);
	 print_mem_layout(log_file, mem_info);

	 vector<mem_regions_t *> final_regions = merge_instrace_and_dump_regions(mem_info, regions);
	 
	 if (start_trace == FILE_BEGINNING){
		 mem_regions_t * mem_region = get_random_output_region(final_regions);
		 uint64 mem_location = get_random_mem_location(mem_region, 20);
		 DEBUG_PRINT(("random mem location we got - %llx\n", mem_location), 1);
		 dest = mem_location;
		 stride = mem_region->stride;
	 }

	 
	 instrace_file.clear();
	 instrace_file.seekg(instrace_file.beg);

	 Expression_tree * tree = new Expression_tree();

	 DEBUG_PRINT(("track info - dest %llx stride %d\n", dest, stride), 1);

	 if (start_trace == FILE_BEGINNING){
		 start_trace = go_to_line_dest(instrace_file, dest, stride);
	 }
	 DEBUG_PRINT(("line no - %d\n", start_trace), 1);

	 instrace_file.clear();
	 instrace_file.seekg(instrace_file.beg);

	 ASSERT_MSG((start_trace != 0), ("ERROR: the selected destination does not exist\n"));

	 //first build the tree for this memory location for sanity purposes
	 build_tree(dest, start_trace, end_trace, instrace_file, tree, disasm);
	 do_remove_signex(tree->get_head(), tree->get_head());
	 remove_full_overlap_nodes_aggressive(tree->get_head(), tree->get_head(), 0);
 
	 uint nodes = number_tree_nodes(tree->get_head());
	 DEBUG_PRINT(("printing to dot file...\n"), 2);
	 print_to_dotfile(concrete_tree_file, tree->get_head(),nodes,0);
	 print_node_tree(tree->get_head(), expression_file);

	 Abs_tree  * abs_tree = new Abs_tree();
	 abs_tree->build_abs_tree(NULL, tree->get_head(), final_regions);
	 nodes = number_tree_nodes(abs_tree->head);
	 print_to_dotfile(abs_tree_file, abs_tree->head, nodes, 0, false);




	 shutdown_image_subsystem(token);
	 exit(0);


	
	 

	 
	
	//if (given_start){

	//	/* build the tree */
	//	expression_tree * tree = new expression_tree();
	//	end_trace = given_end ? end_trace : file_end;
	//	build_tree(dest_to_track, start_trace, end_trace, in, tree);


	//	/*print out the tree*/
	//	out.open(get_filename(folder, out_prefix, 0, "txt")); 
	//	print_node_tree(tree->get_head(), out);
	//	out.close();

	//	/* output to a dot file */
	//	out.open(get_filename(folder, out_prefix + "_dot", 0, "txt"));

	//	uint nodes = number_tree_nodes(tree->get_head());
	//	do_remove_signex(tree->get_head(), tree->get_head());
	//	print_to_dotfile(out, tree->get_head(), nodes, 0);
	//	
	//	out.close();

	//}
	//else{
	//	

	//	vector<mem_info_t *> mem_info;
	//	vector<mem_regions_t *> mem_regions;

	//	/* create the mem layout from the instrace */
	//	create_mem_layout(in, mem_info);
	//	print_mem_layout(mem_info);

	//	/* get input output mem regions and detailed information from the memory layout  - should ideally come from the dump */
	//	get_input_output_mem_regions(mem_info, mem_regions);
	//	print_mem_regions(mem_regions);

	//	bool ok; 


	//	/* get a random memory output location - this should be changed so that the location will always be from the output image */
	//	mem_regions_t * mem_region = get_random_output_region(mem_regions);
	//	uint64 mem_location = get_random_mem_location(mem_region, 1, 10, &ok);

	//	/* get the image location */
	//	vector<uint> base = get_mem_position(mem_region, mem_location);
	//	uint color = base[2];

	//	if (ok){
	//		DEBUG_PRINT(("random memory location - %llu\n",mem_location), 3);
	//	}

	//	vector<int> offset(DIMENSIONS, 0);
	//	vector<uint64> nbd_locations;

	//	nbd_locations.push_back(mem_location);

	//	uint rand_color = 0;


	//	/*get the mem locations of points surrounding the random memory location */
	//	for (int i = -1; i <= 1; i++){
	//		for (int j = -1; j <= 1; j++){
	//				
	//			if (i == 0 && j == 0) continue;

	//			rand_color++;
	//			rand_color %= 3;
	//				
	//			bool success;
	//			offset[0] = i; offset[1] = j; offset[2] = (int)rand_color - (int) color;
	//			uint64 offset_mem_location = get_mem_location(base, offset, mem_region, &success);

	//			if (success){
	//				DEBUG_PRINT(("memory location - %llu\n", offset_mem_location), 3);
	//				nbd_locations.push_back(offset_mem_location);
	//			}


	//		}
	//	}


	//	vector<Abs_node *> abs_nodes; 

	//	for (int i = 0; i < nbd_locations.size(); i++){


	//		Expression_tree * tree = new Expression_tree();

	//		in.clear();
	//		in.seekg(in.beg);

	//		DEBUG_PRINT(("track info - dest %llu stride %d\n", nbd_locations[i], mem_region->stride), 1);
	//		uint lineno = go_to_line_dest(in, nbd_locations[i], mem_region->stride);
	//		DEBUG_PRINT(("line no - %d\n", lineno), 1);

	//		ASSERT_MSG((lineno != 0), ("ERROR: the selected destination does not exist\n"));
	//		in.clear();
	//		in.seekg(in.beg);

	//		build_tree(nbd_locations[i], lineno, FILE_END, in, tree);
	//		do_remove_signex(tree->get_head(), tree->get_head());
	//		remove_full_overlap_nodes_aggressive(tree->get_head(), tree->get_head(), 0);

	//		/*print the node dot file*/

	//		/*out.open(get_filename(folder, out_prefix + "_node", i, "txt"));
	//		uint nodes = number_tree_nodes(tree->get_head());
	//		print_node_tree(tree->get_head(), out);
	//		print_to_dotfile(out, tree->get_head(), nodes, i);
	//		out.close();*/

	//		/*build the abstract and  print the abs_node dot file */
	//		//out.open(get_filename(folder, out_prefix + "_abs_node", i, "txt"));
	//		Abs_tree  * abs_tree = new Abs_tree();
	//		abs_tree->build_abs_tree(NULL, tree->get_head(), mem_regions);
	//		//nodes = number_tree_nodes(abs_tree->head);
	//		//print_to_dotfile(out, abs_tree->head, nodes, i);
	//		//out.close();
	//		
	//		abs_nodes.push_back(abs_tree->head);

	//	}


	//	bool similar = Abs_tree::are_abs_trees_similar(abs_nodes);
	//	if (similar){

	//		cout << "similar" << endl;
	//		out.open(get_filename(folder, out_prefix + "_comp_node", 0, "txt"));
	//		Comp_Abs_tree * comp_tree = new Comp_Abs_tree();
	//		comp_tree->build_compound_tree(comp_tree->head, abs_nodes);
	//		uint nodes = number_tree_nodes(comp_tree->head);
	//		print_to_dotfile(out, comp_tree->head, nodes, 0);
	//		out.close();

	//		Comp_Abs_tree::abstract_buffer_indexes(comp_tree->head);

	//		Abs_node * final_tree = comp_tree->compound_to_abs_tree();
	//		out.open(get_filename(folder, out_prefix + "_node_final", 0, "txt"));
	//		nodes = number_tree_nodes(final_tree);
	//		print_to_dotfile(out, final_tree, nodes, 0, true);


	//		Halide_program * program = new Halide_program(final_tree);

	//		vector<Abs_node *> stack;
	//		program->seperate_to_Funcs(program->head, stack);
	//		program->print_seperated_funcs();
	//		program->print_halide(halide_out);


	//	}

	//}


	//if (argc >= 2)
	//	in.close();
	//if (argc >= 4)
	//	halide_out.close();
	//
	//return 0;

 }


 



 