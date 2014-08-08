#include "utilities.h"
#include "funcwrap.h"

/* filtering when to instrument */
bool filter_bb_level_from_list (module_t * head, instr_t * instr){


	module_data_t * module_data;
	int offset;
	bbinfo_t * bbinfo;
	app_pc pc;
	
	pc = instr_get_app_pc(instr);
	
	if(pc == 0) return false;
	
	module_data = dr_lookup_module(pc);
	
	if(module_data == NULL){
		return false;
	}


	offset = (int)instr_get_app_pc(instr) - (int)module_data->start;
	bbinfo = md_lookup_bb_in_module(head,module_data->full_path,offset);

	dr_free_module_data(module_data);

	return (bbinfo != NULL);

}

bool filter_module_level_from_list (module_t * head, instr_t * instr){
	
	module_data_t * module_data;
	module_t * mdinfo;
	app_pc pc;
	
	pc = instr_get_app_pc(instr);
	
	if(pc == 0) return false;
	
	module_data = dr_lookup_module(pc);
	
	if(module_data == NULL){
		return false;
	}

	mdinfo = md_lookup_module(head, module_data->full_path);

	dr_free_module_data(module_data);

	return (mdinfo != NULL);

}

bool filter_range_from_list (module_t * head, instr_t * instr){
	
	module_data_t * module_data;
	int offset;
	module_t * mdinfo;
	int size;
	int i;
	app_pc pc;
	
	pc = instr_get_app_pc(instr);
	
	if(pc == 0) return false;
	
	module_data = dr_lookup_module(pc);
	
	if(module_data == NULL){
		return false;
	}

	offset = (int)instr_get_app_pc(instr) - (int)module_data->start;

	mdinfo = md_lookup_module(head, module_data->full_path);

	dr_free_module_data(module_data);

	if(mdinfo == NULL){	
		return false;
	}

	/* now check for the range */
	size = mdinfo->bbs[0].start_addr;

	for(i = 1; i<size; i+=2){
		if((offset >= mdinfo->bbs[i].start_addr) && (offset <= mdinfo->bbs[i+1].start_addr)){
			//dr_printf("%d %d\n",size,offset);
			return true;
		}
	}

	return false;
}

bool neg_filter_module(module_t * head, instr_t * instr){
	return !filter_module_level_from_list(head, instr);
}

bool filter_from_list(module_t * head, instr_t * instr, uint mode){

	if(mode == FILTER_BB){
		return filter_bb_level_from_list(head,instr);
	}
	else if(mode == FILTER_MODULE){
		return filter_module_level_from_list(head,instr);
	}
	else if(mode == FILTER_RANGE){
		return filter_range_from_list(head,instr);
	}
	else if(mode == FILTER_NONE){
		return true;
	}
	else if (mode == FILTER_NEG_MODULE){
		return neg_filter_module(head, instr);
	}
	else if (mode == FILTER_FUNCTION){
		return should_filter_func();
	}

	return true;

}

bool filter_from_module_name(module_t * head, char * name, uint mode){

	if (mode == FILTER_NEG_MODULE){
		return (md_lookup_module(head, name) == NULL);
	}
	else{
		return (md_lookup_module(head, name) != NULL);
	}

}

/* need to code to dump PEB and TEB parameters - try to make it cross platform */


bool get_offset_from_module(app_pc instr_addr,uint * offset){

	module_data_t * data = dr_lookup_module(instr_addr);

	if (data != NULL){
		*offset = instr_addr - data->start;
		return true;
	}
	else{
		*offset = 0;
		return false;
	}


}


/* file name conventions adhereance */

uint get_string_length(char * string){
	
	int i = 0;

	while (string != NULL){
		i++;
		string++;
	}

	return i;

}

/* this gets the filename according to the convention */
uint populate_conv_filename(char * dest,const char * folder,const char * name, const char * other_details){

	uint len;

	
	len = dr_snprintf(dest, MAX_STRING_LENGTH, "%s\\%s_%s", folder, name, dr_get_application_name());
	if (other_details != NULL){
		len += dr_snprintf(dest + len, MAX_STRING_LENGTH, "_%s.log", other_details);
	}
	else{
		len += dr_snprintf(dest + len, MAX_STRING_LENGTH, ".log");
	}


	return len;

}
