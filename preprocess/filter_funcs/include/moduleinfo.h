#ifndef _MODULEINFO_H
#define _MODULEINFO_H

#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>
#include "defines.h"



/* these are actually a composition of classes; but we can simply use structs */

struct targetinfo_t{

	uint32_t target;
	uint32_t freq;

};

struct bbinfo_t{

	uint32_t start_addr;		/* offset from the start of the module */
	uint32_t size;

	uint32_t freq;				/* number of times it gets executed */

	std::vector<targetinfo_t *> from_bbs;   /* bbs from which this bb was reached */
	std::vector<targetinfo_t *> to_bbs;	 /* to which basic blocks this bb connects to */

	std::vector<targetinfo_t *> callees;
	std::vector<targetinfo_t *> callers;
	

};

struct funcinfo_t {

	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t freq;

	std::vector<bbinfo_t *> bbs;


};

struct moduleinfo_t{

	moduleinfo_t * next; /* next module information */
	
	char name[MAX_STRING_LENGTH];  /* module full path */
	uint64_t start_addr;
	std::vector<funcinfo_t *> funcs;

	moduleinfo_t(){
		next = NULL;
	}

};

moduleinfo_t * populate_moduleinfo(const char* filename);
void print_moduleinfo(moduleinfo_t * module,const char * filename);
void print_funcs(moduleinfo_t * module,const char * filename);


#endif