#include <Windows.h>
#include "imageinfo.h"
#include "meminfo.h"
#include "moduleinfo.h"
#include "utilities.h"
#include "filter_logic.h"
#include "common_defines.h"
#include "memlayout.h"

using namespace std;

#define DIFF_MODE		1
#define TWO_IMAGE_MODE  2
#define ONE_IMAGE_MODE	3

bool debug = false;
uint32_t debug_level = 0;
ofstream log_file;


void print_usage(){
	printf("usage - format -<name> <value>\n");
	printf("\t exec - the executable which DR analyzed (without exe) \n");
	printf("\t in_image - the in_image filename with ext \n");
	printf("\t out_image - the out_image filename with ext \n");
	printf("\t debug - 1,0 which turns debug mode on/off \n");
	printf("\t debug_level - the level of debugging (higher means more debug info) \n");
	printf("\t mode - mode of filtering \n");
}

int main(int argc, char **argv){

	string process_name(argv[0]);
	int32_t mode = -1;
	vector<string> in_images;
	vector<string> out_images;
	string exec;

	/*********************cmd line arg processing*************************/

	vector<cmd_args_t *> args = get_command_line_args(argc, argv);
	if (args.size() == 0){
		print_usage();
		exit(0);
	}

	for (int i = 0; i < args.size(); i++){
		if (args[i]->name.compare("-exec") == 0){
			exec = args[i]->value;
		}
		else if (args[i]->name.compare("-in_image") == 0){
			in_images.push_back(args[i]->value);
		}
		else if (args[i]->name.compare("-out_image") == 0){
			out_images.push_back(args[i]->value);
		}
		else if (args[i]->name.compare("-debug") == 0){
			debug = args[i]->value[0] - '0';
		}
		else if (args[i]->name.compare("-debug_level") == 0){
			debug_level = atoi(args[i]->value.c_str());
		}
		else if (args[i]->name.compare("-mode") == 0){
			mode = atoi(args[i]->value.c_str());
		}
		else{
			printf("unrecognized argument - %s\n", args[i]->name);
			exit(0);
		}
	}

	/*****************setting up files**************************/

	/*inputs*/
	vector<ifstream *> profile_files;
	vector<vector<ifstream *> > memtrace_files;
	vector<string> in_image_filenames;
	vector<string> out_image_filenames;

	/*get the files*/
	string output_folder = get_standard_folder("output");
	string image_folder = get_standard_folder("image");

	DEBUG_PRINT(("output folder - %s\n",output_folder.c_str()), 5);
	DEBUG_PRINT(("image folder - %s\n", image_folder.c_str()), 5);
	

	vector<string> files = get_all_files_in_folder(output_folder);

	for (int i = 0; i < in_images.size(); i++){
		string profile_string("profile_" + exec + ".exe_" + in_images[i]);
		string memtrace_string("memtrace_" + exec + ".exe_" + in_images[i]);

		DEBUG_PRINT(("profile string - %s\n", profile_string.c_str()), 5);
		DEBUG_PRINT(("memtrace string - %s\n", memtrace_string.c_str()), 5);

		vector<ifstream *> memtrace_per_image;
		for (int j = 0; j < files.size(); j++){
			if (is_prefix(files[j], profile_string)){
				DEBUG_PRINT(("profile - %s\n", (output_folder + "\\" + files[j]).c_str()), 5);
				ifstream * file = new ifstream(output_folder + "\\" + files[j], ifstream::in);
				profile_files.push_back(file);
			}
			else if (is_prefix(files[j], memtrace_string)){
				DEBUG_PRINT(("memtrace - %s\n", (output_folder + "\\" + files[j]).c_str()), 5);
				ifstream * file = new ifstream(output_folder + "\\" + files[j], ifstream::in);
				memtrace_per_image.push_back(file);
			}
		}
		memtrace_files.push_back(memtrace_per_image);
	}
	

	for (int i = 0; i < in_images.size(); i++){
		in_image_filenames.push_back(image_folder + "\\" + in_images[i]);
		out_image_filenames.push_back(image_folder + "\\" + out_images[i]);
	}

	/*outputs*/

	cout << process_name << endl;

	/*check whether process name has .exe or not*/
	size_t find = process_name.find(".exe");
	if (find != string::npos){
		process_name = process_name.substr(0, find);
	}

	ofstream filter_file(get_standard_folder("filter") + "\\" + process_name + "_" +  exec + ".exe.log", ofstream::out); 
	ofstream app_pc_file(get_standard_folder("output") + "\\" + process_name + "_" + exec + ".exe_app_pc.log", ofstream::out);

	if (debug){
		log_file.open(get_standard_folder("log") + "\\" + process_name + "_" + exec + ".exe.log", ofstream::out);
	}
	
	/****************************main algorithm***************************/

	ULONG_PTR token = initialize_image_subsystem();

	if (mode == ONE_IMAGE_MODE || mode == DIFF_MODE){

		//get the module information from the profile
		DEBUG_PRINT(("populating module information....\n"), 5);
		moduleinfo_t * module = populate_moduleinfo(*profile_files[0]);
		DEBUG_PRINT( ("modules populated with profile information  \n") ,5);

		//get the image information
		Gdiplus::Bitmap * in_image_bitmap = open_image(in_image_filenames[0].c_str());
		Gdiplus::Bitmap * out_image_bitmap = open_image(out_image_filenames[0].c_str());
		image_t * in_image = populate_imageinfo(in_image_bitmap);
		image_t * out_image = populate_imageinfo(out_image_bitmap);

		//get the memory region information -> link them together -> filter them
		DEBUG_PRINT(("getting memory region information... \n"), 5);
		vector<pc_mem_region_t *> pc_mems = get_mem_regions_from_memtrace(memtrace_files[0], module);
		DEBUG_PRINT(("linking memory regions together... \n"), 5);
		/* this should return the linking information */
		link_mem_regions(pc_mems,GREEDY);
		DEBUG_PRINT(("filtering out insignificant regions... \n"), 5);
		filter_mem_regions(pc_mems, in_image, out_image, 30);

		//get the function entry points filled up for those PCs
		populate_function_entry_points(pc_mems, module);

		//get the function composition

		//if one image -> filter based on functional composition (to get rid of loading and saving code)
		
		//then get the function with the maximum number of pc_mems
		
		print_mem_layout(log_file, pc_mems);


		DEBUG_PRINT(("filtering based on bb freq on moduleinfo...\n"), 5);
		filter_bbs_based_on_freq(module, in_image, 1);
		print_filter_file(filter_file, module);
		

		vector<mem_info_t *> mems = extract_mem_regions(pc_mems);
		log_file << "********************extracted mems************************" << endl;
		
		print_mem_layout(log_file, mems);

		exit(0);


		
		

		if (mode == ONE_IMAGE_MODE){
			filter_based_on_memory_dependancy(pc_mems, module);
		}

		vector<func_composition_t *> comp = create_func_composition(pc_mems, module);


		print_filter_file(filter_file, module);
		print_app_pc_info(app_pc_file, comp);
		

		if (debug){
			print_moduleinfo(module, log_file);
			print_funcs(module, log_file);
		}

	
	}
	else if(mode == TWO_IMAGE_MODE){

	}
	
	shutdown_image_subsystem(token);

	/* delete the objects and close the files */


	return 0;

}











