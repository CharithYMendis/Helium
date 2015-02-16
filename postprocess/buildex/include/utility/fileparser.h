#ifndef _FILE_PARSER_H
#define _FILE_PARSER_H

#include <iostream>
#include <utility>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <string>

#include "analysis/staticinfo.h"
#include "analysis/x86_analysis.h"

/* parse the extracted files */
cinstr_t * get_next_from_ascii_file(std::ifstream &file);
cinstr_t * get_next_from_bin_file(std::ifstream &file);
void parse_debug_disasm(std::vector<Static_Info *> &info, std::ifstream &file);
vector<cinstr_t * > get_all_instructions(std::ifstream &file);
vec_cinstr walk_file_and_get_instructions(std::ifstream &file, 
										  std::vector<Static_Info *> &static_info);
void reverse_file(std::ofstream &out, std::ifstream &in);

/* debug routines */
void walk_instructions(std::ifstream &file);
void print_disasm(std::vector<Static_Info *> &static_info);
string get_disasm_string(std::vector<Static_Info *> &static_info, uint32_t app_pc);

#endif