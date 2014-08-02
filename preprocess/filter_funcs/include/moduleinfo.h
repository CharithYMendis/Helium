#ifndef _MODULEINFO_H
#define _MODULEINFO_H

#include <stdint.h>

/* these are actually a composition of classes; but we can simply use structs */

struct bbinfo_t{

	uint32_t start_addr;		/* offset from the start of the module */
	uint32_t end_addr;

	uint32_t freq;				/* number of times it gets executed */

	uint32_t * from_bbs;   /* bbs from which this bb was reached */
	uint32_t from_bbs_size;
	uint32_t * to_bbs;	 /* to which basic blocks this bb connects to */
	uint32_t to_bbs_size;

	uint32_t * callers;
	uint32_t callers_size;
	uint32_t * callees;
	uint32_t callees_size;

};

struct funcinfo_t {

	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t freq;

	bbinfo_t * bbs;
	uint32_t bbs_size;

};

struct moduleinfo_t{

	moduleinfo_t * next; /* next module information */
	char * module;  /* module full path */
	funcinfo_t * funcs;
	uint32_t funcs_size;

};


#endif