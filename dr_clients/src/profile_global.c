#include "profile_global.h"
#include "utilities.h"
#include "defines.h"
#include "moduleinfo.h"
#include "drmgr.h"
#include <stdio.h>

/*
TODO:

 1.track all the bbs, but report only the reported basic blocks (track means keep per thread information for all the bbs,but only upate module information for the ones you want)
 2.intra module call addresses and bb jumps are not handled and are erroneous 
 3.to_bbs and calls_to information
 4.information is collected in a global structure, but we could move on to a per thread structure -> do it in another implementation
 5.Move to module->function->bb structure 


*/


/* 
	client for bb information tracking
	Is extensible and not optimized

	filters - 
	1. Read from a file the bbs to track 
	2. Read from a file the modules to track
	3. Read from a file the range within a modules to track
	4. No filters

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

/*********************************** defines *******************************/
#define MAX_STRING_POINTERS 1000000

/* filter modes - refer to utilities (common filtering mode for all files) */

/************************************* macros ******************************/
#define TESTALL(mask, var) (((mask) & (var)) == (mask))
#define TESTANY(mask, var) (((mask) & (var)) != 0)

/******************************** typedefs *********************************/
typedef struct _per_thread_data_t {

	bbinfo_t * bbinfo;
	char module_name[MAX_STRING_LENGTH];
	int prev_bb_start_addr;
	bool is_call_ins;
	int call_ins_addr;
	int last_call_addr;

} per_thread_data_t;

typedef struct _client_arg_t {

	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
	char output_folder[MAX_STRING_LENGTH];
	char extra_info[MAX_STRING_LENGTH];

} client_arg_t;

/***********************function prototypes**************************/

/*analysis clean calls*/
static void bbinfo_population(void* bb,int offset,const char * module,uint is_call,uint call_addr);
static void register_bb(void * bbinfo);
static void called_to_population(app_pc instr_addr, app_pc target_addr);
static void populate_call_target_information();

/*debug and auxiliary prototypes*/
static bool parse_commandline_args (const char * args);
static void print_readable_output();

/************************ global variables **************************/

file_t out_file;
static module_t * filter_head;
static module_t * info_head;
static module_t * call_target_head;
static void *stats_mutex; /* for multithread support */
static int tls_index;

char ** string_pointers;   /* global list of string pointers */
int string_pointer_index = 0;

/* client arguments */
static client_arg_t * client_arg;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];


/********************* function implementations ********************/


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));
	if (dr_sscanf(args, "%s %d %s %s",
		&client_arg->filter_filename,
		&client_arg->filter_mode,
		&client_arg->output_folder,
		&client_arg->extra_info
		) != 4){
		return false;
	}


	return true;
}

void bbinfo_init(client_id_t id, const char * name,
				const char * arguments)
{
	file_t in_file;
	char filename[MAX_STRING_LENGTH];
	char logfilename[MAX_STRING_LENGTH];
	
	drmgr_init();

	filter_head = md_initialize();
	info_head = md_initialize();
	call_target_head = md_initialize();

	DR_ASSERT(parse_commandline_args(arguments) == true);

	populate_conv_filename(filename, client_arg->output_folder, name, client_arg->extra_info);
		
	if(dr_file_exists(filename)){
		dr_delete_file(filename);
	}

	out_file = dr_open_file(filename,DR_FILE_WRITE_OVERWRITE);

	
	if(client_arg->filter_mode != FILTER_NONE){
		strncpy(filename, client_arg->filter_filename, MAX_STRING_LENGTH);

		if(!dr_file_exists(filename)){
			DR_ASSERT_MSG(false,"input file missing\n");
		}

		in_file = dr_open_file(filename,DR_FILE_READ);
		md_read_from_file(filter_head,in_file,true);
		dr_close_file(in_file);

	}

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);

	//string pointers
	string_pointers = (char **)dr_global_alloc(sizeof(char *)*MAX_STRING_POINTERS);

	stats_mutex = dr_mutex_create();
		
	tls_index = drmgr_register_tls_field();

}

void bbinfo_exit_event(void){

	int i=0;

	md_sort_bb_list_in_module(info_head);
	md_print_to_file(call_target_head, logfile, false);
	populate_call_target_information();
	md_print_to_file(info_head, out_file, true);
	md_delete_list(filter_head,true);
	md_delete_list(info_head, true); 
	md_delete_list(call_target_head, false);

	for(i=0;i<string_pointer_index;i++){
		dr_global_free(string_pointers[i],sizeof(char)*MAX_STRING_LENGTH);
	}

	dr_global_free(string_pointers,sizeof(char *)*MAX_STRING_POINTERS);

	drmgr_unregister_tls_field(tls_index);
	dr_mutex_destroy(stats_mutex);
	dr_close_file(out_file);
	if (log_mode){
		dr_close_file(logfile);
	}

	dr_global_free(client_arg,sizeof(client_arg_t));
	drmgr_exit();


}

void 
bbinfo_thread_init(void *drcontext){

	per_thread_data_t * data = (per_thread_data_t *)dr_thread_alloc(drcontext,sizeof(per_thread_data_t));
	
	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));

	/* initialize */
	strncpy(data->module_name,"__init",MAX_STRING_LENGTH);
	data->is_call_ins = false;
	
	/* store this in thread local storage */
	drmgr_set_tls_field(drcontext, tls_index, data);

	DEBUG_PRINT("%s - initializing thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}

void 
bbinfo_thread_exit(void *drcontext){

	per_thread_data_t * data = (per_thread_data_t *)drmgr_get_tls_field(drcontext,tls_index);
	
	/* clean up memory */
	dr_thread_free(drcontext,data,sizeof(per_thread_data_t));

	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}

static void populate_call_target_information(){

	module_t * local_head = info_head;
	int i = 0;
	bbinfo_t * bb;

	while (local_head != NULL){
		for (i = 1; i <= local_head->bbs[0].start_addr; i++){
			bb = md_lookup_bb_in_module(call_target_head, local_head->module, local_head->bbs[i].start_addr);
			if (bb != NULL){
				local_head->bbs[i].is_call_target = true;
			}
			else{
				local_head->bbs[i].is_call_target = false;
			}
		}
		local_head = local_head->next;
	}
}

static void print_readable_output(){

	module_t * local_head = info_head->next;
	int size = 0;
	uint i = 0, j = 0;
	bool printed = 0;


	md_sort_bb_list_in_module(info_head);

	/*first get the number of modules to instrument*/
	while(local_head != NULL){
			
		printed = 0;

		dr_fprintf(out_file,"%s\n",local_head->module);
		size = local_head->bbs[0].start_addr;
		for(i=1;i<=size;i++){
			dr_fprintf(out_file,"%x - %u - ",local_head->bbs[i].start_addr,local_head->bbs[i].freq);
			
			for(j=1;j<=local_head->bbs[i].from_bbs[0].start_addr;j++){
				dr_fprintf(out_file,"%x(%u) ",local_head->bbs[i].from_bbs[j].start_addr,local_head->bbs[i].from_bbs[j].freq);
			}

			dr_fprintf(out_file,"|| ");
			

			for(j=1;j<=local_head->bbs[i].called_from[0].bb_addr;j++){
				dr_fprintf(out_file,"%x - %x(%u) ",local_head->bbs[i].called_from[j].bb_addr,
											       local_head->bbs[i].called_from[j].call_point_addr,
												   local_head->bbs[i].called_from[j].freq);
			}

			dr_fprintf(out_file, ": func : %x",local_head->bbs[i].func->start_addr);
			dr_fprintf(out_file,"\n");
	
		}
		local_head = local_head->next;

	}

}


/* 
still we have not implemented inter module calls/bb jumps; we only update bb information if it is 
in the same module
*/
static void bbinfo_population(void* bb,int offset,const char * module,uint is_call, uint call_addr){
	
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
	data->bbinfo = bbinfo;
	bbinfo->freq++;
	//bbinfo->func = get_current_function(drcontext);
	bbinfo->func_addr = get_current_function_all(drcontext);

	// we are sure that the bbs are from the filtered out modules
	// updating from bbs
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

/* these two clean calls are for setting up the called_to information for bbinfo */
static void
register_bb(void * bbinfo){

	per_thread_data_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	data->bbinfo = (bbinfo_t *)bbinfo;

}

static void 
called_to_population(app_pc instr_addr, app_pc target_addr){

	per_thread_data_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	module_data_t * module = dr_lookup_module(instr_addr);
	module_data_t * target_module = dr_lookup_module(target_addr);
	
	int i = 0;
	int num_calls = 0;
	uint offset = 0;

	bool found = false;

	//DR_ASSERT(module != NULL);
	//DR_ASSERT(target_module != NULL);

	//dr_printf("at call - %x\n", data->bbinfo);

	if ( (module != NULL) && (target_module != NULL) ){
		if (strcmp(module->full_path, target_module->full_path) == 0){

			offset = target_addr - target_module->start;

			num_calls = data->bbinfo->called_to[0].bb_addr;
			for (i = 1; i <= num_calls; i++){
				if (data->bbinfo->called_to[i].bb_addr == offset){
					found = true;
					data->bbinfo->called_to[i].freq++;
					break;
				}
			}

			if (!found){
				
				if (data->bbinfo->called_to[0].bb_addr < MAX_TARGETS - 1){
					data->bbinfo->called_to[0].bb_addr++;
					data->bbinfo->called_to[data->bbinfo->called_to[0].bb_addr].bb_addr = offset;
					offset = instr_addr - module->start;
					data->bbinfo->called_to[data->bbinfo->called_to[0].bb_addr].call_point_addr = offset;
					data->bbinfo->called_to[data->bbinfo->called_to[0].bb_addr].freq = 1;
				}
			}
			
		}

	}

	dr_free_module_data(module);
	dr_free_module_data(target_module);
}

static void
call_target_info(app_pc instr_addr, app_pc target_addr){

	module_data_t * module_data = dr_lookup_module(target_addr);
	uint offset;
	bbinfo_t * bb;

	if (module_data != NULL){
		offset = target_addr - module_data->start;
		dr_mutex_lock(stats_mutex);
		bb = md_lookup_bb_in_module(call_target_head, module_data->full_path, offset);
		if (bb == NULL){
			md_add_bb_to_module(call_target_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, false);
		}
		called_to_population(instr_addr, target_addr);
		dr_mutex_unlock(stats_mutex);
	}

	dr_free_module_data(module_data);


}

void call_target_info_wo_called_to(app_pc instr_addr,app_pc target_addr){

	module_data_t * module_data = dr_lookup_module(target_addr);
	per_thread_data_t * data = drmgr_get_tls_field(dr_get_current_drcontext(), tls_index);
	uint offset;
	bbinfo_t * bb;

	if (module_data != NULL){
		offset = target_addr - module_data->start;
		dr_mutex_lock(stats_mutex);
		bb = md_lookup_bb_in_module(call_target_head, module_data->full_path, offset);
		if (bb == NULL){
			md_add_bb_to_module(call_target_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, false);
		}
		dr_mutex_unlock(stats_mutex);
		data->last_call_addr = offset;
	}
	dr_free_module_data(module_data);
}

dr_emit_flags_t
bbinfo_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr_current, bool for_trace, bool translating,
                void *user_data)
{

		instr_t *instr;
		instr_t * first = instrlist_first(bb);
		instr_t  *last = instrlist_last(bb);
		module_data_t * module_data;
		char * module_name;
		bbinfo_t * bbinfo;
		int offset;

		uint is_call;
		uint is_ret;
		uint call_addr;

		uint bb_size = 0;
		int i = 0;

		uint filtered = true;

		uint srcs;
		reg_id_t reg1 = DR_REG_XAX;
		reg_id_t reg2 = DR_REG_XBX;
		opnd_t opnd1;
		opnd_t opnd2;


		/* get the first non meta instruction */
		for (instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)){
			if (instr_ok_to_mangle(instr)){
				first = instr;
				break;
			}
		}
		instr = NULL;

		if(instr_current != first && instr_current != last)
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
		bbinfo = md_lookup_bb_in_module(info_head,module_data->full_path,offset);


		/* populate and filter the bbs if true go ahead and do instrumentation */
		/* range filtering is not supported as we changing the data structure in place */
		if(client_arg->filter_mode == FILTER_MODULE){
			if(filter_module_level_from_list(filter_head,first)){
				//addr or the module is not present from what we read from file
				if(bbinfo == NULL){
					bbinfo = md_add_bb_to_module(info_head,module_data->full_path,offset,MAX_BBS_PER_MODULE,true);
				}
				DR_ASSERT(bbinfo != NULL);
			}
			else{
				filtered = false;
			}
			
		}
		else if(client_arg->filter_mode == FILTER_BB){
			//addr or the module is not present from what we read from file
			if (filter_bb_level_from_list(filter_head, first)){
				if (bbinfo == NULL){
					bbinfo = md_add_bb_to_module(info_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, true);
				}
				DR_ASSERT(bbinfo != NULL);
			}
			else{
				filtered = false;
			}
			
		}
		else if(client_arg->filter_mode == FILTER_NONE){
			if(bbinfo == NULL){
				bbinfo = md_add_bb_to_module(info_head,module_data->full_path,offset,MAX_BBS_PER_MODULE,true);
			}
			DR_ASSERT(bbinfo != NULL);
		}
		else if (client_arg->filter_mode == FILTER_NEG_MODULE){
			if (filter_from_module_name(filter_head, module_name, client_arg->filter_mode)){
				if (bbinfo == NULL){
					bbinfo = md_add_bb_to_module(info_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, true);
				}
				DR_ASSERT(bbinfo != NULL);
			}
			else{
				filtered = false;
			}
		}
		else if (client_arg->filter_mode == FILTER_FUNCTION){
			if (filter_from_list(filter_head, first, client_arg->filter_mode)){
				if (bbinfo == NULL){
					bbinfo = md_add_bb_to_module(info_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, true);
				}
			}
			else{
				filtered = false;
			}
		}
		else if (client_arg->filter_mode == FILTER_NUDGE){
			if (filter_from_list(filter_head, first, client_arg->filter_mode)){
				//dr_printf("profile came- %s %d\n", module_data->full_path, offset);
				if (bbinfo == NULL){
					bbinfo = md_add_bb_to_module(info_head, module_data->full_path, offset, MAX_BBS_PER_MODULE, true);
				}
			}
			else{
				filtered = false;
			}
		}

		/* if the instr is filtered; only if instr == first; as this will be in the diff file */
		if (filtered){

			/* log the disassembly */
			if (log_mode && (instr_current == first)){

				dr_fprintf(logfile, "%s %d\n", module_name, offset);
				instrlist_disassemble(drcontext, instr_get_app_pc(first), bb, logfile);
			}

			DR_ASSERT(bbinfo != NULL);

			/* optimize this to only run if module is not found */
			md_lookup_module(info_head, module_name)->start_addr = module_data->start;


			//check whether this bb has a call at the end or a ret at the end
			instr = instrlist_last(bb);
			is_call = instr_is_call(instr);
			if (is_call){
				call_addr = (int)instr_get_app_pc(instr) - (int)module_data->start;
			}
			is_ret = instr_is_return(instr);


			bbinfo->is_call = is_call;
			bbinfo->is_ret = is_ret;
			bbinfo->size = instr_get_app_pc(instrlist_last(bb)) - instr_get_app_pc(first) + instr_length(drcontext, instrlist_last(bb));

			dr_mutex_lock(stats_mutex);
			string_pointers[string_pointer_index++] = module_name;
			dr_mutex_unlock(stats_mutex);

			dr_insert_clean_call(drcontext, bb, first, (void *)bbinfo_population, false, 5,
				OPND_CREATE_INTPTR(bbinfo),
				OPND_CREATE_INT32(offset),
				OPND_CREATE_INTPTR(module_name),
				OPND_CREATE_INT32(is_call),
				OPND_CREATE_INT32(call_addr));
		}

		dr_free_module_data(module_data);
		dr_global_free(module_name, sizeof(char)*MAX_STRING_LENGTH);

		if (!filtered){

			if (instr_is_call_direct(last)){

				srcs = instr_num_srcs(last);
				if (srcs < 1){
					dr_printf("%d\n", srcs);
				}
				DR_ASSERT(instr_num_srcs(last) >= 1);
				call_target_info_wo_called_to(instr_get_app_pc(last), opnd_get_addr(instr_get_src(last, 0)));
			}
			else if (instr_is_call_indirect(last)){
				dr_insert_mbr_instrumentation(drcontext, bb, last, (app_pc)call_target_info_wo_called_to, SPILL_SLOT_1);
			}

		}
		else{
			if (instr_current == last){
				//dr_insert_clean_call(drcontext, bb, instr_current, (void *)register_bb, false, 1, OPND_CREATE_INTPTR(bbinfo));
				if (instr_is_call_direct(last)){
					dr_insert_call_instrumentation(drcontext, bb, instr_current, (app_pc)call_target_info);
				}
				else if (instr_is_call_indirect(last)){
					dr_insert_mbr_instrumentation(drcontext, bb, instr_current, (app_pc)call_target_info, SPILL_SLOT_1);
				}
				if (instr_current != first){
					return DR_EMIT_DEFAULT;
				}
			}
		}
		


		return DR_EMIT_DEFAULT;
}

