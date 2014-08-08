#include "filter_logic.h"
#include "moduleinfo.h"
#include "image_manipulation.h"
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;


void filter_based_on_freq(moduleinfo_t * head, image_t * image, uint32_t low_percentage){

	uint32_t size = image->height * image->width;
	uint32_t min_size = ( (double)size * ((double)low_percentage / 100.0) );
	std::cout << size << std::endl;
	std::cout << min_size << std::endl;

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