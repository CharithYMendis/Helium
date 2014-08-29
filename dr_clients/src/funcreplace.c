#include "dr_api.h"
#include "moduleinfo.h"
#include "utilities.h"
#include "drwrap.h"
#include "halide_funcs.h"
//#include <stdio.h>


/* for each client following functions may be implemented

init_func_t init_func;
exit_func_t process_exit;

drmgr_analysis_cb_t analysis_bb;
drmgr_insertion_cb_t instrumentation_bb;
drmgr_xform_cb_t app2app_bb;

thread_func_t thread_init;
thread_func_t thread_exit;


*/

static void pre_func_cb(void * wrapcxt, OUT void ** user_data);

typedef struct _client_arg_t{
	char filter_filename[MAX_STRING_LENGTH];
	uint filter_mode;
} client_arg_t;

typedef struct {
	file_t  logfile;
	file_t  outfile;
} per_thread_t;

static client_arg_t * client_arg;
static module_t * head;
static int tls_index;

static file_t logfile;
static char ins_pass_name[MAX_STRING_LENGTH];
static app_pc code_cache;

static void code_cache_init(void);
static void code_cache_exit(void);


static bool parse_commandline_args(const char * args) {

	client_arg = (client_arg_t *)dr_global_alloc(sizeof(client_arg_t));

	if (dr_sscanf(args, "%s %d", &client_arg->filter_filename,
		&client_arg->filter_mode) != 2){
		return false;
	}

	return true;
}


/* callbacks for the entire process */
void funcreplace_init(client_id_t id, const char * name, const char * arguments)
{

	char logfilename[MAX_STRING_LENGTH];
	file_t in_file;

	drmgr_init();
	drwrap_init();
	tls_index = drmgr_register_tls_field();
	DR_ASSERT(parse_commandline_args(arguments) == true);
	head = md_initialize();

	if (client_arg->filter_mode != FILTER_NONE){
		in_file = dr_open_file(client_arg->filter_filename, DR_FILE_READ);
		md_read_from_file(head, in_file, false);
		dr_close_file(in_file);
	}

	if (log_mode){
		populate_conv_filename(logfilename, logdir, name, NULL);
		logfile = dr_open_file(logfilename, DR_FILE_WRITE_OVERWRITE);
	}
	strncpy(ins_pass_name, name, MAX_STRING_LENGTH);
	code_cache_init();

}

void funcreplace_exit_event(void)
{

	md_delete_list(head, false);
	code_cache_exit();
	dr_global_free(client_arg, sizeof(client_arg_t));
	drmgr_unregister_tls_field(tls_index);
	if (log_mode){
		dr_close_file(logfile);
	}
	drwrap_exit();
	drmgr_exit();

}

/* callbacks for threads */
void funcreplace_thread_init(void *drcontext){
	per_thread_t * data;

	DEBUG_PRINT("%s - initializing thread %d\n", ins_pass_name, dr_get_thread_id(drcontext));
	data = dr_thread_alloc(drcontext, sizeof(per_thread_t));
	drmgr_set_tls_field(drcontext, tls_index, data);

}

void
funcreplace_thread_exit(void *drcontext){
	per_thread_t * data;
	data = drmgr_get_tls_field(drcontext, tls_index);
	dr_thread_free(drcontext, data, sizeof(per_thread_t));
	DEBUG_PRINT("%s - exiting thread done %d\n", ins_pass_name, dr_get_thread_id(drcontext));

}


/* callbacks for basic blocks */
dr_emit_flags_t
funcreplace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating,
OUT void **user_data){

	return DR_EMIT_DEFAULT;

}

dr_emit_flags_t
funcreplace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
instr_t *instr, bool for_trace, bool translating,
void *user_data)
{
	return DR_EMIT_DEFAULT;
}


dr_emit_flags_t
funcreplace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
bool for_trace, bool translating)
{
	return DR_EMIT_DEFAULT;
}

void clean_call_halide(){

	int x = 2 + 3;
	
	//printf("charith\n");
	//dr_switch_to_app_state(dr_get_current_drcontext());
	do_nothing_halide();
	//dr_switch_to_dr_state(dr_get_current_drcontext());
	//dr_printf("clean_call_halide\n");
	/*do_blur_test();
	dr_printf("clean_call_halide\n");
	drwrap_replace_native_fini(dr_get_current_drcontext());*/

}

static void pre_func_cb(void * wrapcxt, OUT void ** user_data){
	

	dr_mcontext_t mc = { sizeof(mc), DR_MC_ALL };
	uint eax;
	uint other_reg;
	uint width = 144;
	uint height = 120;
	unsigned char * values = dr_global_alloc(sizeof(unsigned char) * width * height * 3);
	unsigned char * read_back = dr_global_alloc(sizeof(unsigned char) * width * height * 3);
	int i = 0;
	int j = 0;
	uint written, read;
	uint ok;
	uint count = 0;
	uint arg = 0;


	dr_printf("pre_func_cb pre\n");
	arg = drwrap_get_arg(wrapcxt, 0);
	ok = dr_safe_read(arg, width * height * 3, values, &read);
	dr_printf("%u %u read\n", ok, read);

	/*for (int j = 0; j < height; j++){
		for (int i = 0; i < width; i++){
			values[i + (j + 0 * height) * width] = 255 - values[i + (j + 0 * height) * width];
			values[i + (j + 1 * height) * width] = 255 - values[i + (j + 1 * height) * width];
			values[i + (j + 2 * height) * width] = 255 - values[i + (j + 2 * height) * width];
			count++;
		}
	}*/

	
	dr_get_mcontext(dr_get_current_drcontext(), &mc);
	eax = reg_get_value(DR_REG_EAX, &mc);

	other_reg = reg_get_value(DR_REG_ECX, &mc);
	dr_printf("ecx - %u\n", other_reg);
	
	
	do_func_test(drwrap_get_arg(wrapcxt, 0), eax);
	
	ok = dr_safe_read(eax, width * height * 3, read_back, &read);
	dr_printf("%u %u read\n", ok, read);

	for (i = 0; i < 10; i++){
		dr_printf("%u %u\n", read_back[i], values[i]);
	}

	//ok = dr_safe_write(eax, height * width * 3, values, &written);
	//dr_printf("%u %u written\n",ok, written);

	dr_global_free(values, sizeof(unsigned char) * width * height * 3);
	dr_global_free(read_back, sizeof(unsigned char) * width * height * 3);

	dr_printf("pre_func_cb post\n");
	drwrap_skip_call(wrapcxt, 0, 0);

}


static void
code_cache_init(void)
{
	void         *drcontext;
	instrlist_t  *ilist;
	instr_t      *where;
	instr_t		 *mov;
	instr_t		 *ret;
	byte         *end;

	drcontext = dr_get_current_drcontext();
	code_cache = dr_nonheap_alloc(PAGE_SIZE,
		DR_MEMPROT_READ |
		DR_MEMPROT_WRITE |
		DR_MEMPROT_EXEC);
	ilist = instrlist_create(drcontext);
	
	where = INSTR_CREATE_push(drcontext,opnd_create_reg(DR_REG_XBP));
	instrlist_meta_append(ilist, where);
	mov = INSTR_CREATE_mov_st(drcontext, opnd_create_reg(DR_REG_XSP), opnd_create_reg(DR_REG_XBP));
	instrlist_meta_append(ilist, mov);
	ret = INSTR_CREATE_ret(drcontext);
	instrlist_meta_append(ilist, ret);
	dr_insert_clean_call(drcontext, ilist, ret, (void *)clean_call_halide, false, 0);
	
	/* Encodes the instructions into memory and then cleans up. */
	end = instrlist_encode(drcontext, ilist, code_cache, false);
	DR_ASSERT((end - code_cache) < PAGE_SIZE);
	instrlist_clear_and_destroy(drcontext, ilist);
	/* set the memory as just +rx now */
	dr_memory_protect(code_cache, PAGE_SIZE, DR_MEMPROT_READ | DR_MEMPROT_EXEC);
}

static void
code_cache_exit(void)
{
	dr_nonheap_free(code_cache, PAGE_SIZE);
}

void funcreplace_module_load(void * drcontext, module_data_t * module, bool loaded){

	module_t * md = md_lookup_module(head, module->full_path);
	int i = 0;
	app_pc address;

	DEBUG_PRINT("module load - %s\n", module->full_path);

	if (md != NULL){
		for (int i = 1; i <= md->bbs[0].start_addr; i++){
			address = md->bbs[i].start_addr + module->start;
			//DEBUG_PRINT("replacing function %x of %s with %x\n", address, module->full_path, pre_func_cb);
			//drwrap_replace(address, (app_pc)clean_call_halide, true);
			//drwrap_replace_native(address, clean_call_halide, true, 0, NULL, false);
			drwrap_wrap(address, pre_func_cb, NULL);
			DEBUG_PRINT("replaced\n");
			
		}
	}

}




