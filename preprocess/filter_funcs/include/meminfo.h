#ifndef _MEMINFO_H
#define _MEMINFO_H

#include <stdint.h>

struct mem_region_t {

	uint32_t type;  /*mem type*/
	uint32_t direction; /* input / output */

	/* start and end instructions */
	uint64_t start;
	uint64_t end;

	/* stride */
	vector<pair<uint32_t, uint32_t> > stride_freqs;
};


struct pc_meminfo_t {

	uint32_t pc;
	vector<mem_region_t *> regions;

};



#endif