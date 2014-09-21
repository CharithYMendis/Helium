#include "dr_api.h"
#include "defines.h"
#include "drutil.h"
#include "moduleinfo.h"
#include "utilities.h"
#include "memdump.h"


/* for each client following functions may be implemented

init_func_t init_func;
exit_func_t process_exit;

drmgr_analysis_cb_t analysis_bb;
drmgr_insertion_cb_t instrumentation_bb;
drmgr_xform_cb_t app2app_bb;

thread_func_t thread_init;
thread_func_t thread_exit;


*/

#define MAX_CLONE_INS	100
#define MAX_REGIONS		100

typedef struct _client_arg_t{

	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
	char app_pc_filename[MAX_STRING_LENGTH];
	char output_folder[MAX_STRING_LENGTH];

} client_arg_t;

typedef struct {
	file_t  logfile;
	file_t  outfile;
} per_thread_t;

typedef struct {

	app_pc base_pc;
	uint size;

} mem_alloc_t ;


/******************************************global variables****************************/

static client_arg_t * client_arg;
static module_t * done_head;
static module_t * filter_head;
static module_t * app_pc_head;
static int tls_index;
static void * mutex;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];

static instr_t ** instr_clones[MAX_CLONE_INS];
static uint instr_clone_amount = 0;

static mem_alloc_t read_regions[MAX_REGIONS];
static uint read_region_size = 0;
static mem_alloc_t write_regions[MAX_REGIONS];
static uint write_region_size = 0;
static uint written_count = 0;

/********************************implementation******************************************/

static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d %s %s", &client_arg->filter_filename,
									&client_arg->filter_mode,
									&client_arg->app_pc_filename,
									&client_arg->output_folder) != 4){
		return false;
	}

	return true;
}


/* callbacks for the entire process */
void memdump_init(client_id_t id, const char * name, const char * arguments)
{

	char logfilename[MAX_STRING_LENGTH];
	file_t in_file;

	drmgr_init();
	drutil_init();
	drwrap_init();
	tls_index = drmgr_register_tls_field();
	DR_ASSERT(parse_commandline_args(arguments) == true);

	filter_head = md_initialize();
	done_head = md_initialize();
	app_pc_head = md_initialize();
	
	in_file = dr_open_file(client_arg->filter_filename, DR_FILE_READ);
	md_read_from_file(filter_head, in_file, false);
	dr_close_file(in_file);
	
	in_file = dr_open_file(client_arg->app_pc_filename, DR_FILE_READ);
	md_read_from_file(app_pc_head, in_file, false);
	dr_close_file(in_file);


	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);
	mutex = dr_mutex_create();

}

void memdump_exit_event(void)
{

	int i = 0;

	md_delete_list(filter_head, false);
	md_delete_list(done_head, false);
	md_delete_list(app_pc_head, false);

	dr_global_free(client_arg, sizeof(client_arg_t));
	drmgr_unregister_tls_field(tls_index);
	if (log_mode){
		dr_close_file(logfile);
	}
	for (i = 0; i < instr_clone_amount; i++){
		instr_destroy(dr_get_current_drcontext(), instr_clones[i]);
	}
	dr_mutex_destroy(mutex);
	drutil_exit();
	drmgr_exit();
	drwrap_exit();

}

/* callbacks for threads */
void memdump_thread_init(void *drcontext){
	per_thread_t * data;

	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));
	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_index, data);

}

void
memdump_thread_exit(void *drcontext){
	per_thread_t * data;
	data = drmgr_get_tls_field(drcontext, tls_index);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));
	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}

/*************utility functions**********************/

static bool is_mem_region_present(mem_alloc_t * region, app_pc base_pc, uint size, uint region_size){

	int i = 0; 
	for (i = 0; i < region_size; i++){
		if (region[i].base_pc == base_pc && region[i].size == size){
			return true;
		}
	}
	return false;
}

static void add_to_mem_region(mem_alloc_t * region, app_pc base_pc, uint size, uint * region_size){


	DR_ASSERT(*region_size < MAX_REGIONS);
	region[*region_size].base_pc = base_pc;
	region[*region_size].size = size;
	(*region_size)++;


}

static char * get_mem_dump_filename(app_pc base_pc, uint size, uint write, uint other_info){

	char other_details[MAX_STRING_LENGTH];
	char * filename = dr_global_alloc(sizeof(char) * MAX_STRING_LENGTH);
	
	dr_snprintf(other_details, MAX_STRING_LENGTH, "%x_%d_%d_%d", base_pc, size, write, other_info);
	populate_conv_filename(filename, client_arg->output_folder, ins_pass_name, other_details);
	return filename;


}

static void do_mem_dump(file_t file, uint base_pc,uint size){

	uint read;
	bool ok;
	byte * mem_values = dr_global_alloc(sizeof(byte) * size);
	ssize_t written;

	ok = dr_safe_read(base_pc, size, mem_values, &read);
	DR_ASSERT(ok);
	written = dr_write_file(file, mem_values, size);
	DEBUG_PRINT("read %d from %x of size %d and written %d\n", read, base_pc, size, written);

	dr_global_free(mem_values, sizeof(byte) * size);

}

void clean_call_mem_information(instr_t * instr, app_pc mem_val, uint write){

	void * drcontext = dr_get_current_drcontext();
	module_data_t * data = dr_lookup_module(instr_get_app_pc(instr));
	uint offset;

	app_pc base_pc;
	size_t size;
	uint prot;
	file_t dump_file;
	char * dump_filename;
	
	DR_ASSERT(data != NULL);
	offset = instr_get_app_pc(instr) - data->start;

	dr_mutex_lock(mutex);

	//if (!md_lookup_bb_in_module(done_head, data->full_path, offset)){

		//md_add_bb_to_module(done_head, data->full_path, offset, MAX_BBS_PER_MODULE, false); 
		dr_query_memory(mem_val, &base_pc, &size, &prot);
		//DEBUG_PRINT("base pc - %x, size - %u, write - %u\n", base_pc, size, write);
		if (write){  /* postpone till the end of the function */
			if (!is_mem_region_present(write_regions, base_pc, size, write_region_size)){
				DEBUG_PRINT("write registered - offset - %x memval %x\n", offset, mem_val);
				add_to_mem_region(write_regions, base_pc, size, &write_region_size);
				DEBUG_PRINT("base pc %x, size %d\n", base_pc, size); 
			}
		}
		else{
			if (!is_mem_region_present(read_regions, base_pc, size, read_region_size)){
				add_to_mem_region(read_regions, base_pc, size, &read_region_size);
				//DEBUG_PRINT("size - %d\n", read_region_size);
				//DEBUG_PRINT("present - %d\n", is_mem_region_present(read_regions, base_pc, size, read_region_size));
				//dr_abort();
				DEBUG_PRINT("read registered - offset - %x memval %x\n", offset,  mem_val);
				DEBUG_PRINT("base pc %x, size %d\n", base_pc, size);
				dump_filename = get_mem_dump_filename(base_pc, size, write,0);
				dump_file = dr_open_file(dump_filename, DR_FILE_WRITE_OVERWRITE);
				DEBUG_PRINT("%s dumping file\n", dump_filename);
				do_mem_dump(dump_file, base_pc, size);
				DEBUG_PRINT("file dumped\n");
				dr_global_free(dump_filename, sizeof(char) * MAX_STRING_LENGTH);
				dr_close_file(dump_file);
			}
		}



	//}

	dr_mutex_unlock(mutex);
	dr_free_module_data(data);


}

/* callbacks for basic blocks */
dr_emit_flags_t
memdump_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	return DR_EMIT_DEFAULT;

}


dr_emit_flags_t
memdump_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{

	reg_id_t reg1 = DR_REG_XAX;
	reg_id_t reg2 = DR_REG_XBX;
	int i = 0;

	if(filter_bb_level_from_list(app_pc_head, instr)){

		dr_save_reg(drcontext, bb, instr, reg1, SPILL_SLOT_1);
		dr_save_reg(drcontext, bb, instr, reg2, SPILL_SLOT_2);
		
		dr_mutex_lock(mutex);
		DEBUG_PRINT("instrumenting %x pc\n", instr_get_app_pc(instr));

		instr_clones[instr_clone_amount] = instr_clone(drcontext, instr);

		for (i = 0; i < instr_num_srcs(instr); i++){
			if (opnd_is_memory_reference(instr_get_src(instr, i))) {
				drutil_insert_get_mem_addr(drcontext, bb, instr, instr_get_src(instr,i), reg1, reg2);
				dr_insert_clean_call(drcontext, bb, instr, clean_call_mem_information, false, 3, 
					OPND_CREATE_INTPTR(instr_clones[instr_clone_amount]), opnd_create_reg(reg1), OPND_CREATE_INTPTR(false));
			}
		}
		for (i = 0; i < instr_num_dsts(instr); i++){
			if (opnd_is_memory_reference(instr_get_dst(instr, i))) {
				drutil_insert_get_mem_addr(drcontext, bb, instr, instr_get_dst(instr, i), reg1, reg2);
				dr_insert_clean_call(drcontext, bb, instr, clean_call_mem_information, false, 3, 
					OPND_CREATE_INTPTR(instr_clones[instr_clone_amount]), opnd_create_reg(reg1), OPND_CREATE_INTPTR(true));
			}
		}
		

		instr_clone_amount++;

		dr_mutex_unlock(mutex);

		dr_restore_reg(drcontext, bb, instr, reg1, SPILL_SLOT_1);
		dr_restore_reg(drcontext, bb, instr, reg2, SPILL_SLOT_2);


	}

	return DR_EMIT_DEFAULT;
}


dr_emit_flags_t
memdump_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}

static void post_func_cb(void * wrapcxt, void ** user_data){
	//do the dump for the written app_pcs
	int i = 0;
	file_t dump_file;
	char * dump_filename;

	DEBUG_PRINT("post function call for dumping\n");

	/* if for same memdump it is overwritten */
		for (i = 0; i < write_region_size; i++){
			dump_filename = get_mem_dump_filename(write_regions[i].base_pc, write_regions[i].size, true, written_count);
			dump_file = dr_open_file(dump_filename, DR_FILE_WRITE_OVERWRITE);
			do_mem_dump(dump_file, write_regions[i].base_pc, write_regions[i].size);
			dr_global_free(dump_filename, sizeof(char) * MAX_STRING_LENGTH);
			dr_close_file(dump_file);
		}
		written_count++;
	

}

void memdump_module_load(void * drcontext, module_data_t * module, bool loaded){

	module_t * md = md_lookup_module(filter_head, module->full_path);
	int i = 0;
	app_pc address;

	
	if (md != NULL){
		for (int i = 1; i <= md->bbs[0].start_addr; i++){
			
			address = md->bbs[i].start_addr + module->start;
			DEBUG_PRINT("%s module %x function wrapping\n", md->module, address);
			drwrap_wrap(address, NULL, post_func_cb);
			DEBUG_PRINT("wrapped\n");

		}
	}

}




