#include "moduleinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"


/* private functions */

/* gets a new element */
static module_t * new_elem (char * name,
							unsigned int list_length){

	module_t * elem = (module_t *)dr_global_alloc(sizeof(module_t));

	elem->module = (char *)dr_global_alloc(sizeof(char)*MAX_STRING_LENGTH);
	strncpy(elem->module,name,MAX_STRING_LENGTH);

	elem->bbs = (bbinfo_t *)dr_global_alloc(sizeof(bbinfo_t)*list_length);
	elem->bbs[0].start_addr = 0;    // this is there for storing the length

	elem->size_bbs = list_length;
	elem->next = NULL;

	return elem;

}

/* gets the tail of the linked list */
static module_t * get_tail (module_t * head){
	module_t * tail = head;
	while(head != NULL){
		tail = head;
		head = head->next;
	}
	return tail;
}

/* adds an address to the linear list with addresses */
static bbinfo_t * add_bb_to_list (bbinfo_t * bb_list, unsigned int addr, bool extra_info, uint size){

	DR_ASSERT(size > bb_list[0].start_addr);
	

	bb_list[0].start_addr++;  //first element of the start address will have the length
	bb_list[bb_list[0].start_addr].start_addr = addr;
	bb_list[bb_list[0].start_addr].freq = 0;
	bb_list[bb_list[0].start_addr].printable = true;

	if(extra_info){
		//initialize from and to bbs
		bb_list[bb_list[0].start_addr].from_bbs = (call_bb_info_t *)dr_global_alloc(sizeof(call_bb_info_t)*MAX_TARGETS);
		bb_list[bb_list[0].start_addr].from_bbs[0].start_addr = 0;

		bb_list[bb_list[0].start_addr].to_bbs = (call_bb_info_t *)dr_global_alloc(sizeof(call_bb_info_t)*MAX_TARGETS);
		bb_list[bb_list[0].start_addr].to_bbs[0].start_addr = 0;

		//initialize call target
		bb_list[bb_list[0].start_addr].called_from = (call_target_info_t *)dr_global_alloc(sizeof(call_target_info_t)*MAX_TARGETS);
		bb_list[bb_list[0].start_addr].called_from[0].bb_addr = 0;

		//initialize called tos
		bb_list[bb_list[0].start_addr].called_to = (call_target_info_t *)dr_global_alloc(sizeof(call_target_info_t)*MAX_TARGETS);
		bb_list[bb_list[0].start_addr].called_to[0].bb_addr = 0;

		bb_list[bb_list[0].start_addr].func = NULL;
		bb_list[bb_list[0].start_addr].func_addr = 0;

	}
	 
	return &bb_list[bb_list[0].start_addr];

}


/* compare function for qsort */
static int compare_func (const void * a, const void * b){
	
	bbinfo_t * a_bb = (bbinfo_t *)a;
	bbinfo_t * b_bb = (bbinfo_t *)b;
	return (a_bb->start_addr - b_bb->start_addr);

}

static
void getFinalName(char * buf){

	char temp[MAX_STRING_LENGTH];
	int i=0;
	int copy_index = 0;
	while(buf[i]!=0){
		if(buf[i]=='\\'){
			temp[copy_index] = '\0';
			copy_index = 0;
			i++;
			continue;
		}
		temp[copy_index++] = buf[i];
		i++;
	}
	temp[copy_index] = '\0';

	strncpy(buf,temp,MAX_STRING_LENGTH);

}


/* public functions */

/* intial node - dummy node to initialize the data structure */
module_t * md_initialize(){
	return new_elem("__init",100);
}


int internal_compare_prefix(char * prefix, char * name){

	int i = 0;

	while (prefix[i] != '*'){
		if ( (prefix[i] != name[i]) || (name[i] == '\0') /* prefix is longer */ ){
			return 1;
		}
		i++;
	}
	return 0;

}

bool internal_is_prefix(char * string){
	/* asterix included means that this is a prefix */
	char * asterix = strchr(string, '*');
	return (asterix != NULL);

}

/* looks up the linked list using name and returns the node with matching name*/
module_t * md_lookup_module (module_t * head,char * name){

	while(head!=NULL){

		if (internal_is_prefix(head->module)){
			if (internal_compare_prefix(head->module, name) == 0){
				return head;
			}
		}
		else{
			if (strcmp(name, head->module) == 0){
				return head;
			}
		}
		head = head->next;
	}
	return NULL;

}

/* gets the module position with respect to the module head */
int md_get_module_position(module_t * head, char * name){

	int pos = 0;

	while (head != NULL){

		if (internal_is_prefix(head->module)){
			if (internal_compare_prefix(head->module, name) == 0){
				return pos;
			}
		}
		else{
			if (strcmp(name, head->module) == 0){
				return pos;
			}
		}
		head = head->next;
		pos++;
	}
	
	return -1;

}


bool md_add_module(module_t * head, char * name, uint length_list_bbs){
	
	module_t * tail;

	if (md_lookup_module(head, name) == NULL){
		tail = get_tail(head);
		tail->next = new_elem(name, length_list_bbs);
		return true;
	}

	return false;


}


/* adds an element to the linked list*/
/* important assumes that the head is not null - therefore, the user should initialize the head */
bbinfo_t * md_add_bb_to_module(module_t * head,
								char * name,
								unsigned int addr,
								unsigned int length_list_bbs,
								bool extra_info){

	module_t * module = md_lookup_module(head,name);
	module_t * new_module;
	if(module != NULL){
		return add_bb_to_list(module->bbs,addr,extra_info, module->size_bbs);
	}
	else{
		module = get_tail (head);
		new_module = new_elem(name,length_list_bbs);
		module->next = new_module;
		return add_bb_to_list(new_module->bbs,addr,extra_info,new_module->size_bbs);
	}
}

/* sorts the elements stored in individual lists of the linked list */
void md_sort_bb_list_in_module (module_t * head){
	while(head != NULL){ 
		qsort(&head->bbs[1],head->bbs[0].start_addr,sizeof(bbinfo_t),compare_func);
		head = head->next;
	}
}

/* frees memory of the stored list */
void md_delete_list (module_t * head, bool extra_info){
	
	module_t * prev;
	int i = 0;
	int j= 0;

	while(head != NULL){
		dr_global_free(head->module,sizeof(char)*MAX_STRING_LENGTH);
		if(extra_info){
			for(i=1;i<=head->bbs[0].start_addr;i++){
				dr_global_free(head->bbs[i].from_bbs,sizeof(call_bb_info_t)*MAX_TARGETS);
				dr_global_free(head->bbs[i].to_bbs,sizeof(call_bb_info_t)*MAX_TARGETS);
				dr_global_free(head->bbs[i].called_from,sizeof(call_target_info_t)*MAX_TARGETS);
				dr_global_free(head->bbs[i].called_to, sizeof(call_target_info_t)*MAX_TARGETS);
			}
		}
		dr_global_free(head->bbs,sizeof(bbinfo_t)*head->size_bbs);
		prev = head;
		head = head->next;
		dr_global_free(prev,sizeof(module_t));
	}

}


/* assumes a sorted list - does a binary search on it */
bbinfo_t* md_lookup_bb_in_module(module_t * head, char * name, unsigned int addr){
	 
	module_t * module = md_lookup_module(head,name);
	bbinfo_t * list;
	unsigned int size;
	int i;


	if(module != NULL){
		list = module->bbs;
		size = list[0].start_addr;

		for(i=1;i<=size;i++){
			if(addr == list[i].start_addr){
				return &list[i];
			}
		
		}

	}
	return NULL;

}


/* file I/O */
void md_read_from_file (module_t * head, file_t file, bool extra_info){
	 
	uint64 map_size;
	size_t actual_size;
	bool ok;
	void * map = NULL;
	char * line;

	/*loop variables*/
	int i;
	int j;

	/* linked list structure specific variables */
	int no_modules;
	int no_instructions;
	unsigned int addr;
	char module_name[MAX_STRING_LENGTH];

	/* for filling up the linked list data structure */
	module_t * elem;

	ok = dr_file_size(file,&map_size);
	if(ok){
		actual_size = (size_t)map_size;
		DR_ASSERT(actual_size == map_size);
		map = dr_map_file(file, &actual_size, 0, NULL, DR_MEMPROT_READ, 0);
	}


	dr_sscanf((char *)map,"%d\n",&no_modules);
	//dr_printf("%d\n",no_modules);  //debug

	line = (char *)map;
	for(i=0;i<no_modules;i++){
		
		line = strchr(line,'\n');
		line++; //start of the next line
		
		//dr_sscanf(line,"%[^\t\n]\n",module_name);
		dr_get_token(line,module_name,MAX_STRING_LENGTH);
		//getFinalName(module_name);


		//dr_printf("%s\n",module_name); //debug

		line = strchr(line,'\n');
		line++; //start of the next line
		dr_sscanf(line,"%d\n",&no_instructions);
		//dr_printf("%d\n",no_instructions);  //debug

		//create a new element
		elem = new_elem(module_name,no_instructions+2);
		head->next = elem;
		head = elem;

		for(j=0;j<no_instructions;j++){

			line = strchr(line,'\n');
			line++; //start of the next line
			dr_sscanf(line,"%u\n",&addr);
			//dr_printf(line,"%x\n",addr); //debug
			add_bb_to_list(head->bbs,addr, extra_info, head->size_bbs);
		}

	}

}

void print_bb_info(bbinfo_t * bb, file_t file, bool extra_info){

	int i = 0;

	DR_ASSERT(bb != NULL);

	if (!extra_info){
		dr_fprintf(file, "%x\n", bb->start_addr);
	}
	else{
		/*func, bb_addr, size, freq, is_call, is_ret, <from_bbs_count>, from_bb, freq, .., <caller_count>, caller, freq, ..., <to_bbs_count>, to_bbs, freq, ..., <callee_count>, callee, freq*/
		if (bb->func != NULL){
			dr_fprintf(file,"%x,", bb->func->start_addr);
		}
		else{
			dr_fprintf(file, "0,");
		}

		//dr_fprintf(file,"%x,",bb->func_addr);
		dr_fprintf(file, "%x,%u,%u,", bb->start_addr, bb->size, bb->freq);
		dr_fprintf(file, "%u,%u,%u,", bb->is_call, bb->is_ret, bb->is_call_target);
		dr_fprintf(file, "%u,", bb->from_bbs[0].start_addr);

		for (i = 1; i <= bb->from_bbs[0].start_addr; i++){
			dr_fprintf(file, "%x,%u,", bb->from_bbs[i].start_addr, bb->from_bbs[i].freq);
		}
		dr_fprintf(file, "%u,", bb->to_bbs[0].start_addr);
		for (i = 1; i <= bb->to_bbs[0].start_addr; i++){
			dr_fprintf(file, "%x,%u,", bb->to_bbs[i].start_addr, bb->to_bbs[i].freq);
		}
		dr_fprintf(file, "%u,", bb->called_from[0].bb_addr);
		for (i = 1; i <= bb->called_from[0].bb_addr; i++){
			dr_fprintf(file, "%x,%u,", bb->called_from[i].bb_addr, bb->called_from[i].freq);
		}
		dr_fprintf(file, "%u,", bb->called_to[0].bb_addr);
		for (i = 1; i <= bb->called_to[0].bb_addr; i++){
			dr_fprintf(file, "%x,%u,", bb->called_to[i].bb_addr, bb->called_to[i].freq);
		}
		dr_fprintf(file, "\n");
	}

}


void md_print_to_file (module_t * head,file_t file, bool extra_info){

	/* output format
		number of modules
		<module name>
		number of addresses
		addresses
		<module name>
		number of addresses
		addresses
		...
	*/


	int i;
	int limit;
	module_t * module;
	unsigned int number = 0;

	/*first get the number of modules to instrument*/
	module = head->next;
	while(module != NULL){
		number++;
		module = module->next;
	}
	dr_fprintf(file,"%u\n",number);

	head = head->next;
	while(head != NULL){
		dr_fprintf(file,"%s\n",head->module);
		dr_fprintf(file, "%x\n", head->start_addr);
		limit = head->bbs[0].start_addr;
		dr_fprintf(file,"%u\n",limit);
		for(i=1;i<=limit;i++){
			print_bb_info(&head->bbs[i], file, extra_info);
			//dr_fprintf(file,"%u\n",head->bbs[i].start_addr);
		}
		head = head->next;
	}


}
