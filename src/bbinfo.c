#include "bbinfo.h"


/* 
	client for bb information tracking for seperate modules
	Is extensible and not optimized

	filters - 
	1. Read from a file the bbs to track 
	2. Give out a set of modules to keep track of - need to recompile as it is encoded in main_modules array
	3. Track of them all

	tracked info 
	1. bb execution frequencies 
	2. callers of a particular bb + frequecies 
	3. If bb is a call target who called it + frequencies + bb which called it

*/

/*  
   all global state should be stored using thread local storage

   algorithm for getting call target
   final instr of bb record and note whether it is a call instruction
   then the next bb (the first instruction) will be the call target - record this

   algorithm for getting the from bbs
   record the bb last executed
   then the next bb will the target - record from bb 

   algorithm for getting the to bbs
   this is a little tricky and will not be implemented directly as of yet.   
   indirectly can be got by a walk and that will be implemented
   
*/

/* defines */
#define MAX_STRING_POINTERS 1000000

/* modes */
#define FILTER_FROM_FILE		1
#define FILTER_FROM_ARRAY_MOD	2
#define FILTER_NONE				3

/* macros */
#define TESTALL(mask, var) (((mask) & (var)) == (mask))
#define TESTANY(mask, var) (((mask) & (var)) != 0)

/* typedefs */
typedef struct _per_thread_data_t {

	char module_name[MAX_STRING_LENGTH];
	int prev_bb_start_addr;
	bool is_call_ins;
	int call_ins_addr;

} per_thread_data_t;


/*analysis clean calls*/
static void clean_call(void* bb,int offset,const char * module,uint is_call,uint call_addr);

/*debug and auxiliary prototypes*/
void print_commandline_args (client_arg_t * args);
bool parse_commandline_args (const char * args);
void get_full_filename_with_process(char * fileName,char * dest,uint processId);
void get_full_filename(char * fileName, char * dest);
static void process_output();

/*global dr variables - not thread safe*/
file_t in_file;
file_t out_file;
file_t summary_file;
module_t * head;
static void *stats_mutex; /* for multithread support */
static int tls_index;

char ** string_pointers;   /* global list of string pointers */
int string_pointer_index = 0;

/* client arguments */
client_arg_t * client_arg;

/* constant values for module names */
static const char * main_modules[] = {"C:\\Program Files (x86)\\Adobe\\Adobe Photoshop CS6\\Required\\Plug-Ins\\Extensions\\MMXCore.8BX",
							"C:\\Program Files (x86)\\Adobe\\Adobe Photoshop CS6\\Required\\Plug-Ins\\Filters\\Standard MultiPlugin.8BF"};
static const int main_modules_length = 2;




void bbinfo_init(client_id_t id, 
				const char * arguments)
{
	//create file
	char filename[MAX_STRING_LENGTH];
	
	drmgr_init();

	head = md_initialize();

	//get the output files
	parse_commandline_args(arguments);


	get_full_filename_with_process(client_arg->out_filename,filename,dr_get_process_id());
		
	if(dr_file_exists(filename)){
		dr_delete_file(filename);
	}

	out_file = dr_open_file(filename,DR_FILE_WRITE_OVERWRITE);

	get_full_filename_with_process(client_arg->summary_filename,filename,dr_get_process_id());

	if(dr_file_exists(filename)){
		dr_delete_file(filename);
	}

	summary_file = dr_open_file(filename,DR_FILE_WRITE_OVERWRITE);

	if(client_arg->mode == FILTER_FROM_FILE){
		get_full_filename(client_arg->in_filename,filename);

		if(dr_file_exists(filename)){
			DR_ASSERT_MSG(false,"input file missing\n");
		}

		in_file = dr_open_file(filename,DR_FILE_READ);

		md_read_from_file(head,in_file);

	}

	//string pointers
	string_pointers = (char **)dr_global_alloc(sizeof(char *)*MAX_STRING_POINTERS);

	stats_mutex = dr_mutex_create();
		
	tls_index = drmgr_register_tls_field();

}

void 
bbinfo_thread_init(void *drcontext){

	per_thread_data_t * data = (per_thread_data_t *)dr_thread_alloc(drcontext,sizeof(per_thread_data_t));
	
	/* initialize */
	strncpy(data->module_name,"__init",MAX_STRING_LENGTH);
	data->is_call_ins = false;
	
	/* store this in thread local storage */
	drmgr_set_tls_field(drcontext, tls_index, data);

}


void 
bbinfo_thread_exit(void *drcontext){

	per_thread_data_t * data = (per_thread_data_t *)drmgr_get_tls_field(drcontext,tls_index);
	
	/* clean up memory */
	dr_thread_free(drcontext,data,sizeof(per_thread_data_t));
	

}

//need to change
static void process_output(){

	module_t * local_head = head->next;
	int size = 0;
	uint i = 0, j = 0;
	bool printed = 0;


	md_sort_bb_list_in_module(head);

	/*first get the number of modules to instrument*/
	while(local_head != NULL){
			
		printed = 0;

		dr_fprintf(out_file,"%s\n",local_head->module);
		size = local_head->bbs[0].start_addr;
		for(i=1;i<=size;i++){
			dr_fprintf(out_file,"%x - %u - ",local_head->bbs[i].start_addr,local_head->bbs[i].freq);
			if(local_head->bbs[i].freq > client_arg->threshold ){
				if(!printed){
					dr_fprintf(summary_file,"%s\n",local_head->module);
					printed = 1;
				}
				dr_fprintf(summary_file,"%x - %u - ",local_head->bbs[i].start_addr,local_head->bbs[i].freq);
			}
			for(j=1;j<=local_head->bbs[i].from_bbs[0].start_addr;j++){
				dr_fprintf(out_file,"%x(%u) ",local_head->bbs[i].from_bbs[j].start_addr,local_head->bbs[i].from_bbs[j].freq);
				if(local_head->bbs[i].freq > client_arg->threshold ){
					dr_fprintf(summary_file,"%x(%u) ",local_head->bbs[i].from_bbs[j].start_addr,local_head->bbs[i].from_bbs[j].freq);
				}
			}

			dr_fprintf(out_file,"|| ");
			if(local_head->bbs[i].freq > client_arg->threshold ){
				dr_fprintf(summary_file,"|| ");
			}

			for(j=1;j<=local_head->bbs[i].called_from[0].bb_addr;j++){
				dr_fprintf(out_file,"%x - %x(%u) ",local_head->bbs[i].called_from[j].bb_addr,
											       local_head->bbs[i].called_from[j].call_point_addr,
												   local_head->bbs[i].called_from[j].freq);
				if(local_head->bbs[i].freq > client_arg->threshold ){
					dr_fprintf(summary_file,"%x - %x(%u) ",local_head->bbs[i].called_from[j].bb_addr,
														   local_head->bbs[i].called_from[j].call_point_addr,
														   local_head->bbs[i].called_from[j].freq);
				}
			}
			dr_fprintf(out_file,"\n");
			if(local_head->bbs[i].freq > client_arg->threshold ){
				dr_fprintf(summary_file,"\n");
			}
		}
		local_head = local_head->next;

	}

}

void bbinfo_exit_event(void){

	int i=0;

	process_output();
	dr_global_free(client_arg,sizeof(client_arg_t));
	md_delete_list(head);

	for(i=0;i<string_pointer_index;i++){
		dr_global_free(string_pointers[i],sizeof(char)*MAX_STRING_LENGTH);
	}

	dr_global_free(string_pointers,sizeof(char *)*MAX_STRING_POINTERS);

	drmgr_unregister_tls_field(tls_index);
	dr_mutex_destroy(stats_mutex);
	dr_close_file(summary_file);
	dr_close_file(out_file);

	drmgr_exit();
}



/* 
still we have not implemented inter module calls/bb jumps; we only update bb information if it is 
in the same module
*/
static void clean_call(void* bb,int offset,const char * module,uint is_call,uint call_addr){
	
	//get the drcontext
	void * drcontext;
	int i;
	bbinfo_t* bbinfo;
	per_thread_data_t *data ;

	bool have_bb = false;
	bool have_call = false;

	//first acquire the lock before modifying this global structure
	dr_mutex_lock(stats_mutex);

	drcontext = dr_get_current_drcontext();

	//get the tls field
	data = (per_thread_data_t *) drmgr_get_tls_field(drcontext,tls_index);

	bbinfo = (bbinfo_t*) bb;
	bbinfo->freq++;

	if(strcmp(module,data->module_name) == 0){		
		//updating from bbs
		for(i=1;i<=bbinfo->from_bbs[0].start_addr;i++){
			if(data->prev_bb_start_addr == bbinfo->from_bbs[i].start_addr){
				bbinfo->from_bbs[i].freq++;
				have_bb = true;
				break;
			}
		}
		if(!have_bb && (bbinfo->from_bbs[0].start_addr < MAX_TARGETS - 1) ){
			bbinfo->from_bbs[++(bbinfo->from_bbs[0].start_addr)].start_addr = data->prev_bb_start_addr;
			bbinfo->from_bbs[(bbinfo->from_bbs[0].start_addr)].freq = 1;
		}

		//updating call target information
		if(data->is_call_ins){
			for(i=1;i<=bbinfo->called_from[0].bb_addr;i++){
				if(data->prev_bb_start_addr == bbinfo->called_from[i].bb_addr){
					bbinfo->called_from[i].freq++;
					have_call = true;
					break;
				}
			}
			if(!have_call && (bbinfo->called_from[0].bb_addr < MAX_TARGETS - 1)){
				bbinfo->called_from[++(bbinfo->called_from[0].bb_addr)].bb_addr = data->prev_bb_start_addr;
				bbinfo->called_from[(bbinfo->called_from[0].bb_addr)].call_point_addr = data->call_ins_addr;
				bbinfo->called_from[(bbinfo->called_from[0].bb_addr)].freq = 1;
			}
		}
	}

	//update information
	
	data->prev_bb_start_addr = (uint)offset;
	strncpy(data->module_name,module,MAX_STRING_LENGTH);
	data->is_call_ins = is_call;
	data->call_ins_addr = call_addr;

	//unlock the lock
	dr_mutex_unlock(stats_mutex);


}

dr_emit_flags_t
bbinfo_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating){
	return DR_EMIT_DEFAULT;
}

dr_emit_flags_t
bbinfo_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data)
{
    return DR_EMIT_DEFAULT;
}


dr_emit_flags_t
bbinfo_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr_current, bool for_trace, bool translating,
                void *user_data)
{

		instr_t *instr;
		instr_t * first = instrlist_first(bb);
		module_data_t * module_data;
		char * module_name;
		bbinfo_t * bbinfo;
		int offset;

		int i = 0;
		bool wanted = false;

		uint is_call;
		uint call_addr;
		
		if(instr_current != first)
			return DR_EMIT_DEFAULT;

		//get the module data and if module + addr is present then add frequency counting
		module_data = dr_lookup_module(instr_get_app_pc(first));

		//dynamically generated code - module information not available
		if(module_data == NULL){  
			return DR_EMIT_DEFAULT;
		}

		module_name = (char *)dr_global_alloc(sizeof(char)*MAX_STRING_LENGTH);
		strncpy(module_name,module_data->full_path,MAX_STRING_LENGTH);

		offset = (int)instr_get_app_pc(first) - (int)module_data->start;
		bbinfo = md_lookup_bb_in_module(head,module_data->full_path,offset);

		//module is present
		if(client_arg->mode == FILTER_FROM_ARRAY_MOD){
			for(i=0;i<main_modules_length;i++){
				if(strcmp(module_data->full_path,main_modules[i]) == 0){
					wanted = true;
					break;
				}
			}
			if(wanted){
				//addr or the module is not present from what we read from file
				if(bbinfo == NULL){
					bbinfo = md_add_bb_to_module(head,module_data->full_path,offset,MAX_BBS_PER_MODULE);
				}
				DR_ASSERT(bbinfo != NULL);
			}
			else{
				dr_free_module_data(module_data);
				dr_global_free(module_name,sizeof(char)*MAX_STRING_LENGTH);
				return DR_EMIT_DEFAULT;
			}
		}
		else if(client_arg->mode == FILTER_FROM_FILE){

			//addr or the module is not present from what we read from file
			if(bbinfo == NULL){
				dr_free_module_data(module_data);
				dr_global_free(module_name,sizeof(char)*MAX_STRING_LENGTH);
				return DR_EMIT_DEFAULT;
			}
		}

		//check whether this bb has a call at the end
		instr = instrlist_last(bb);
		is_call = instr_is_call(instr);
		if(is_call){
			call_addr = (int)instr_get_app_pc(instr) - (int)module_data->start;
		}

		dr_mutex_lock(stats_mutex);
		string_pointers[string_pointer_index++] = module_name;
		dr_mutex_unlock(stats_mutex);

		dr_insert_clean_call(drcontext,bb,first,(void *)clean_call,false,5,
			OPND_CREATE_INTPTR(bbinfo),
			OPND_CREATE_INT32(offset),
			OPND_CREATE_INTPTR(module_name),
			OPND_CREATE_INT32(is_call),
			OPND_CREATE_INT32(call_addr));

		dr_free_module_data(module_data);

		return DR_EMIT_DEFAULT;
}


/* debug and auxiliary functions */

void print_commandline_args (client_arg_t * args){
	
	dr_printf("%s - %s - %s - %s\n",args->folder,args->in_filename,args->out_filename,client_arg->summary_filename);
}


bool parse_commandline_args (const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));
	if(dr_sscanf(args,"%s %s %s %s %u %u",
									   &client_arg->folder,
									   &client_arg->in_filename,
									   &client_arg->out_filename,
									   &client_arg->summary_filename,
									   &client_arg->threshold,
									   &client_arg->mode)!=6){
		return false;
	}

	
	return true;
}

void get_full_filename_with_process(char * fileName,char * dest,uint processId) {

	char filenamel[MAX_STRING_LENGTH];
	char number[MAX_STRING_LENGTH];
	filenamel[0] = '\0';

	//folder
	strncpy(dest,client_arg->folder,MAX_STRING_LENGTH);
	

	//construct the filename
	strncat(filenamel,fileName,MAX_STRING_LENGTH);
	strncat(filenamel,"_",MAX_STRING_LENGTH);
	strncat(filenamel,dr_get_application_name(),MAX_STRING_LENGTH);
	sprintf(number,"%d",processId);
	strncat(filenamel,"_",MAX_STRING_LENGTH);
	strncat(filenamel,number,MAX_STRING_LENGTH);
	strncat(filenamel,".txt",MAX_STRING_LENGTH);


	//final absolute path
	strncat(dest,filenamel,MAX_STRING_LENGTH);


}

void get_full_filename(char * fileName,char * dest){

	//folder
	strncpy(dest,client_arg->folder,MAX_STRING_LENGTH);
	
	//construct the filename
	strncat(dest,fileName,MAX_STRING_LENGTH);
	strncat(dest,".txt",MAX_STRING_LENGTH);
}