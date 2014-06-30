#ifndef _FILE_PARSER_H
#define _FILE_PARSER_H

#include <iostream>
#include <fstream>
#include "canonicalize.h"
#include <vector>

using namespace std;

cinstr_t * get_next_from_ascii_file(ifstream &file);
cinstr_t * get_next_from_bin_file(ifstream &file);

vector<cinstr_t * > get_all_instructions(ifstream &file);
void walk_instructions(ifstream &file);

void go_forward_line(ifstream &file);
bool go_backward_line(ifstream &file);
void reverse_file(ofstream &out, ifstream &in);
void go_to_line(uint line_no, ifstream &file);

void print_cinstr(cinstr_t * instr);

#endif