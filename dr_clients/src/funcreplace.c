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


int amount_exec = 0;
int amount_inner = 0;
int amount_val = 0;



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

static void pre_func_cb_2(void * wrapcxt, OUT void ** user_data){

	//width and height eax and ecx -> previous function
	//input  - 0x08
	//output - 0x0c


	uint input;
	uint output;
	uint width;
	uint height;
	uint other;
	uint other_2;
	unsigned char * values;
	unsigned char * read_back;
	uint written, read;
	uint ok;
	int i = 0;
	int j = 0;

	char filename[MAX_STRING_LENGTH];
	char amount_exec_string[MAX_STRING_LENGTH];
	file_t dump_file;

	amount_exec++;
	input = drwrap_get_arg(wrapcxt, 0);
	output = drwrap_get_arg(wrapcxt, 1);
	height = drwrap_get_arg(wrapcxt, 2); //120 - height
	width = drwrap_get_arg(wrapcxt, 3);  //144 - width
	other = drwrap_get_arg(wrapcxt, 4); 
	other_2 = drwrap_get_arg(wrapcxt, 5);  

	dr_printf("%d - input - %x, output - %x, %d, %d\n", amount_exec, input, output, width, height);

	if (other != other_2){
		dr_printf("not similar\n");
	}

	values = dr_global_alloc(sizeof(unsigned char) * other * height);
	read_back = dr_global_alloc(sizeof(unsigned char) * other * height);

	//do_func_test(width, height, input, output);

	//ok = dr_safe_read(input, other * height, values, &read);
	//dr_printf("%u %u read\n", ok, read);

	/*dr_snprintf(amount_exec_string, MAX_STRING_LENGTH, "%d", amount_exec++);
	populate_conv_filename(filename, logdir, ins_pass_name, amount_exec_string);
	dump_file = dr_open_file(filename, DR_FILE_WRITE_OVERWRITE);
	dr_write_file(dump_file, values, width * height);
	dr_close_file(dump_file);*/

	do_blur_test(other, height, input, output);
	//dr_printf("%d,%d,%d\n", i, j, j*height + i);
	//for (i = 0; i < 10; i++){
	
	
	/*for (j = 0; j < height; j++){
		for (i = 0; i < width; i++){
			values[j * other + i] = 255 - values[j * other + i];
		}
	}*/
	
	//dr_printf("updating done\n");
	//dr_safe_write(output, other * height, values, &read);
	
	//dr_printf("writing done\n");

	//ok = dr_safe_read(output, other * height, read_back, &read);
	//dr_printf("%u %u read\n", ok, read);
	dr_global_free(values, other * height);
	dr_global_free(read_back, other * height);

	drwrap_skip_call(wrapcxt, 0, 0);
	


}

static void pre_func_cb_3(void * wrapcxt, OUT void ** user_data){
	//amount_val++;
	//dr_printf("%d\n", amount_val);
}

static void pre_func_cb(void * wrapcxt, OUT void ** user_data){
	

	dr_mcontext_t mc = { sizeof(mc), DR_MC_ALL };
	uint eax;
	uint other_reg;
	int i = 0;
	int j = 0;
	uint written, read;
	uint ok;
	uint count = 0;
	uint arg = 0;
	unsigned char * values;

	//ecx - full size of the image
	//0x08 - input image
	//eax - output image

	dr_printf("pre_func_cb pre\n");
	arg = drwrap_get_arg(wrapcxt, 0);
	dr_printf("input - %x\n", arg);

	dr_get_mcontext(dr_get_current_drcontext(), &mc);
	eax = reg_get_value(DR_REG_EAX, &mc);
	dr_printf("output - %x\n", eax);
	other_reg = reg_get_value(DR_REG_ECX, &mc);
	dr_printf("ecx - %u\n", other_reg);
	dr_printf("%d - inner\n", ++amount_inner);

	values = dr_global_alloc(sizeof(unsigned char) * other_reg);

	dr_safe_read(arg, other_reg, values, &read);

	for (i = 0; i < other_reg; i++){
		values[i] = 255 - values[i];
	}

	dr_safe_write(eax, other_reg, values, &read);

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
			if (md->bbs[i].start_addr == 21420880){
				drwrap_wrap(address, pre_func_cb, NULL);
			}
			else if (md->bbs[i].start_addr == 9645248){
				drwrap_wrap(address, pre_func_cb_3, NULL);
			}
			else{
				drwrap_wrap(address, pre_func_cb_2, NULL);
			}
			DEBUG_PRINT("replaced - %x\n", md->bbs[i].start_addr);
			
			//DEBUG_PRINT("replacing function %x of %s with %x\n", address, module->full_path, pre_func_cb);
			//drwrap_replace(address, (app_pc)clean_call_halide, true);
			//drwrap_replace_native(address, clean_call_halide, true, 0, NULL, false);
			
			
		}
	}

}




