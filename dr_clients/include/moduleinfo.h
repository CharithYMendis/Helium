#ifndef _MODULEINFO_EXALGO_H
#define _MODULEINFO_EXALGO_H

#include "dr_api.h"
#include "functrace.h"

#define MAX_TARGETS	100
#define MAX_BBS_PER_MODULE 10000

/* containers for bb storage - not optimized */

/*

conventions used -
bbs[0].start_addr will contain the length of valid bbs populated for this module
from_bbs[0].start_addr will contain the length of valid from bbs for this bb
to_bbs[0].bb_addr will contain the length of valid to bbs for this bb
called_from[0].bb_addr will contain the length of call targets which called this bb
*/

/* if you change bbinfo struct then you need to change functions add_pc_to_list + delete_list */

//from which bbs was this called
typedef struct _call_bb_info_t {
	char * module;
	uint start_addr;
	uint freq;
	uint is_ret;
} call_bb_info_t;

//if the bb is a call target 
typedef struct _call_target_info_t {
	char * module;
	uint bb_addr;
	uint call_point_addr;
	uint freq;
} call_target_info_t;


//basic structure to carry bb information
typedef struct _bbinfo_t {
	
	uint start_addr;
	uint freq;
	uint size;
	
	call_bb_info_t * from_bbs;
	call_bb_info_t * to_bbs;

	call_target_info_t * called_from;
	call_target_info_t * called_to;
	
	function_t * func;
	bool printable;

} bbinfo_t;



//module information
typedef struct _module_t {
	struct _module_t * next;
	char * module;
	uint64 start_addr;
	bbinfo_t * bbs;
	uint size_bbs;
} module_t;



/* get the head - handle */
module_t * md_initialize();

/* add a module to the list */
bool md_add_module(module_t * head, char * name, uint length_list_bbs);
/* add an element */
bbinfo_t * md_add_bb_to_module(module_t * head, char * name, unsigned int addr, unsigned int length_list_bbs, bool extra_info);

/* look up the module */
module_t * md_lookup_module (module_t * head,char * name);
/* check for the presence of an element */
bbinfo_t * md_lookup_bb_in_module (module_t * head, char * name, unsigned int addr);

/* parse the file and fill the linked list */
void md_read_from_file (module_t * head, file_t file, bool extra_info);
/* print the addresses according to the protocol */
void md_print_to_file (module_t * head, file_t file, bool extra_info);

/* deletes the linked list */
void md_delete_list (module_t * head, bool extra_info);

/* operations on the data structure */
/* sorts the elements stored in individual lists of the linked list */
void md_sort_bb_list_in_module (module_t * head);

#endif