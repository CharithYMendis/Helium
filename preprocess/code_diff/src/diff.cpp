#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

//dynamically generated code not handled

using namespace std;

typedef unsigned long long uint64;
typedef unsigned int uint32;

struct modules_t {
  string name;
  uint64 mount;
  vector<uint64> addresses;
};

struct returnParse_t {
  uint64 noOfBasicBlocks;
  uint64 noOfModules;
  modules_t* module;
  vector<uint64> duplicateIndexes;
  uint64 *locateOriginals;
};

//get the number of basic blocks - disparity due to 65535 
void checkStructure(returnParse_t* first){

	uint64 noOfBBs = 0;
	for(int i=0;i<first->noOfModules;i++){
		noOfBBs += first->module[i].addresses.size();
	}
	
	cout << first->noOfBasicBlocks << " " << noOfBBs << endl;
	
}

//checks for unique modules in first file (compared with the second file)
void checkModules (returnParse_t* first,returnParse_t* second){
	
	for(int i=0;i<first->noOfModules;i++){
		string firstName = first->module[i].name;
		bool found = 0;
		for(int j=0;j<second->noOfModules;j++){
			string secondName = second->module[j].name;
			if(secondName.compare(firstName) == 0){
				found = 1;
				break;
			}
		}
		if(!found){
			cout << firstName << endl;
		}
	}

}

//checks for duplicate names
void checkDuplicates(returnParse_t* first){
	int dups = 0;
	for(int i=0;i<first->noOfModules;i++){
		string firstName = first->module[i].name;
		bool found = 0;
		for(int j=0;j<first->noOfModules;j++){
			if(i==j) continue;
			string secondName = first->module[j].name;
			if(secondName.compare(firstName) == 0){
				found = 1;
				dups++;
				break;
			}
		}
		if(found){
			cout << firstName << endl;
		}
	}
	cout << dups << endl;

}


void searchAndReport(returnParse_t* first, returnParse_t* second, returnParse_t* report){
  
  uint64 matched = 0;
  uint64 notMatched = 0;
  uint64 notInModule = 0;
  
  modules_t* reportModule = report->module;
  unsigned int currentModule = 0;
  
  
  //get the number of modules that match

  //things in first and not in second
  for(int i=0;i<first->noOfModules;i++){
  
	unsigned int printName = 1;

	//check if this is a duplicate
	if(first->locateOriginals[i] != -1){  //duplicate - accounted for all in the original
		continue;
	}
  
    string firstName = first->module[i].name;
    int secondIndex = -1;
    
    //if it is not a photoshop module
    if(firstName.find("Photo") == string::npos){
      continue;
    }
	
	//remove unknowns
	if(firstName.find("C:") == string::npos){
		continue;
	}


    for(int j=0;j<second->noOfModules;j++){
      string secondName = second->module[j].name;
      if(secondName.compare(firstName) == 0){
		secondIndex = j;  //get the first index of the match guaranteed to the original
		break;
      }
    }
	
	if(secondIndex == -1){
		notInModule++;
		cout << "not matched -> " <<  firstName << endl;
	}

    if(secondIndex != -1){

	  vector<uint64> firstAdd = first->module[i].addresses;
	  vector<uint64> secondAdd = second->module[secondIndex].addresses;

	  if(first->module[i].name.compare(second->module[secondIndex].name)!=0){
		cout << "assert failed" << endl;
	  }

		for(int j=0;j<firstAdd.size();j++){
			if(find(secondAdd.begin(),secondAdd.end(),firstAdd[j])==secondAdd.end()){
				notMatched++;
				if(printName){
					printName = 0;
					currentModule++;
					reportModule[currentModule-1].name = firstName;
				}		
				reportModule[currentModule-1].addresses.push_back(firstAdd[j]);
			}
			else{
			  matched++;
			}  
		}
	}
    else{
		notMatched += first->module[i].addresses.size();
		if(printName == 1){
			printName = 0;
			currentModule++;
			reportModule[currentModule-1].name = firstName;
		}
		vector<uint64> firstAdd = first->module[i].addresses;
		for(int j=0;j<firstAdd.size();j++){
			reportModule[currentModule-1].addresses.push_back(firstAdd[j]);
		}
    }

  }
  
  report->noOfModules = currentModule;

  cout << "summary : " << endl;
  cout << matched << " " << notMatched << endl; 
  cout << notInModule << endl;
  
}

string getModule (const char*  line){
  int number, start;
  char moduleName[200];
  sscanf(line,"%d, %d, %[^\n]",&number,&start,moduleName);
  string str(moduleName);
  return str;
}

unsigned int fillBBdata (returnParse_t* returnVal,const char* line,int maxModules){
  
  unsigned int moduleNumber,size;
  uint64 startAddress;
  modules_t* modules = returnVal->module;
  
  if(sscanf(line,"module[%u]: %llx,%u",&moduleNumber,&startAddress,&size)!=3){
	cout << "assert failed" << endl;
  }
  if(moduleNumber >= maxModules){ 
    return moduleNumber;
  }
  
  uint64 place;
  if(find(returnVal->duplicateIndexes.begin(),returnVal->duplicateIndexes.end(),moduleNumber) == returnVal->duplicateIndexes.end()){ //not duplicate
	place = moduleNumber;
  }
  else{  //duplicates
	place = returnVal->locateOriginals[moduleNumber];
  }
  
  modules[place].addresses.push_back(startAddress);

  return place;
  
} 

//remove duplicate entries
void removeDuplicates(returnParse_t* first){

	for(int i=0;i<first->noOfModules;i++){
		sort(first->module[i].addresses.begin(),first->module[i].addresses.end());
		first->module[i].addresses.erase(
			unique(first->module[i].addresses.begin(),first->module[i].addresses.end()),
			first->module[i].addresses.end());
	}

}


//need a function to parse the a file and return a data structure 
returnParse_t* parseFiles(ifstream &file){
  
  string line;

  //DRCOV first line
  getline(file,line);
 
  //need to get the number of modules
  getline(file,line,' ');
  getline(file,line,' ');
  getline(file,line);

  uint64 noOfModules = atoi(line.c_str());
  cout << "modules : " << noOfModules << endl;
 
  modules_t* modules = new modules_t[noOfModules + 1]; //noOfModules + space for dynamically generated code (later)
  
  returnParse_t* returnVal = new returnParse_t;
  returnVal->noOfModules = noOfModules;
  returnVal->module = modules;
  returnVal->locateOriginals = new uint64[noOfModules + 1];
  
  //initialize locate originals to -1
  for(int i=0;i<noOfModules + 1;i++){
	returnVal->locateOriginals[i] = -1;
  }
  
  for(uint64 i=0; i<noOfModules; i++){
    getline(file,line);
    string moduleName = getModule(line.c_str());
	(&modules[i])->name=moduleName;
	for(uint64 j=0;j<i;j++){
		string previousNames = modules[j].name;
		if(previousNames.compare(moduleName) == 0){ // we have a duplicate
			returnVal->duplicateIndexes.push_back(i);
			returnVal->locateOriginals[i] = j;
			break;
		}
	}
  }
  
  cout << "duplicate modules: " << returnVal->duplicateIndexes.size() << endl;

  //get the bb count
  getline(file,line,' ');
  getline(file,line,' ');
  getline(file,line,' ');
  
  uint64 noOfBasicBlocks = atoi(line.c_str()); 
  cout << "basicblocks : " << noOfBasicBlocks << endl;
  
  returnVal->noOfBasicBlocks = noOfBasicBlocks;

  getline(file,line);
  getline(file,line);
  
  //for counting invalid modules
  uint64 invalidModules = 0;

  for(uint64 i=0;i<noOfBasicBlocks;i++){
    
	unsigned int number;
	
	getline(file,line);
    number = fillBBdata(returnVal,line.c_str(),noOfModules);
	if(number>=noOfModules){
		invalidModules++;
	}
  }
  
  cout << "invalid bbs : " << invalidModules << endl;
  cout << "valid bbs :" << noOfBasicBlocks - invalidModules << endl;
  

  return returnVal;

}


void printToFile(ofstream &out, returnParse_t* data){

	out << data->noOfModules << endl;
	modules_t* modData = data->module;
	for(int i=0;i<data->noOfModules;i++){
		out << "\"" << modData[i].name << "\"" << endl;
		out << modData[i].addresses.size() << endl;
		for(int j=0;j<modData[i].addresses.size();j++){
			out << modData[i].addresses[j] << endl;
		}
	}

}

int main(int argc, char ** argv){

  if(argc != 4) { 
    cout << "not enough arguments" << endl;
    return 1; 
  }

  //pass the two files
  string first(argv[1], find(argv[1], argv[1] + 400, '\0'));
  string second(argv[2], find(argv[2], argv[2] + 400, '\0'));
  string outname(argv[3], find(argv[3], argv[3] + 400, '\0'));
  
  ifstream first_file, second_file;
  
  first_file.open(first.c_str());
  second_file.open(second.c_str());
  
  returnParse_t* first_data = parseFiles(first_file);
  returnParse_t* second_data = parseFiles(second_file);
  
  cout << "before duplicate bb removal :" << endl;
  checkStructure(first_data);
  checkStructure(second_data);
  
  removeDuplicates(first_data);
  removeDuplicates(second_data);
  
  cout << "after duplicate bb removal :" << endl;
  checkStructure(first_data);
  checkStructure(second_data);

  //if possible use the file with the highest bbs first
  //search and if there is a difference, then dump it
  
  ofstream outFile;
  outFile.open(outname.c_str());
  
  
  returnParse_t* dataReported = new returnParse_t;
  dataReported->module = new modules_t[first_data->noOfModules];

  searchAndReport(first_data,second_data,dataReported);
  
  printToFile(outFile,dataReported);

  outFile.close();

  first_file.close();
  second_file.close();

  return 0;

}
