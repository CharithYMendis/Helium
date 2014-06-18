 //main application - read a instrace in a file and then build an expression tree
 //you need to choose where to start and end the trace and other criteria in this file's implementation.
 #include <stdio.h>
 #include <iostream>
 #include <vector>
 #include <string.h>
 #include <sstream>
 #include "exbuild.h"
 #include "node.h"
 #include "canonicalize.h"
 #include "assert.h"
 #include "fileparser.h"
 #include <stdlib.h>
 #include <fstream>

#define MAX_STRING_LENGTH 200
 
 using namespace std;

 int main(int argc, char ** argv){
	
	//remember backwards traversal
	//arguments - file name, file_type, {start line (instr) and end line (instr)} / {heap mem addr, its location in the instrace}
	printf("args - %d\n",argc);
	
	for(int i=0;i <argc; i++){
		printf("%s\n",argv[i]);
	}
	
	assert(argc == 5);
	
	//the 5 arguments are as follows.
	//1. name of the program
	//2. in file
	//3. out file
	//4. line to start tracking -> gives fexibility to start from a mid point in the execution trace
	//5. destination to start tracking (the value field)
	//end of trace tracking -> not yet supported; also can track based on app_pc instead of line no

	ifstream in;
	in.open(argv[1],std::ifstream::in);
	ofstream out;
	out.open(argv[2],std::ofstream::out);
	unsigned int lineno = atoi(argv[3]);
	unsigned int dest_to_track = atoi(argv[4]);

	cinstr_t * instr;
	int no_rinstrs;
	rinstr_t * rinstr;
	Expression_tree * tree = new Expression_tree();


	//we need to scroll to the place from where we need to build the instrace
	int curpos = 0;
	char dummystr[MAX_STRING_LENGTH];

	while(curpos < lineno - 1){
		in.getline(dummystr,MAX_STRING_LENGTH);
		curpos++;
	}

	//now we need to read the next line and start from the correct destination
	bool dest_present = false;
	int index = -1;

	//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well.

	instr = get_next_from_ascii_file(in);    
	rinstr = cinstr_to_rinstrs(instr,no_rinstrs);
	for(int i=no_rinstrs - 1; i>=0; i--){
		if(rinstr[i].dst.value == dest_to_track){
			assert( (rinstr[i].dst.type != IMM_FLOAT_TYPE) && (rinstr[i].dst.type != IMM_INT_TYPE) );
			index = i;
			dest_present = true;
			break;
		}
	}

	assert( (dest_present == true) && (index >= 0) ); //we should have found the destination

	//do the initial processing
	for(int i=index; i>=0; i--){
		tree->update_frontier(&rinstr[i]);
	}

	//exit(0);

	
	//do the rest of expression tree building
	while(!in.eof()){
		instr = get_next_from_ascii_file(in);
		if(instr != NULL){
			rinstr = cinstr_to_rinstrs(instr,no_rinstrs);
			cout << "to " << no_rinstrs << endl;
			for(int i=no_rinstrs - 1; i>=0; i--){
				tree->update_frontier(&rinstr[i]);
			}
		}
	}

	tree->flatten_to_expression(out);

	in.close();
	out.close();
	
	return 0;

 }


 