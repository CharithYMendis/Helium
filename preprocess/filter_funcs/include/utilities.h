#ifndef _UTILITIES_H
#define _UTILITIES_H

#include <iostream>
#include <string>
#include <vector>

#define MAX_PATH_LENGTH		500

std::vector<std::string> get_all_files_in_folder(std::string folder);
bool is_prefix(std::string str, std::string prefix);


#endif