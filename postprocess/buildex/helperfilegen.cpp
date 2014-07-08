#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <regex>

#define MAX_CHARACTERS 300

using namespace std;

int main(){

	ifstream in("include/defines.h");
	ofstream out_reg("include/print_regs.h");
	ofstream out_op("include/print_ops.h");
	
	char line_c[MAX_CHARACTERS];
	
	string op_regex = "OP_[a-z0-9_]+";
	string dr_regex = "DR_REG_[A-Z0-9_]+";
	string reg_regex = "\"[a-z0-9]+\"";
	
	out_op << "switch(operation){" << endl;
	out_reg << "switch(reg){" << endl;
	
	
	while(!in.eof()){
	
		in.getline(line_c,MAX_CHARACTERS);
		string line(line_c);
		
		regex reg_op(op_regex);
		regex reg_reg(reg_regex);
		regex reg_dr(dr_regex);
		
		sregex_iterator it_op(line.begin(), line.end(), reg_op);
		sregex_iterator it_reg(line.begin(),line.end(), reg_reg);
		sregex_iterator it_dr(line.begin(),line.end(), reg_dr);
		
		sregex_iterator it_end;
		
		if(it_op != it_end){
			out_op << "case " << (*it_op).str() << ": return \"" << (*it_op).str() << "\";" << endl; 
		}
		
		if((it_reg != it_end) && (it_dr != it_end)){
			out_reg << "case " << (*it_dr).str() << ": return " << (*it_reg).str() << ";" << endl; 
		}
		
	
	}
	
	out_op << "}" << endl;
	out_reg << "}" << endl;
	
	return 0;

}