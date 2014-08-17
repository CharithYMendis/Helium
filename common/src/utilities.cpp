#include <Windows.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include "common_defines.h"
#include "utilities.h"


using namespace std;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);


/* debug and helper functions */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

std::vector<cmd_args_t *> get_command_line_args(int argc, char ** argv){

	std::vector<cmd_args_t *> args;

	for (int i = 1; i < argc; i += 2){

		std::string name(argv[i]);
		std::string value(argv[i+1]);
	
		assert(name[0] == '-'); /* this ensures that first command line arg is actually a name*/

		cmd_args_t * arg = new cmd_args_t;
		arg->name = name;
		arg->value = value;
		args.push_back(arg);

	}

	return args;


}

vector<string> get_all_files_in_folder(string folder)
{
	vector<string> names;
	char search_path[200];
	sprintf(search_path, "%s\\*.*", folder.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// read all (real) files in current folder, delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}


	return names;
}

string get_standard_folder(string type){


	char * folder = NULL;
	if (type.compare("filter") == 0){
		folder = getenv(FILTER_FOLDER_ENV_VAR);
	}
	else if (type.compare("parent") == 0){
		folder = getenv(PARENT_FOLDER_ENV_VAR);
	}
	else if (type.compare("output") == 0){
		folder = getenv(OUTPUT_FOLDER_ENV_VAR);
	}
	else if (type.compare("log") == 0){
		folder = getenv(LOG_FOLDER_ENV_VAR);
	}
	else if (type.compare("halide") == 0){
		folder = getenv(HALIDE_FOLDER_ENV_VAR);
	}
	else if (type.compare("image") == 0){
		folder = getenv(IMAGE_FOLDER_ENV_VAR);
	}

	assert(folder != NULL);
	string folder_s(folder);

	
	return folder_s;

}


bool is_prefix(string str, string prefix){
	for (int i = 0; i < prefix.size(); i++){
		if (i >= str.size()) return false;
		if (str[i] != prefix[i]) return false;
	}
	return true;
}


void print_progress(uint32_t * count, uint32_t mod){

	if (debug){
		(*count)++;
		if ( (*count % mod)  == 0){
			printf("\t progress - %d\n", *count);
		}
	}


}