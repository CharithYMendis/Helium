#ifndef _PRINT_HELPER_H
#define _PRINT_HELPER_H

#include <stdint.h>
#include <string>
#include <vector>

/* obtaining the string form of x86 objects */
std::string regname_to_string(uint32_t reg);
std::string operation_to_string(uint32_t operation);
std::string opnd_to_string(operand_t * opnd);
std::string dr_operation_to_string(uint32_t operation);

/* printing dot files helper functions */
std::string dot_get_edge_string(uint32_t from, uint32_t to);
std::string dot_get_node_string(uint32_t node_num, std::string node_string);


#endif