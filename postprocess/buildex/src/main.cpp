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

#include  "..\..\..\dr_clients\include\output.h"
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

#include "sympy.h"

 using namespace std;

 bool debug = false;
 uint32_t debug_level = 0;
 ofstream log_file;

 void													test_linear_solver();
 vec_cinstr filter_instr_trace(uint32_t start_pc, uint32_t end_pc, vec_cinstr &unfiltered_instrs);
 
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
	 printf("\t seed - the seed to select the random memory point\n");
	 printf("\t stride - the memory width at the particular location\n");
	 printf("\t tree_build - the tree building method \"random - 1\",\"random set - 2\",\"all - 3\"\n");
	 printf("\t mode - the mode in which this tool is running mem_info_stage, tree_build_stage, abstraction_stage, halide_output_stage\n");
 }

 /* tree build modes */
#define BUILD_RANDOM		1
#define BUILD_RANDOM_SET	2
#define	BUILD_SIMILAR		3
#define BUILD_CLUSTERS		4


 /* stage to stop */
#define MEM_INFO_STAGE			1	
#define TREE_BUILD_STAGE		2
#define ABSTRACTION_STAGE		3
#define HALIDE_OUTPUT_STAGE		4

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
	 uint32_t start_pc = 0;
	 uint32_t end_pc = 0;
	 uint32_t seed = 10;
	 uint32_t tree_build = BUILD_RANDOM;
	 uint32_t mode = TREE_BUILD_STAGE;


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
		 else if (args[i]->name.compare("-start_pc") == 0){
			 start_pc = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-seed") == 0){
			 seed = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-tree_build") == 0){
			 tree_build = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-mode") == 0){
			 mode = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-end_pc") == 0){
			 end_pc = atoi(args[i]->value.c_str());
		 }
		 else{
			 ASSERT_MSG(false, ("ERROR: unknown option\n"));
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
			 if (is_prefix(files[i], "instrace_" + exec + ".exe_" + in_image + "_asm_instr")){
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
	 ofstream algebric_tree_file;
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
		 /* final algebric filter file */
		 algebric_tree_file.open(output_folder + file_substr + "_algebric.dot", ofstream::out);
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

	 /********************************MEMORY INFORMATION***************************************/

	 /* disassembly of instructions acquired */
	 DEBUG_PRINT(("getting disassembly trace\n"), 1);
	 vector<disasm_t *> disasm;
	 if (debug){
		 disasm = parse_debug_disasm(disasm_file);
		 print_disasm(disasm);
	 }

	 /* analyzing mem dumps for input and output image locations */
	 DEBUG_PRINT(("analyzing mem dumps\n"), 1);
	 vector<mem_regions_t *> regions = get_image_regions_from_dump(memdump_files, in_image_filename, out_image_filename);
	 DEBUG_PRINT(("analyzing instrace file - %s\n", instrace_filename.c_str()), 1);

	 /* independently create the memory layout from the instrace */
	 vector<mem_info_t *> mem_info;
	 vector<pc_mem_region_t *> pc_mem_info;
	 create_mem_layout(instrace_file, mem_info);
	 create_mem_layout(instrace_file, pc_mem_info);
	 link_mem_regions_greedy(mem_info, 0);
	 link_mem_regions(pc_mem_info, 1);
	 print_mem_layout(log_file, mem_info);
	 print_mem_layout(log_file, pc_mem_info);


	 mem_info = extract_mem_regions(pc_mem_info);

	 /* merge these two information - instrace mem info + mem dump info */
	 vector<mem_regions_t *> total_mem_regions;
	 vector<mem_regions_t *> image_regions = merge_instrace_and_dump_regions(total_mem_regions, mem_info, regions);

	 print_mem_regions(total_mem_regions);

	 if (mode == MEM_INFO_STAGE){
		 exit(0);
	 }
	
	 /******************************************************************************************/
	 
	 /*******************************TREE BUILDING**********************************************/
	 
	 /* Q1. what points do I build the dependancy tree?  
			(i) capture random points and see if they are similar; otherwise repeat
			(ii) capture for all the points and then select a random sample of points with similar trees

		Q2. What if not enough similar points are there?
			e.g:- filters with big dependancies (box blur)
			(i) canonicalize and simplify the filters -> they should have the same form if not -> some input dependancy is there
			(ii) need to identify their iterative operation and represent it in some way (stop at intermediate points)
	 
	 */


	 /* get all the instructions and preprocess */
	 instrace_file.clear();
	 instrace_file.seekg(0, instrace_file.beg);

	 vector<disasm_t * > disasm_strings = parse_debug_disasm(disasm_file);
	 vec_cinstr instrs_forward_unfiltered = walk_file_and_get_instructions(instrace_file, disasm_strings);
	 /* need to filter unwanted instrs from the file we got */
	 vec_cinstr instrs_forward = filter_instr_trace(start_pc, end_pc, instrs_forward_unfiltered);
	
	 
	 /* make a copy for the backwards analysis */
	 vec_cinstr instrs_backward;
	 for (int i = instrs_forward.size() - 1; i >= 0; i--){
		 cinstr_t * new_cinstr = new cinstr_t(*instrs_forward[i].first);
		 pair<cinstr_t *, string *> instr_string = make_pair(new_cinstr, instrs_forward[i].second);
		 instrs_backward.push_back(instr_string);
	 }


	 DEBUG_PRINT(("number of dynamic instructions : %d\n", instrs_backward.size()), 2);

	
	 /*preprocessing*/
	 update_regs_to_mem_range(instrs_backward);
	 update_regs_to_mem_range(instrs_forward);

	 update_floating_point_regs(instrs_backward, BACKWARD_ANALYSIS, disasm, start_pc);
	 DEBUG_PRINT(("updated backward instr's floating point regs\n"), 2);
	 update_floating_point_regs(instrs_forward, FORWARD_ANALYSIS, disasm, start_pc);
	 DEBUG_PRINT(("updated forward instr's floating point regs\n"), 2);

	 /*for (int i = 0; i < instrs_backward.size(); i++){
		 ASSERT_MSG((instrs_backward[i] != instrs_forward[i]), ("ERROR: cannot be equal\n"));
		 cinstr_t * first = instrs_backward[i].first;
		 cinstr_t * second = instrs_forward[i].first;
		 ASSERT_MSG((first != second), ("ERROR: cannot be equal cinstr\n"));
		 ASSERT_MSG((first->srcs != second->srcs), ("ERROR: cannot be equal cinstr srcs\n"));
		 ASSERT_MSG((first->dsts != second->dsts), ("ERROR: cannot be equal cinstr dsts\n"));
	 }*/



	 /* data structures that will be passed to the next stage */
	 vector<Expression_tree *> conc_trees;

	 /* capture the function start points if the end trace is not given specifically */
	 vector<uint32_t> start_points;
	 vector<uint32_t> start_points_forward;
	 if (end_trace == FILE_ENDING){
		 start_points = get_instrace_startpoints(instrs_backward, start_pc);
		 start_points_forward = get_instrace_startpoints(instrs_forward, start_pc);
		 DEBUG_PRINT(("no of funcs captured - %d\n start points : \n", start_points.size()), 1);
		 for (int i = 0; i < start_points.size(); i++){
			 DEBUG_PRINT(("%d-", start_points[i]), 1);
		 }
		 DEBUG_PRINT(("\n"), 1);
	 }


	 if (tree_build == BUILD_RANDOM){

		 if (start_trace == FILE_BEGINNING){
			 mem_regions_t * random_mem_region = get_random_output_region(image_regions);
			 uint64 mem_location = get_random_mem_location(random_mem_region, seed);
			 DEBUG_PRINT(("random mem location we got - %llx\n", mem_location), 1);
			 dest = mem_location;
			 stride = random_mem_region->bytes_per_pixel;
		 }
		 else{
			 /* else I assume that the stride and the dest are set properly */
			 ASSERT_MSG((stride != 0 && dest != 0), ("ERROR: if the starting point is given please specify the dest and stride\n"));
		 }
		 DEBUG_PRINT(("func pc entry - %x\n", start_pc), 1);

		 //Node * node = create_tree_for_dest(dest, stride, instrace_file, start_points, start_trace, end_trace, disasm)->get_head();
		 Expression_tree * tree = new Expression_tree();
		 build_tree(dest, stride, start_points, start_trace, end_trace, tree, instrs_backward, disasm);
		 conc_trees.push_back(tree);
		
	 }
	 else if (tree_build == BUILD_RANDOM_SET){

		 /*ok we need find a set of random locations */
		 vector<uint64_t> nbd_locations = get_nbd_of_random_points(image_regions, seed, &stride);

		 /* ok now build trees for the set of locations */
		 for (int i = 0; i < nbd_locations.size(); i++){

			 Expression_tree * tree = new Expression_tree();
			 build_tree(nbd_locations[i], stride, start_points, FILE_BEGINNING, end_trace, tree, instrs_backward, disasm);
			 conc_trees.push_back(tree);
			 
		 }

		 /* checking similarity of the trees and if not repeat?? */
		 
		 vector< vector< Expression_tree * > > cat_trees = categorize_trees(conc_trees);

	 }
	 else if (tree_build == BUILD_SIMILAR){
		 conc_trees = get_similar_trees(image_regions, seed, &stride, instrs_backward, start_points, end_trace, disasm);
 	 }
	 else if (tree_build == BUILD_CLUSTERS){
		 vector< vector< Expression_tree *> > clustered_trees = cluster_trees(image_regions, start_points, instrs_backward, disasm, output_folder + file_substr);
	 }

	 /*debug printing*/
	 for (int i = 0; i < conc_trees.size(); i++){

		 //DEBUG_PRINT(("printing out the expression\n"), 2);
		 //ofstream expression_file(output_folder + file_substr + "_expression_" + to_string(i) + ".txt", ofstream::out);
		 //print_node_tree(conc_trees[i], expression_file);
		 //cout << get_simplify_string(conc_trees[i]) << endl;
		 uint no_nodes = number_tree_nodes(conc_trees[i]->get_head());
		 DEBUG_PRINT(("printing to dot file...\n"), 2);
		 ofstream conc_file(output_folder + file_substr + "_conctree_" + to_string(i) + ".dot", ofstream::out);
		 print_to_dotfile(conc_file, conc_trees[i]->get_head(), no_nodes, 0);
		
	 }

	 if (mode == TREE_BUILD_STAGE){
		 exit(0);
	 }

	 
	 /***************************************ABSTRACTION*********************************************************/

	 vector<Abs_node *> abs_nodes;
	 for (int i = 0; i < conc_trees.size(); i++){
		 Abs_tree  * abs_tree = new Abs_tree();
		 abs_tree->build_abs_tree(NULL, conc_trees[i]->get_head(), total_mem_regions);
		 abs_nodes.push_back(abs_tree->head);
	 }


	 /* debug printing */
	 for (int i = 0; i < abs_nodes.size(); i++){
		 uint32_t nodes = number_tree_nodes(abs_nodes[i]);
		 ofstream abs_file(output_folder + file_substr + "_abstree_" + to_string(i) + ".dot", ofstream::out);
		 print_to_dotfile(abs_file, abs_nodes[i], nodes, 0, false);
	 }


	 if (mode == ABSTRACTION_STAGE){
		 exit(0);
	 }


	 /**************************************HALIDE OUTPUT + ALGEBRIC FILTERS********************************************************/
	 
	bool similar = Abs_tree::are_abs_trees_similar(abs_nodes);
	if (similar){

		DEBUG_PRINT(("the trees are similar, now getting the algebric filter....\n"), 1);
		Comp_Abs_tree * comp_tree = new Comp_Abs_tree();
		comp_tree->build_compound_tree(comp_tree->head, abs_nodes);
		uint nodes = number_tree_nodes(comp_tree->head);
		print_to_dotfile(compound_tree_file, comp_tree->head, nodes, 0);
		

		Comp_Abs_tree::abstract_buffer_indexes(comp_tree->head);

		Abs_node * final_tree = comp_tree->compound_to_abs_tree();
		//nodes = number_tree_nodes(final_tree);
		print_to_dotfile(algebric_tree_file, final_tree, nodes, 0, true);


		Halide_program * program = new Halide_program(final_tree);

		vector<Abs_node *> stack;
		program->seperate_to_Funcs(program->head, stack);
		program->print_seperated_funcs();
		program->print_halide(halide_file);

	}
	else{
		DEBUG_PRINT(("the trees are not similar; please check\n"), 1);
	}

	 concrete_tree_file.close();
	 expression_file.close();
	 compound_tree_file.close();
	 expression_file.close();
	 algebric_tree_file.close();

	 shutdown_image_subsystem(token);
	 return 0;

 }


 vec_cinstr filter_instr_trace(uint32_t start_pc, uint32_t end_pc, vec_cinstr &unfiltered_instrs){


	 vec_cinstr instrs;
	 bool start = false;

	 for (int i = 0; i < unfiltered_instrs.size(); i++){
		 if (unfiltered_instrs[i].first->pc == start_pc){
			 start = true;
		 }
		 if (start) instrs.push_back(unfiltered_instrs[i]);
		 if (unfiltered_instrs[i].first->pc == end_pc){
			 start = false;
		 }
	 }

	 return instrs;


 }





 











 

 



 