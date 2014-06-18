//this is just to get the range of memory accesses

#include <stdio.h>
#include <iostream>
#include <vector>

typedef unsigned int uint;

using namespace std;

#define MAX_ACCESSES 0xffff
//#define TEST

int main(){

	FILE * trace;
	FILE * output;
	
#ifdef TEST
	trace = fopen("test.txt","r");
#else
	trace = fopen("C:\\Charith\\Dropbox\\Research\\development\\memtrace\\build_32\\bin\\memtrace.4588.log","r");
#endif

	output = fopen("output.txt","w+");

	if(trace == NULL){
		printf("file does not exist\n");
		return 1;
	}

	char read_write;
	uint instruction_addr;
	uint mem_addr;
	uint access_width;
	
	vector < pair < uint,uint > > per_ins_accesses [MAX_ACCESSES + 1];


	while(fscanf(trace,"%x,%c,%u,%x\n",&instruction_addr,&read_write,&access_width,&mem_addr) == 4){
			
		//first check whether we can merge 
		uint index = instruction_addr & 0xffff;
		
		//printf("%x,%c,%u,%x\n",instruction_addr,read_write,access_width,mem_addr);
		
		//first time for that instruction
		if(per_ins_accesses[index].size() == 0){
			//add the current entry
			pair<uint,uint> range (mem_addr,mem_addr + access_width);
			per_ins_accesses[index].push_back(range);
			continue;
		}
		
		int status = 0;
		
		for(int i=0;i<per_ins_accesses[index].size();i++){
			pair< uint,uint >  range = per_ins_accesses[index][i];		
			
			if(mem_addr + access_width <= range.second &&
				mem_addr >= range.first ){ //within the range
					status = 1;
					break;
			}	
			
			if(mem_addr + access_width > range.second && mem_addr <= range.second){  // [ *******-------]---- can be the case that mem_addr >= range.first
				per_ins_accesses[index][i].second = mem_addr + access_width;
				if(mem_addr < range.first){
					per_ins_accesses[index][i].first = mem_addr;
				}
				status = 1;
				break;
			}
			
			if(mem_addr + access_width >= range.first && mem_addr < range.first){ // ----[-----******]  can be the case that mem_addr + access_width > range.second
				per_ins_accesses[index][i].first = mem_addr;
				if(mem_addr + access_width > range.second){  // this is actually covered by the condition in the previous if
					per_ins_accesses[index][i].second = mem_addr + access_width;
				}
				status = 1;
				break;
			}			
		}
		
		//ok check status and if zero we will need a new entry
		//assume that the memory is accessed in a manner that is strided; no irregular accesses
		if(status == 0)
			per_ins_accesses[index].push_back(make_pair(mem_addr,mem_addr+access_width));

			
	}
	
	for(int i=0;i<=MAX_ACCESSES;i++){
		if(per_ins_accesses[i].size() > 0){
			fprintf(output,"%x - %u\n",i,per_ins_accesses[i].size());
			for(int j=0;j<per_ins_accesses[i].size();j++){
				fprintf(output,"(%x,%x)\n",per_ins_accesses[i][j].first,per_ins_accesses[i][j].second);
			}
		}
	}
	
	fclose(output);
	fclose(trace);
	
	return 0;


}