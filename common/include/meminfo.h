#ifndef _EXALGO_MEMINFO_H
#define _EXALGO_MEMINFO_H

#include "../../dr_clients/include/output.h"
#include <stdint.h>
#include <vector>

//directions
#define MEM_INPUT		0x1
#define MEM_OUTPUT		0x2

//linking mode
#define GREEDY			1
#define DYNAMIC_PROG	2

/* input typedef */
struct mem_input_t {

	std::string module;
	uint32_t pc;

	uint64_t mem_addr;
	bool write;
	uint32_t stride;
	uint32_t type;

};

/* output typedefs */
struct mem_info_t {

	uint32_t type;  /*mem type*/
	uint32_t direction; /* input / output */

	/* start and end instructions */
	uint64_t start;
	uint64_t end;

	/* stride */
	uint32_t prob_stride;
	std::vector<std::pair<uint32_t, uint32_t> > stride_freqs;

	std::vector < mem_info_t * > mem_infos; // if merged this would be filled

};


struct pc_mem_region_t {

	/* at least pc should be populated */
	std::string module;
	uint32_t pc;

	std::vector<pc_mem_region_t *> from_regions;  /* these are memory depedancies */
	std::vector<mem_info_t *> regions;
	std::vector<pc_mem_region_t *> to_regions;   /* these are memory dependencies */

};

/* pc mem region related functions - most use the mem_info_t for actual work */
void				 update_mem_regions(std::vector<pc_mem_region_t *> &pc_mems, mem_input_t * input); /* done */
void				 postprocess_mem_regions(std::vector<pc_mem_region_t *> &mem); /* done */
void				 print_mem_layout(std::ostream &file, std::vector<pc_mem_region_t *> &pc_mems); /* done */
bool				 random_dest_select(std::vector<pc_mem_region_t *> &pc_mems, std::string module, uint64_t app_pc, uint64_t * dest, uint32_t * stride); /* done */
void				 link_mem_regions(std::vector<pc_mem_region_t *> &pc_mems, uint32_t mode); /* done */
std::vector<mem_info_t *> extract_mem_regions(std::vector<pc_mem_region_t *> &pc_mems); /* done */

void				 populate_memory_dependancies(std::vector<pc_mem_region_t *> &regions);
void				 rank_pc_mems_from_highest(std::vector<pc_mem_region_t *> &pc_mems);

/* mem info related functions */
void				 update_mem_regions(std::vector<mem_info_t *> &mem_info, mem_input_t * input); /* done */
void				 postprocess_mem_regions(std::vector<mem_info_t *> &mem); /* done */
void				 print_mem_layout(std::ostream &file, std::vector<mem_info_t *> &mem); /* done */
bool				 random_dest_select(std::vector<mem_info_t *> &mem, uint64_t * dest, uint32_t * stride); /* done */
bool				 link_mem_regions(std::vector<mem_info_t *> &mem, uint32_t app_pc);
bool				 link_mem_regions_greedy(std::vector<mem_info_t *> &mem, uint32_t app_pc);

uint32_t get_stride(mem_info_t * mem, uint32_t dim, uint32_t total_dims);
uint32_t get_extents(mem_info_t * mem, uint32_t dim, uint32_t total_dims);
uint32_t get_number_dimensions(mem_info_t * mem);
void link_mem_regions_2(std::vector<pc_mem_region_t *> &pc_mems, uint32_t mode);
bool link_mem_regions_greedy_2(std::vector<mem_info_t *> &mem, uint32_t app_pc);


#endif