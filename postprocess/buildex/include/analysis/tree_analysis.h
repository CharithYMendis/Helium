#ifndef _TREE_ANALYSIS
#define _TREE_ANALYSIS

#include <fstream>
#include <iostream>
#include <vector>
#include <stdint.h>

#include "trees/trees.h"
#include "analysis/staticinfo.h"
#include "meminfo.h"


#define FILE_BEGINNING  -2
#define FILE_ENDING		-1

/* conc_tree building */

Conc_Tree * build_conc_tree(uint64_t destination, 
				uint32_t stride, 
				std::vector<uint32_t> start_points, 
				int start_trace, 
				int end_trace, 
				Conc_Tree * tree,
				vec_cinstr &instrs,
				std::vector<mem_regions_t *> &regions);
	

void build_conc_tree(uint64_t destination, 
				uint32_t stride, 
				std::vector<uint32_t> start_points, 
				int start_trace, 
				int end_trace, 
				Conc_Tree * tree,
				std::ifstream &file,
				std::vector<mem_regions_t *> &regions);
	
void build_conc_trees_for_conditionals(
				std::vector<uint32_t> start_points, 
				Conc_Tree * tree, 
				vec_cinstr &instrs,
				std::vector<mem_regions_t *> &regions);

/* tree clustering and other categorizing */				
				
std::vector< std::vector<Conc_Tree *> >  categorize_trees(
				std::vector<Conc_Tree * > trees);
				
std::vector<Conc_Tree *> get_similar_trees(
				std::vector<mem_regions_t *> image_regions,
				std::vector<mem_regions_t *> total_regions,	
				uint32_t seed, 
				uint32_t * stride, 
				std::vector<uint32_t> start_points,
				int32_t start_trace,
				int32_t end_trace, 
				vec_cinstr &instrs);

std::vector< std::vector <Conc_Tree *> > cluster_trees
				(std::vector<mem_regions_t *> mem_regions, 
				std::vector<mem_regions_t *> total_regions,
				std::vector<uint32_t> start_points, 
				vec_cinstr &instrs, 
				std::string output_folder);

/* abs tree building */	

struct Abs_Tree_Charac{

	Abs_Tree * tree;
	bool is_recursive;
	std::vector< std::pair< int32_t, int32_t > > extents;
	Abs_Node * red_node;


};

std::vector<Abs_Tree_Charac *> build_abs_trees(
			std::vector< std::vector< Conc_Tree *> > clusters, 
			std::string folder, 
			uint32_t no_trees, 
			std::vector<mem_regions_t *> total_regions, 
			uint32_t skip,
			std::vector<pc_mem_region_t *> &pc_mem);




#endif