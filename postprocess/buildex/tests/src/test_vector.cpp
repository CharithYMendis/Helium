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

#include  "..\..\..\dr_clients\include\output.h"
#include "utility\fileparser.h"
#include "utility\defines.h"

#include "analysis\staticinfo.h"
#include "analysis\preprocess.h"
#include "analysis\tree_analysis.h"


#include "utilities.h"
#include "meminfo.h"
#include "imageinfo.h"
#include "common_defines.h"


using namespace std;


bool debug = false;
uint32_t debug_level = 0;
ofstream log_file;


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
}


int main(int argc, char ** argv){


	/* setting up the files and other inputs and outputs for the program */

	/* file and process names */
	string process_name(argv[0]);
	string exec;
	string in_image;
	string out_image;

	int32_t start_trace = FILE_BEGINNING;
	int32_t end_trace = FILE_ENDING;
	uint64_t dest = 0;
	uint32_t stride = 0;

	uint32_t start_pc = 0; /* start pc of the function */
	uint32_t end_pc = 0; /* end pc of the function */
	uint32_t seed = 10;

	uint32_t version = VER_NO_ADDR_OPND;


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
		else if (args[i]->name.compare("-end_pc") == 0){
			end_pc = atoi(args[i]->value.c_str());
		}
		else if (args[i]->name.compare("-version") == 0){
			version = atoi(args[i]->value.c_str());
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
	string filter_folder = get_standard_folder("filter");
	vector<string> files = get_all_files_in_folder(output_folder);

	/* inputs */
	ifstream		  instrace_file;
	ifstream		  disasm_file;

	string			  disasm_filename;
	string			  instrace_filename;



	/* get the instrace file with the largest size */
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
	
	ASSERT_MSG(instrace_file.good(), ("instrace file cannot be opened\n"));

	/* get the disasm file */
	max_size = -1;
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

	/************************************************************************/
	/*    preprocessing instruction trace                                   */
	/************************************************************************/

	/* disassembly of instructions acquired */
	DEBUG_PRINT(("getting disassembly trace\n"), 1);
	vector<Static_Info *> static_info;
	parse_debug_disasm(static_info, disasm_file);

	if (debug_level >= 4){
		print_disasm(static_info);
	}

	/* get all the instructions and preprocess */
	instrace_file.clear();
	instrace_file.seekg(0, instrace_file.beg);

	vec_cinstr instrs_forward_unfiltered = walk_file_and_get_instructions(instrace_file, static_info, version);
	/* need to filter unwanted instrs from the file we got */
	vec_cinstr instrs_forward = filter_instr_trace(start_pc, end_pc, instrs_forward_unfiltered);

	/* make a copy for the backwards analysis */
	vec_cinstr instrs_backward;
	for (int i = instrs_forward.size() - 1; i >= 0; i--){
		cinstr_t * new_cinstr = new cinstr_t(*instrs_forward[i].first);
		pair<cinstr_t *, Static_Info *> instr_string = make_pair(new_cinstr, instrs_forward[i].second);
		instrs_backward.push_back(instr_string);
	}

	DEBUG_PRINT(("number of dynamic instructions : %d\n", instrs_backward.size()), 2);

	/*preprocessing*/
	update_regs_to_mem_range(instrs_backward);
	update_regs_to_mem_range(instrs_forward);

	vector<uint32_t> start_pcs;
	vector<uint32_t> end_pcs;
	start_pcs.push_back(start_pc);

	update_floating_point_regs(instrs_backward, BACKWARD_ANALYSIS, static_info, start_pcs);
	DEBUG_PRINT(("updated backward instr's floating point regs\n"), 2);
	update_floating_point_regs(instrs_forward, FORWARD_ANALYSIS, static_info, start_pcs);
	DEBUG_PRINT(("updated forward instr's floating point regs\n"), 2);


	/************************************************************************/
	/*        reducing instructions				                            */
	/************************************************************************/


	for (int i = 0; i < instrs_forward.size(); i++){
		int amount = 0;
		rinstr_t *  rinstr = cinstr_to_rinstrs(instrs_forward[i].first, amount, instrs_forward[i].second->disassembly, i);
		delete rinstr;
	}


	return 0;

}