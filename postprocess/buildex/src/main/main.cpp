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
#include <assert.h>
#include <algorithm>

#include  "..\..\..\dr_clients\include\output.h"
#include "utility\fileparser.h"
#include "utility\defines.h"

#include "analysis\tree_analysis.h"
#include "analysis\staticinfo.h"
#include "analysis\preprocess.h"
#include "analysis\conditional_analysis.h"
#include "analysis\indirection_analysis.h"

#include "memory/memregions.h"
#include "memory/memdump.h"
#include "memory/meminstrace.h"
#include "memory/memanalysis.h"

#include "halide/halide.h"

#include "utilities.h"
#include "meminfo.h"
#include "imageinfo.h"
#include "common_defines.h"

 using namespace std;

 bool debug = false;
 uint32_t debug_level = 0;
 ofstream log_file;

 uint32_t Tree::num_paras = 0;


 bool conctree_opt = true;
 bool abstree_opt = false;
 bool debug_tree = false;
 uint32_t fraction = 1;


 
 void print_usage(){
	 printf("usage - format -<name> <value>\n");
	 printf("\t exec - the executable which DR analyzed (with exe/EXE) \n");
	 printf("\t thread_id - the thread id that the instrace file belongs to\n");
	 
	 printf("\t start_trace - tracing start line\n");
	 printf("\t end_trace - tracing end line\n");
	 printf("\t dest - the destination to be tracked\n");
	 printf("\t stride - the memory width at the particular location\n");

	 printf("\t in_image - the in_image filename with ext \n");
	 printf("\t out_image - the out_image filename with ext \n");
	 printf("\t config - the middle name of the config file config_<name>.log\n");
	 
	 printf("\t debug - 1,0 which turns debug mode on/off \n");
	 printf("\t debug_level - the level of debugging (higher means more debug info) \n");
	 
	 printf("\t seed - the seed to select the random memory point\n");
	 printf("\t tree_build - the tree building method \"random - 1\",\"random set - 2\",\"similar - 3\",\"clustered - 4\"\n");
	 printf("\t mode - the mode in which this tool is running mem_info_stage, tree_build_stage, abstraction_stage, halide_output_stage\n");
 
	 printf("\t dump - whether the application memory dump should be used\n");
	 printf("\t start_pcs - a comma separated list of start of captured functions\n");
	 printf("\t end_pcs - a comma separated list of end of captured functions\n");

	 /*auxiliary variables*/
	 printf("\t version - version of the instrace - with and without addr calc information\n");
	 printf("\t skip - take the nth tree for abstraction\n");
	 printf("\t no_trees - number of trees to be included in the abstraction process\n");

	 printf("\t anaopt - analysis options\n");
	 printf("\t fraction - fraction of the trees to be built\n");
	 printf("\t abstree_opt - turn on abstract tree optimizations\n");
	 printf("\t conctree_opt - turn on conc tree optimizations\n");
	 printf("\t debug_tree - whether printing all the trees are enabled\n");

 }

 /* tree build modes */
#define BUILD_RANDOM		1
#define BUILD_RANDOM_SET	2
#define	BUILD_SIMILAR		3
#define BUILD_CLUSTERS		4


 /* stage to stop */
#define MEM_INFO_STAGE			1	
#define CONDITIONAL_STAGE		2
#define TREE_BUILD_STAGE		3
#define ABSTRACTION_STAGE		4
#define HALIDE_OUTPUT_STAGE		5

 /* analysis switch on off */
#define DEPENDANT_ANALYSIS		(1<<1)
#define CONDITIONAL_ANALYSIS	(1<<2)
#define	INPUT_REGION_SELECTION	(1<<3)
#define ALL_ANALYSIS			(0xFF)



 int main(int argc, char ** argv){

	 /* setting up the files and other inputs and outputs for the program */

	 /* file and process names */
	 string process_name(argv[0]);
	 string exec;
	 string in_image;
	 string out_image;
	 string config;

	 int32_t start_trace = FILE_BEGINNING;
	 int32_t end_trace = FILE_ENDING;
	 uint64_t dest = 0;
	 uint32_t stride = 0;
	 
	 int32_t thread_id = -1;
	 uint32_t start_pc = 0; /* start pc of the function */
	 uint32_t end_pc = 0; /* end pc of the function */
	 uint32_t seed = 10;

	 uint32_t tree_build = BUILD_RANDOM;
	 uint32_t mode = TREE_BUILD_STAGE;

	 uint32_t dump = 1;
	 uint32_t version = VER_NO_ADDR_OPND;

	 vector<uint32_t> start_pcs;
	 vector<uint32_t> end_pcs;

	 uint32_t skip = 4;
	 uint32_t no_trees = 30;

	 uint32_t anaopt = ALL_ANALYSIS;


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
		 else if (args[i]->name.compare("-start_pcs") == 0){
			 vector<string> start_pcs_string = split(args[i]->value, ',');
			 for (int j = 0; j < start_pcs_string.size(); j++){
				 start_pcs.push_back(atoi(start_pcs_string[j].c_str()));
			 }
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
		 else if (args[i]->name.compare("-end_pcs") == 0){
			 vector<string> end_pcs_string = split(args[i]->value, ',');
			 for (int j = 0; j < end_pcs_string.size(); j++){
				 end_pcs.push_back(atoi(end_pcs_string[j].c_str()));
			 }
		 }
		 else if (args[i]->name.compare("-dump") == 0){
			 dump = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-version") == 0){
			 version = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-anaopt") == 0){
			 anaopt = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-skip") == 0){
			 skip = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-no_trees") == 0){
			 no_trees = atoi(args[i]->value.c_str());
		 }

		 else if (args[i]->name.compare("-abstree_opt") == 0){
			 abstree_opt = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-conctree_opt") == 0){
			 conctree_opt = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-fraction") == 0){
			 fraction = atoi(args[i]->value.c_str());
		 }
		 else if (args[i]->name.compare("-debug_tree") == 0){
			 debug_tree = atoi(args[i]->value.c_str());
		 }
		 
		 else{
			 ASSERT_MSG(false, ("ERROR: unknown option\n"));
		 }
	 }


	 ASSERT_MSG((start_pcs.size() == end_pcs.size()), ("ERROR: start and end pcs sizes should match\n"));
	 ASSERT_MSG(!exec.empty(), ("exec must be specified\n"));
	 ASSERT_MSG((!in_image.empty()) && (!out_image.empty()), ("image must be specified\n"));

	 /********************************open the files************************************/

	 /*get all the files in output folder*/
	 string output_folder = get_standard_folder("output");
	 string filter_folder = get_standard_folder("filter");
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
			 if (is_prefix(files[i], "instrace_" + exec + "_" + in_image + "_instr")){
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

	 /* get the disasm file */
	struct _stat buf;
	int64_t max_size = -1;
	/* get the instrace files for this exec */
	for (int i = 0; i < files.size(); i++){
		if (is_prefix(files[i], "instrace_" + exec + "_" + in_image + "_asm_instr")){
			/*open the file*/
			string file = output_folder + "\\" + files[i];
			_stat(file.c_str(), &buf);
			if (max_size < buf.st_size){
				max_size = buf.st_size;
				disasm_filename = file;
			}

		}
	}
	ASSERT_MSG((!disasm_filename.empty()), ("suitable disasm file cannot be located\n"));
	disasm_file.open(disasm_filename, ifstream::in);
	ASSERT_MSG(disasm_file.good(), ("disasm file cannot be opened\n"));
	 

	/* populate the memory dump filenames */
	 for (int i = 0; i < files.size(); i++){
		 if (is_prefix(files[i], "memdump_" + exec)){
			 memdump_files.push_back(output_folder + "\\" + files[i]);
		 }
	 }

	 /* get the images */
	 in_image_filename = get_standard_folder("image") + "\\" + in_image;
	 out_image_filename = get_standard_folder("image") + "\\" + out_image;

	 /* get the app_pcs to track files */
	 app_pc_filename = filter_folder + "\\filter_" + exec + "_app_pc.log";
	 app_pc_file.open(app_pc_filename, ifstream::in);

	 /* get the image mem config files - these have common configs for a given image processing program like Photoshop (hardcoded)  */
	 config_filename = filter_folder + "\\config_" + config + ".log";
	 config_file.open(config_filename, ifstream::in);

	 /* outputs */
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
	 }

	 halide_file.open(get_standard_folder("halide") + file_substr + "_halide.cpp", ofstream::out);

	 /* debugging printfs */

	 DEBUG_PRINT(("instrace file - %s\n", instrace_filename.c_str()), 3);
	 DEBUG_PRINT(("config file - %s\n", config_filename.c_str()), 3);
	 DEBUG_PRINT(("app_pc file - %s\n", app_pc_filename.c_str()), 3);
	 DEBUG_PRINT(("in image file - %s\n", in_image_filename.c_str()), 3);
	 DEBUG_PRINT(("out image file - %s\n", out_image_filename.c_str()),3);
	 DEBUG_PRINT(("disasm file - %s\n", disasm_filename.c_str()),3);
	 DEBUG_PRINT(("memdump files - \n"),3);
	 for (int i = 0; i < memdump_files.size(); i++){
		 DEBUG_PRINT(("%s\n", memdump_files[i].c_str()), 3);
	 }

	 /************************************************************************/
	 /*						MEMORY ANALYSIS                                  */
	 /************************************************************************/
		
	 /****need tests******
		1. Extraction from the pc_mems and mem_info raw should be the same.
	 */

	 DEBUG_PRINT(("****************start mem info stage******************\n"), 2);

	 ULONG_PTR token = initialize_image_subsystem();

	 /* analyzing mem dumps for input and output image locations if dump is to be analyzed - if false,
	  * we will only get memory information from the instrace; this is the case for non-image processing 
	  * programs.
	  */
	 vector<mem_regions_t *> dump_regions;
	 if (dump){
		 dump_regions = get_image_regions_from_dump(memdump_files, in_image_filename, out_image_filename);
		 LOG(log_file, "dump regions....." << endl);
		 print_mem_regions(log_file, dump_regions);
	 }
	 


	 /* independently create the memory layout from the instrace */
	 DEBUG_PRINT(("analyzing instrace file - %s\n", instrace_filename.c_str()), 1);

	 /* problems pc mem extraction and mem region is not getting the same results? why? */

	 vector<mem_info_t *> mem_info;
	 vector<pc_mem_region_t *> pc_mem_info;

	 create_mem_layout(instrace_file, mem_info, version);
	 create_mem_layout(instrace_file, pc_mem_info, version);


	 for (int i = 0; i < mem_info.size(); i++){
		 mem_info[i]->order = i;
	 }

	 sort_mem_info(mem_info);
	 link_mem_regions_greedy_dim(mem_info, 0);
	 
	 LOG(log_file, "*********** pc_mem_info *************" << endl);
	 print_mem_layout(log_file, pc_mem_info);
	 LOG(log_file, "*********** mem_info *************" << endl);
	 print_mem_layout(log_file, mem_info);

	 
	 vector<vector<mem_info_t *> > mergable = get_merge_opportunities(mem_info, pc_mem_info);
	 merge_mem_regions_pc(mergable, mem_info);


	 

	 DEBUG_PRINT(("*******************end of mem info stage*********************\n"), 2);

	 if (mode == MEM_INFO_STAGE){
		 exit(0);
	 }


	 /******************************gathering instruction trace********************************/

	 DEBUG_PRINT(("*******************instruction gathering/preprocessing stage*********************\n"), 2);

	 /* disassembly of instructions acquired */
	 vector<Static_Info *> static_info;
	 parse_debug_disasm(static_info, disasm_file);

	 if (debug_level >= 4){
		 print_disasm(static_info);
	 }

	 print_disasm(static_info);

	 /* get all the instructions and preprocess */
	 instrace_file.clear();
	 instrace_file.seekg(0, instrace_file.beg);

	 vec_cinstr instrs_forward_unfiltered = walk_file_and_get_instructions(instrace_file, static_info, version);
	 /* need to filter unwanted instrs from the file we got */
	 vec_cinstr instrs_forward = filter_instr_trace(start_pcs, end_pcs, instrs_forward_unfiltered);


	 /* make a copy for the backwards analysis */
	 vec_cinstr instrs_backward;
	 for (int i = instrs_forward.size() - 1; i >= 0; i--){
		 cinstr_t * new_cinstr = create_new_cinstr(*instrs_forward[i].first);
		 pair<cinstr_t *, Static_Info *> instr_string = make_pair(new_cinstr, instrs_forward[i].second);
		 instrs_backward.push_back(instr_string);
	 }
	 DEBUG_PRINT(("number of dynamic instructions : %d\n", instrs_backward.size()), 2);

	 /*preprocessing*/
	 DEBUG_PRINT(("for both forward and backward instr traces\n"), 2);
	 update_regs_to_mem_range(instrs_backward);
	 update_regs_to_mem_range(instrs_forward);

	 update_floating_point_regs(instrs_backward, BACKWARD_ANALYSIS, static_info, start_pcs);
	 update_floating_point_regs(instrs_forward, FORWARD_ANALYSIS, static_info, start_pcs);

	 DEBUG_PRINT(("*******************end of instruction gathering/preprocessing stage*********************\n"), 2);

	 /*******************************************more memory and input/output selection********************************************************/
	 

	 DEBUG_PRINT(("******************memory input and output selection********************************\n"), 2);

	 vector<uint32_t> candidate_ins;
	 vector<uint32_t> start_points_mem;
	 start_points_mem = get_instrace_startpoints(instrs_forward, start_pcs);

	 mem_regions_t * input_mem_region = NULL;
	 mem_regions_t * output_mem_region = NULL;

	 remove_possible_stack_frames(pc_mem_info, mem_info, static_info, instrs_forward);
	 
	 /* merge these two information - instrace mem info + mem dump info */

	 vector<mem_regions_t *> total_mem_regions;
	 vector<mem_regions_t *> image_regions = merge_instrace_and_dump_regions(total_mem_regions, mem_info, dump_regions);

	 /* get possible buffers */
	 mark_possible_buffers(pc_mem_info, total_mem_regions, static_info, instrs_forward);
	 
	 vector<mem_regions_t*> regions = get_input_output_regions(image_regions, total_mem_regions, pc_mem_info, candidate_ins, instrs_forward, start_points_mem);
	 
	 for (int i = 0; i < total_mem_regions.size(); i++){
		 total_mem_regions[i]->dependant = false;
	 }

	 input_mem_region = regions[0];
	 output_mem_region = regions[1];

	 vector<mem_regions_t *> input_regions;
	 if ( (anaopt & INPUT_REGION_SELECTION) == INPUT_REGION_SELECTION){
		 input_regions = get_input_regions(total_mem_regions, pc_mem_info, start_points_mem, instrs_forward);
		 log_file << " input regions " << endl;
		 print_mem_regions(log_file, input_regions);
		 log_file << "input done " << endl;
	 }
	 else{
		 input_regions.push_back(input_mem_region);
	 }
	 
	 image_regions.clear();
	 image_regions.push_back(output_mem_region); /* we do not need to check whether this region is there; can be a duplicate, this is for random tree building */

	 DEBUG_PRINT(("-------input -----\n"),2);
	 print_mem_regions(cout,input_mem_region);
	 DEBUG_PRINT(("-------output -----\n"), 2);
	 print_mem_regions(cout,output_mem_region);

	 LOG(log_file, "image regions ----->" << endl);
	 print_mem_regions(log_file,image_regions);
	 LOG(log_file, "total memory regions ----->" << endl);
	 print_mem_regions(log_file,total_mem_regions);

	 DEBUG_PRINT(("******************memory input and output selection done********************************\n"), 2);

	 /**************************** forward analysis for conditionals, indirection *************************************************************/
	 
	 vector<uint32_t> app_pc;
	 vector<uint32_t> app_pc_indirect;
	 vector<uint32_t> app_pc_total;
	 vector<vector<uint32_t> > app_pc_vec;
	 vector<Jump_Info * > cond_app_pc;


	DEBUG_PRINT(("before filter static ins : %d\n", static_info.size()), 2);
	filter_disasm_vector(instrs_forward, static_info);
	DEBUG_PRINT(("after filter static ins : %d\n", static_info.size()), 2);

	/*vector<mem_regions_t *> inputs;
	for (int i = 0; i < total_mem_regions.size(); i++){
		if ((total_mem_regions[i]->direction & MEM_INPUT) == MEM_INPUT){
			inputs.push_back(total_mem_regions[i]);
		}
	}
	app_pc = find_dependant_statements(instrs_forward, input_mem_region, static_info);
	*/
	
	if ((anaopt & DEPENDANT_ANALYSIS) == DEPENDANT_ANALYSIS){
		log_file << " dependant analysis " << endl;
		app_pc_vec = find_dependant_statements_with_indirection(instrs_forward, input_regions, static_info, start_points_mem);
	}
	else{
		app_pc_vec.push_back(app_pc);
		app_pc_vec.push_back(app_pc);
	}

	app_pc = app_pc_vec[0];
	app_pc_total = app_pc_vec[1];
	app_pc_indirect.resize(app_pc_vec[1].size());
	vector<uint32_t>::iterator it;

	sort(app_pc_vec[0].begin(), app_pc_vec[0].end());
	sort(app_pc_vec[1].begin(), app_pc_vec[1].end());

	it = set_difference(app_pc_vec[1].begin(), app_pc_vec[1].end(), app_pc_vec[0].begin(), app_pc_vec[0].end(), app_pc_indirect.begin());
	app_pc_indirect.resize(it - app_pc_indirect.begin());

	LOG(log_file, "dependant analysis-------->" << endl);
	LOG(log_file,"dependant statements" << endl);
	if (debug_level >= 2){
		for (int i = 0; i < app_pc.size(); i++){
			string disasm_string = get_disasm_string(static_info, app_pc[i]);
			LOG(log_file,app_pc[i] << " " << disasm_string << endl);
		}
	}

	LOG(log_file,"indirect dependant statements" << endl);
	for (int i = 0; i < app_pc_indirect.size(); i++){
		string disasm_string = get_disasm_string(static_info, app_pc_indirect[i]);
		LOG(log_file,app_pc_indirect[i] << " " << disasm_string << endl);
	}


	
	if ((anaopt & CONDITIONAL_ANALYSIS) == CONDITIONAL_ANALYSIS){
		cond_app_pc = find_dependant_conditionals(app_pc_total, instrs_forward, static_info);
	}

	LOG(log_file, "dependant conditionals" << endl);
	for (int i = 0; i < cond_app_pc.size(); i++){
		string dis_jmp_string = get_disasm_string(static_info, cond_app_pc[i]->jump_pc);
		string dis_cond_string = get_disasm_string(static_info, cond_app_pc[i]->cond_pc);

		LOG(log_file, i + 1 << " - conditional " << endl);
		LOG(log_file, "jump ");
		LOG(log_file, cond_app_pc[i]->jump_pc << " " << dis_jmp_string << endl);
		LOG(log_file, "cond_pc ");
		LOG(log_file, cond_app_pc[i]->cond_pc << " " << dis_cond_string << endl);
		LOG(log_file, "target_pc ");
		LOG(log_file, cond_app_pc[i]->target_pc << endl);
		LOG(log_file, "fall_pc ");
		LOG(log_file, cond_app_pc[i]->fall_pc << endl);
		LOG(log_file, "merge_pc ");
		LOG(log_file, cond_app_pc[i]->merge_pc << endl);
	}
	 
	 populate_conditional_instructions(static_info, cond_app_pc);
	 populate_dependant_indireciton(static_info, app_pc_indirect);

	 LOG(log_file,"populated cond. instrs" << endl);
	 for (int i = 0; i < static_info.size(); i++){
		 Static_Info * info = static_info[i];
		 if (info->conditionals.size() > 0){
			  LOG(log_file, "pc : " << info->pc << " " << info->disassembly << endl);
			 //conditionals
			 for (int j = 0; j < info->conditionals.size(); j++){
				 LOG(log_file, "cond " << info->conditionals[j].first->cond_pc << "jump " << info->conditionals[j].first->jump_pc  << " taken : " << info->conditionals[j].second << endl);
			 }
		 }
	 }

	 if (mode == CONDITIONAL_STAGE){
		 exit(0);
	 }



	 /********************************* tree construction *******************************************************************/

	 vector<Conc_Tree *> conc_trees;
	 vector< vector< Conc_Tree *> > clustered_trees;

	 /* capture the function start points if the end trace is not given specifically */
	 vector<uint32_t> start_points;
	 vector<uint32_t> start_points_forward;
	 if (end_trace == FILE_ENDING){
		 start_points = get_instrace_startpoints(instrs_backward, start_pcs);
		 start_points_forward = get_instrace_startpoints(instrs_forward, start_pcs);
		 DEBUG_PRINT(("no of funcs captured - %d\n start points : \n", start_points.size()), 1);
		 for (int i = 0; i < start_points.size(); i++){
			 DEBUG_PRINT(("%d-", start_points[i]), 1);
		 }
		 DEBUG_PRINT(("\n"), 1);
	 }


	 if (tree_build == BUILD_RANDOM){

		 uint64_t farthest = get_farthest_mem_access_point(total_mem_regions);

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
		 Conc_Tree * tree = new Conc_Tree();
		 Conc_Tree * initial = build_conc_tree(dest, stride, start_points, start_trace, end_trace, tree, instrs_backward, farthest, total_mem_regions);
		 tree->print_conditionals();
		 cout << "creating conditional trees" << endl;
		 build_conc_trees_for_conditionals(start_points, tree, instrs_backward, farthest, total_mem_regions);
		 for (int i = 0; i < tree->conditionals.size(); i++){
			 conc_trees.push_back(tree->conditionals[i]->tree);
		 }
		 conc_trees.push_back(tree);
		 if (initial != NULL){
			 build_conc_trees_for_conditionals(start_points, initial, instrs_backward, farthest, total_mem_regions);
			 conc_trees.push_back(initial);
		 }

	 }
	 else if (tree_build == BUILD_RANDOM_SET){

		 uint64_t farthest = get_farthest_mem_access_point(total_mem_regions);
		 /*ok we need find a set of random locations */
		 vector<uint64_t> nbd_locations = get_nbd_of_random_points(image_regions, seed, &stride);

		 /* ok now build trees for the set of locations */
		 for (int i = 0; i < nbd_locations.size(); i++){

			 Conc_Tree * tree = new Conc_Tree();
			 Conc_Tree * initial = build_conc_tree(nbd_locations[i], stride, start_points, FILE_BEGINNING, end_trace, tree, instrs_backward, farthest, total_mem_regions);
			 build_conc_trees_for_conditionals(start_points, tree, instrs_backward,farthest, total_mem_regions);

			 conc_trees.push_back(tree);
			 if (initial != NULL){
				 build_conc_trees_for_conditionals(start_points, initial, instrs_backward, farthest, total_mem_regions);
				 conc_trees.push_back(initial);
			 }


		 }

		 /* checking similarity of the trees and if not repeat?? */
		 vector<vector<Conc_Tree *> > clustered_conc_trees = categorize_trees(conc_trees);
		 ASSERT_MSG((clustered_conc_trees.size() == 1), ("ERROR: The trees should be similar\n"));

	 }
	 else if (tree_build == BUILD_SIMILAR){
		 uint64_t farthest = get_farthest_mem_access_point(total_mem_regions);
		 conc_trees = get_similar_trees(image_regions, total_mem_regions, seed, &stride, start_points, start_trace, end_trace, farthest, instrs_backward);
	 }
	 else if (tree_build == BUILD_CLUSTERS){
		 uint64_t farthest = get_farthest_mem_access_point(total_mem_regions);
		 clustered_trees = cluster_trees(image_regions, total_mem_regions, start_points, instrs_backward, farthest, output_folder + file_substr);
	 }


	 /* number the trees - for all the conc trees built */
	 if (tree_build == BUILD_CLUSTERS){
		 for (int i = 0; i < clustered_trees.size(); i++){
			 for (int j = 0; j < clustered_trees[i].size(); j++){
				 clustered_trees[i][j]->number_tree_nodes();
			 }
		 }
		 cout << "numbering done" << endl;

	 }
	 else{
		 for (int i = 0; i < conc_trees.size(); i++){
			 conc_trees[i]->number_tree_nodes();
			 cout << "number of nodes : " << conc_trees[i]->num_nodes << endl;
		 }
	 }



	 /* debug printing - need to change the branches */
	 for (int i = 0; i < conc_trees.size(); i++){

		 /* Expression printing */
		 DEBUG_PRINT(("printing out the expression\n"), 2);
		 ofstream expression_file(output_folder + file_substr + "_expression_" + to_string(i) + ".txt", ofstream::out);
		 
		 DEBUG_PRINT(("printing to dot file...\n"), 2);
		 ofstream conc_file(output_folder + file_substr + "_conctree_" + to_string(i) + ".dot", ofstream::out);
		 conc_trees[i]->print_dot(conc_file,"conc",i);
		 
	 }

	 if (clustered_trees.size() > 0){
		 for (int i = 0; i < clustered_trees.size(); i++){
			 cout << i << " size: " << clustered_trees[i].size() << endl;
			 DEBUG_PRINT(("printing to dot file...\n"), 2);
			 ofstream conc_file(output_folder + file_substr + "_conctree_" + to_string(i) + ".dot", ofstream::out);
			 clustered_trees[i][0]->print_dot(conc_file, "conc", i);
			 cout << "conditionals: " << clustered_trees[i][0]->conditionals.size() << endl;
		 }
	 }

	 if (mode == TREE_BUILD_STAGE){
		 exit(0);
	 }


	 /***************************************ABSTRACTION*********************************************************/

	 vector<Abs_Tree_Charac *> abs_trees;
	 Abs_Tree * final_abs_tree;


	 if (clustered_trees.size() > 0){
		 abs_trees = build_abs_trees(clustered_trees, output_folder, 4, total_mem_regions, 30, pc_mem_info);
		 cout << "building abstract trees done" << endl;
	 }


	 if (conc_trees.size() > 0){
		 vector<Abs_Tree *> abs_trees;
		 for (int i = 0; i < conc_trees.size(); i++){
			 Abs_Tree * abs_tree = new Abs_Tree();
			 abs_tree->build_abs_tree_unrolled(conc_trees[i], total_mem_regions);
			 abs_tree->number_tree_nodes();
			 abs_trees.push_back(abs_tree);
		 }


		 /* debug printing */
		 for (int i = 0; i < abs_trees.size(); i++){
			 ofstream abs_file(output_folder + file_substr + "_abstree_" + to_string(i) + ".dot", ofstream::out);
			 abs_trees[i]->print_dot(abs_file,"abs",i);
		 }

		 Comp_Abs_Tree * comp_tree = new Comp_Abs_Tree();
		 comp_tree->build_compound_tree_unrolled(abs_trees);
		 comp_tree->number_tree_nodes();
		 ofstream comp_file(output_folder + file_substr + "_comp_tree.dot", ofstream::out);
		 comp_tree->print_dot(comp_file,"comp",0);
		 comp_tree->abstract_buffer_indexes();
		 final_abs_tree = comp_tree->compound_to_abs_tree();

		 ofstream alg_file(output_folder + file_substr + "_algebraic.dot", ofstream::out);
		 uint32_t max_dimensions = final_abs_tree->get_maximum_dimensions();
		 final_abs_tree->print_dot_algebraic(alg_file, "alg", 0, get_vars("x",max_dimensions));

	 }

	 if (mode == ABSTRACTION_STAGE){
		 exit(0);
	 }


	 /**************************************HALIDE OUTPUT + ALGEBRIC FILTERS********************************************************/

	 Halide_Program * halide = new Halide_Program(); 

	 if (abs_trees.size() == 0){
		 halide->populate_pure_funcs(final_abs_tree);
	 }
	 else{
		 for (int i = 0; i < abs_trees.size(); i++){
			 if (abs_trees[i]->is_recursive){
				 cout << "red func populated" << endl;
				 halide->populate_red_funcs(abs_trees[i]->tree, abs_trees[i]->extents, abs_trees[i]->red_node);
			 }
			 else{
				 cout << "pure func populated" << endl;
				 halide->populate_pure_funcs(abs_trees[i]->tree); 
			 }
		 }
	 }

	 halide->populate_vars(4);
	 halide->populate_input_params();
	 halide->populate_params();

	 vector<string> red_variables;
	 halide->print_halide_program(halide_file, red_variables);
	 halide->get_memory_regions(memdump_files);

	 shutdown_image_subsystem(token);
	 return 0;
 }


 





 











 

 



 