#include <Windows.h>
#include "image_manipulation.h"
#include "meminfo.h"
#include "moduleinfo.h"
#include "utilities.h"
#include "defines.h"
#include "filter_logic.h"

using namespace std;

#define DIFF_MODE		1
#define TWO_IMAGE_MODE  2
#define ONE_IMAGE_MODE	3

int main(int argc, char **argv){

	ULONG_PTR token = initialize_image_subsystem();

	string input_folder = "..\\..\\..\\out_files";

	vector<string> files = get_all_files_in_folder(input_folder);

	string mode(argv[1]);
	string image_1(argv[2]);
	string image_2;
	if (atoi(argv[1]) == TWO_IMAGE_MODE){
		image_2.assign(argv[3]);
	}

	DEBUG_PRINT("reading bbinfo\n");
	int index = -1;
	for (int i = 0; i < files.size(); i++){
		if (is_prefix(files[i], "profile")){
			index = i;
			break;
		}
	}

	if (index == -1){
		printf("ERROR: profile file not found\n");
		exit(1);
	}

	//create the profile filename
	string profile_file = input_folder + "\\" + files[index];
	string output_file = input_folder + "\\read_output.txt";
	string output_func_file = input_folder + "\\read_funcs.txt";

	moduleinfo_t * module = populate_moduleinfo(profile_file.c_str());
	image_t * image = populate_imageinfo(image_1.c_str());

	filter_based_on_freq(module, image, 90);
	filter_based_on_composition(module);

	print_moduleinfo(module, output_file.c_str());
	print_funcs(module, output_func_file.c_str());

	

	shutdown_image_subsystem(token);



	return 0;

}











