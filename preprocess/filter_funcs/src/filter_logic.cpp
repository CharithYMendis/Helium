#include "filter_logic.h"
#include "moduleinfo.h"
#include "imageinfo.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include "meminfo.h"
#include "memlayout.h"

using namespace std;


void filter_based_on_freq(moduleinfo_t * head, image_t * image, uint32_t min_threshold){

	uint32_t size = image->height * image->width;
	uint32_t min_size = ( (double)size * ((double)min_threshold / 100.0) );

	while (head != NULL){

		for (int i = 0; i < head->funcs.size(); i++){

			bool keep = false;
			funcinfo_t * func = head->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){
				bbinfo_t * bb = func->bbs[j];
				if (bb->freq >= min_size){
					keep = true;
					break;
				}
			}

			if (!keep){
				head->funcs.erase(head->funcs.begin() + i--);
			}
		}

		head = head->next;
	}


}

void filter_bbs_based_on_freq(moduleinfo_t * head, image_t * image, uint32_t min_threshold){

	uint32_t size = image->height * image->width;
	uint32_t min_size = ((double)size * ((double)min_threshold / 100.0));


	while (head != NULL){

		for (int i = 0; i < head->funcs.size(); i++){

			bool keep = false;
			funcinfo_t * func = head->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){
				bbinfo_t * bb = func->bbs[j];
				if (bb->freq < min_size){
					func->bbs.erase(func->bbs.begin() + j--);
				}
			}

		}

		head = head->next;
	}


}



/* doesn't work on mutually recursive functions - will add a check in future for that */
void filter_based_on_composition(moduleinfo_t * module){

	while (module != NULL){

		vector<uint32_t> removable;

		for (int i = 0; i < module->funcs.size(); i++){
			funcinfo_t * func = module->funcs[i];
			for (int j = 0; j < func->bbs.size(); j++){
				bbinfo_t * bb = func->bbs[j];
				for (int k = 0; k < bb->callers.size(); k++){
					uint32_t target = bb->callers[k]->target;
					for (int m = 0; m < module->funcs.size(); m++){
						if ( (i != m) && (module->funcs[m]->start_addr == target) ){
							removable.push_back(m);
							break;
						}
					}
				}
			}

		}

		std::vector<uint32_t>::iterator it;
		sort(removable.begin(), removable.end());
		it = unique(removable.begin(), removable.end());
		removable.resize(std::distance(removable.begin(), it));
		

		for (int i = 0; i < removable.size(); i++){
			module->funcs.erase(module->funcs.begin() + removable[i] - i);
		}


		module = module->next;
	}

}

void filter_mem_regions(vector<mem_info_t *> &mems, image_t * in_image, image_t * out_image, uint32_t min_threshold){

	/* use a conservative guess to filter out unnecessary memory regions */
	uint32_t in_image_area = in_image->width * in_image->height;
	uint32_t out_image_area = out_image->width * out_image->height;

	uint32_t min_area = min(in_image_area, out_image_area);


	for (int i = 0; i < mems.size(); i++){
		uint32_t size = (mems[i]->end - mems[i]->start) / (mems[i]->prob_stride);
		//if (size < 10){
		if (size <  (min_area * min_threshold / 100) ){
			mems.erase(mems.begin() + i--);
		}
	}



}


void filter_mem_regions(vector<pc_mem_region_t *> &pc_mems, image_t * in_image, image_t * out_image, uint32_t min_threshold){


	for (int i = 0; i < pc_mems.size(); i++){
		filter_mem_regions(pc_mems[i]->regions, in_image, out_image, min_threshold);
		if (pc_mems[i]->regions.size() == 0){
			pc_mems.erase(pc_mems.begin() + i--);
		}
	}

}





void filter_based_on_memory_dependancy(vector<pc_mem_region_t *> &pc_mems, moduleinfo_t * head){

	/* the filter should have regions which are incoming and outgoing */

	populate_memory_dependancies(pc_mems);

	vector<func_composition_t *> funcs = create_func_composition(pc_mems, head);

	for (int i = 0; i < funcs.size(); i++){

		uint32_t direction = 0;

		for (int j = 0; j < funcs[i]->region.size(); j++){
			pc_mem_region_t * region = funcs[i]->region[j];

			if (region->from_regions.size() > 0){
				direction |= 0x1;
			}
			if (region->to_regions.size() > 0){
				direction |= 0x2;
			}

		}

		if (direction != 0x03){
			moduleinfo_t * module = find_module(head, funcs[i]->module_name);
			for (int j = 0; j < module->funcs.size(); j++){
				if (module->funcs[j]->start_addr == funcs[i]->func_addr){
					module->funcs.erase(module->funcs.begin() + j--);
				}
			}
		}
	}

}