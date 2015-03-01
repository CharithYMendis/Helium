#ifndef _MODULEINFO_H
#define _MODULEINFO_H

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "common_defines.h"



/* these are actually a composition of classes; but we can simply use structs */

struct targetinfo_t{

	uint32_t target;
	uint32_t freq;

};

struct bbinfo_t{

	uint32_t start_addr;		/* offset from the start of the module */
	uint32_t size;
	uint32_t freq;				/* number of times it gets executed */
	uint32_t is_ret;
	uint32_t is_call;
	uint32_t is_call_target;

	/* we are also keeping backward information - specially when global function information is missing */
	uint32_t func_addr;

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


/* parsing file into the strucuture */
moduleinfo_t * populate_moduleinfo(std::ifstream &file);

/* information retrieval */
moduleinfo_t * find_module(moduleinfo_t * head, uint64_t start_addr);
moduleinfo_t * find_module(moduleinfo_t * head, std::string name);
funcinfo_t * find_func(moduleinfo_t * module, uint32_t start_addr);
funcinfo_t * find_func_app_pc(moduleinfo_t * module, uint32_t app_pc);
bbinfo_t * find_bb(funcinfo_t * func, uint32_t addr);
bbinfo_t * find_bb(moduleinfo_t * module, uint32_t addr);
bool is_funcs_present(moduleinfo_t * head);
uint32_t get_probable_func(moduleinfo_t * head, moduleinfo_t * current, uint32_t start_addr);
moduleinfo_t * get_probable_call_targets(moduleinfo_t * head);
moduleinfo_t * get_probable_callers(moduleinfo_t * head);
moduleinfo_t * get_call_targets(moduleinfo_t * head);

/* informational printing */
void print_moduleinfo(std::ofstream &file, moduleinfo_t * module);
void print_funcs(std::ofstream &file, moduleinfo_t * module);
void print_bbinfo(std::ofstream &file, moduleinfo_t * module);

/* filter file printing */
void print_filter_file(std::ofstream &file, moduleinfo_t * head);
void print_funcs_filter_file(std::ofstream &file, std::string name, uint32_t func_addr);
void print_bb_filter_file(std::ofstream &file, moduleinfo_t * head);


#endif