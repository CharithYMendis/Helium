#include <stdio.h>
 #include <iostream>
 #include <vector>
 #include <string.h>
 #include <sstream>
 #include <stdlib.h>
 #include <fstream>

 #include "assert.h"
 #include "fileparser.h"

 int main(int argc, char ** argv){

	assert(argc == 3);
	
	ifstream in;
	in.open(argv[1],std::ifstream::in);
	ofstream out;
	out.open(argv[2],std::ofstream::out);

	//this is for reversing the file - only works when compiled using gcc
	if(in.good() && out.good()){
		reverse_file(out,in);
	}

	in.close();
	out.close();
	
	return 0;

 }