#ifndef _PRINT_DOT_H
#define _PRINT_DOT_H

#include <fstream>
#include "node.h"
#include "defines.h"
#include "print.h"
#include "print_dot.h"
#include <string>


void print_to_dotfile(ofstream &file, Node * head, uint no_of_nodes);

#endif