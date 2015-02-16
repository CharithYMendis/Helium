 //main application - read a instrace in a file and then build an expression tree
 //you need to choose where to start and end the trace and other criteria in this file's implementation.

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

#include "utilities.h"
#include "meminfo.h"
#include "imageinfo.h"

 using namespace std;

 bool debug = false;
 uint32_t debug_level = 0;
 ofstream log_file;
 
 void print_usage(){
	 printf("usage - format -<name> <value>\n");
	 printf("\t exec - the executable which DR analyzed (without exe) \n");
	 printf("\t thread_id - the thread id that the instrace file belongs to\n");
	 printf("\t start_line - tracing start line\n");
	 printf("\t dest - the destination to be tracked\n");
	 printf("\t end_line - tracing end line\n");
	 printf("\t in_image - the in_image filename with ext \n");
	 printf("\t out_image - the out_image filename with ext \n");
	 printf("\t config - the middle name of the config file config_<name>.log\n");
	 printf("\t debug - 1,0 which turns debug mode on/off \n");
	 printf("\t debug_level - the level of debugging (higher means more debug info) \n");
	 printf("\t seed - the seed to select the random memory point\n");
	 printf("\t stride - the memory width at the particular location\n");
	 printf("\t tree_build - the tree building method \"random - 1\",\"random set - 2\",\"all - 3\"\n");
	 printf("\t mode - the mode in which this tool is running mem_info_stage, tree_build_stage, abstraction_stage, halide_output_stage\n");
 }

 /* tree build modes */
#define BUILD_RANDOM		1
#define BUILD_RANDOM_SET	2
#define	BUILD_SIMILAR		3
#define BUILD_CLUSTERS		4


 /* stage to stop */
#define MEM_INFO_STAGE			1	
#define TREE_BUILD_STAGE		2
#define ABSTRACTION_STAGE		3
#define HALIDE_OUTPUT_STAGE		4

 int main(int argc, char ** argv){

	 return 0;
 }


 





 











 

 



 