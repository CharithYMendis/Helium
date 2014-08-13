#ifndef _EXALGO_UTILITIES_H
#define _EXALGO_UTILITIES_H

#include <vector>

#define HALIDE_FOLDER_ENV_VAR	"EXALGO_HALIDE_FOLDER"
#define OUTPUT_FOLDER_ENV_VAR	"EXALGO_OUTPUT_FOLDER"
#define FILTER_FOLDER_ENV_VAR	"EXALGO_FILTER_FOLDER"
#define LOG_FOLDER_ENV_VAR		"EXALGO_LOG_FOLDER"
#define PARENT_FOLDER_ENV_VAR	"EXALGO_PARENT_FOLDER"
#define IMAGE_FOLDER_ENV_VAR	"EXALGO_IMAGE_FOLDER"


struct cmd_args_t{

	std::string name;
	std::string value;

};

std::vector<std::string> split(const std::string &s, char delim);
std::vector<cmd_args_t *> get_command_line_args(int argc, char ** argv);
std::vector<std::string> get_all_files_in_folder(std::string folder);
std::string get_standard_folder(std::string type);
bool is_prefix(std::string str, std::string prefix);


#endif