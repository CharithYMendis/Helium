/*given a file and some other parameters this will build the expression tree*/
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "analysis/tree_analysis.h"
#include "common_defines.h"
#include "utility/fileparser.h"
#include "analysis/x86_analysis.h"
#include "utility/defines.h"

using namespace std;

void update_jump_conditionals(Conc_Tree * tree,
	vec_cinstr &instrs,
	uint32_t pos);

/************************************************************************/
/*  Tree building routines                                              */
/************************************************************************/

pair<int32_t, int32_t> get_start_and_end_points(vector<uint32_t> start_points, uint64_t dest, uint32_t stride, uint32_t start_trace, uint32_t end_trace, vec_cinstr &instrs){

	int32_t start = start_trace;
	int32_t end = end_trace;

	if (start_trace == FILE_BEGINNING){
		bool found = false;
		for (int i = 0; i < instrs.size(); i++){
			for (int j = 0; j < instrs[i].first->num_dsts; j++){
				if (instrs[i].first->dsts[j].value == dest &&  instrs[i].first->dsts[j].width == stride){
					start = i + 1;
					found = true;
					break;
				}
			}
			if (found) break;
		}
		ASSERT_MSG(found, ("ERROR: dest not found within the instruction trace\n"));
	}



	if (end_trace == FILE_ENDING){
		for (int i = 0; i < start_points.size(); i++){
			if (start <= start_points[i]){
				end = start_points[i];
			}
		}
	}



	return make_pair(start, end);


}

/* conc tree building routine */
void build_conc_tree(uint64_t destination,
					uint32_t stride,
					std::vector<uint32_t> start_points,
					int start_trace,
					int end_trace,
					Conc_Tree * tree,
					vec_cinstr &instrs){

	DEBUG_PRINT(("build_tree(concrete)....\n"), 2);

	uint curpos;

	pair<int32_t, int32_t> points = get_start_and_end_points(start_points, destination, stride, start_trace, end_trace, instrs);


	cout << points.first << " " << points.second << endl;

	start_trace = points.first;
	end_trace = points.second;


	if (end_trace != FILE_ENDING){
		ASSERT_MSG((end_trace >= start_trace), ("ERROR: trace end should be greater than the trace start\n"));
	}
	else{
		end_trace = instrs.size();
	}

	if (start_trace != FILE_BEGINNING){
		ASSERT_MSG((start_trace < instrs.size()), ("ERROR: trace start should be within the limits of the file"));
		start_trace = start_trace - 1;
	}
	else{
		start_trace = 0;
	}

	curpos = start_trace;


	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;


	//now we need to read the next line and start from the correct destination
	bool dest_present = false;
	int index = -1;

	//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well. can have an assert for this

	instr = instrs[curpos].first;
	//cout << instr->pc << endl;


	ASSERT_MSG((instr != NULL), ("ERROR: you have given a line no beyond this file\n"));

	string string_disasm = instrs[curpos].second->disassembly;
	if (debug && debug_level >= 3){
		cout << string_disasm << endl;
	}


	if (instrs[curpos].second != NULL){
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, instrs[curpos].second->disassembly, curpos);
	}
	else{
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
	}


	if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs); }

	for (int i = no_rinstrs - 1; i >= 0; i--){
		if (rinstr[i].dst.value == destination){
			ASSERT_MSG((rinstr[i].dst.type != IMM_FLOAT_TYPE) && (rinstr[i].dst.type != IMM_INT_TYPE), ("ERROR: dest cannot be an immediate\n"));
			index = i;
			dest_present = true;
			break;
		}
	}

	ASSERT_MSG((dest_present == true) && (index >= 0), ("ERROR: couldn't find the dest to start trace\n")); //we should have found the destination

	//do the initial processing
	for (int i = index; i >= 0; i--){
		if (instrs[curpos].second != NULL){
			tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos);
		}
		else{
			tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, NULL, curpos);
		}
	}

	curpos++;

	vector<pair<uint32_t, uint32_t> > lines;

	//do the rest of expression tree building
	for (; curpos < end_trace; curpos++) {

		instr = instrs[curpos].first;
		rinstr = NULL;
		DEBUG_PRINT(("->line - %d\n", curpos), 3);
		if (instr != NULL){

			if (debug && debug_level >= 4){
				if (instrs[curpos].second != NULL){
					cout << instrs[curpos].second->disassembly << endl;
				}
			}

			if (instrs[curpos].second != NULL){
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, instrs[curpos].second->disassembly, curpos);
			}
			else{
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
			}

			if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs); }
			bool updated = false;
			bool affected = false;
			for (int i = no_rinstrs - 1; i >= 0; i--){
				if (instrs[curpos].second != NULL){
					updated = tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos);
				}
				else{
					updated = tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, NULL, curpos);
				}

				if (updated){
					affected = true;
					lines.push_back(make_pair(curpos, instr->pc));
				}
			}

			
			if (affected){  /* that is this instr affects the frontier */

				update_jump_conditionals(tree, instrs, curpos);
			}
		}

		delete[] rinstr;
	}

	DEBUG_PRINT(("build_tree(concrete) - done\n"), 2);
	//print_vector(lines,disasm);
	//remove_po_node(tree->get_head(), tree->get_head(), NULL, 0);
	//order_tree(tree->get_head());
	tree->canonicalize_tree();

}

/* this updates conditional statements that are affecting a particular tree */
void update_jump_conditionals(Conc_Tree * tree,
							  vec_cinstr &instrs, 
							  uint32_t pos){

	cinstr_t * instr = instrs[pos].first;
	Static_Info *  static_info = instrs[pos].second;
	vector< pair<Jump_Info *, bool> > input_dep_conditionals = static_info->conditionals;
	
	DEBUG_PRINT(("checking for conditionals attached - instr %d\n", instr->pc), 3);

	for (int i = 0; i < input_dep_conditionals.size(); i++){

		Jump_Info * jump_info = input_dep_conditionals[i].first;
		bool taken = input_dep_conditionals[i].second;
		//get the line number for this jump info
		uint32_t line_cond = 0;
		uint32_t line_jump = 0;

		cout << jump_info->cond_pc << endl;

		for (int j = pos; j < instrs.size(); j++){ //changed pos + 1 to pos
			cinstr_t * instr_j = instrs[j].first;
			if (instr_j->pc == jump_info->jump_pc){
				line_jump = j;
			}
			if (instr_j->pc == jump_info->cond_pc){
				line_cond = j;
				break;
			}
		}

		ASSERT_MSG((line_cond != 0), ("ERROR: couldn't find the conditional instruction\n"));
		ASSERT_MSG((line_jump != 0), ("ERROR: couldn't find the jump instruction\n"));

		//added
		bool actual_taken = is_branch_taken(instrs[line_jump].first->opcode, instrs[line_jump].first->eflags);

		ASSERT_MSG((actual_taken == taken), ("ERROR: branch direction information is inconsistent\n"));

		bool is_there = false;
		for (int j = 0; j < tree->conditionals.size(); j++){
			if ((jump_info == tree->conditionals[i]->jump_info) &&
				(line_cond == tree->conditionals[i]->line_cond) &&
				(taken == tree->conditionals[i]->taken) &&
				(line_jump == tree->conditionals[i]->line_jump)){
				is_there = true;
				break;
			}
		}

		if (!is_there){
			Conc_Tree::conditional_t * condition = new Conc_Tree::conditional_t;
			condition->jump_info = jump_info;
			condition->line_cond = line_cond;
			condition->line_jump = line_jump;
			condition->taken = taken;
			tree->conditionals.push_back(condition);
		}

	}


}


void build_conc_tree(uint64_t destination,
	uint32_t stride,
	std::vector<uint32_t> start_points,
	int start_trace,
	int end_trace,
	Conc_Tree * tree,
	std::ifstream &file){

}


void build_conc_trees_for_conditionals(
	std::vector<uint32_t> start_points,
	Conc_Tree * tree,
	vec_cinstr &instrs){


	for (int i = 0; i < tree->conditionals.size(); i++){

		Jump_Info * info = tree->conditionals[i]->jump_info;
		uint32_t line_cond = tree->conditionals[i]->line_cond;
		uint32_t line_jump = tree->conditionals[i]->line_jump;

		cinstr_t * instr = instrs[line_cond].first;

		vector<Conc_Tree *> cond_trees;

		if (instr->opcode == OP_cmp){

			/* cmp x1, x2 - 2 srcs and no dest */

			for (int j = 0; j < instr->num_srcs; j++){
				//now build the trees
				if (instr->srcs[j].type != IMM_INT_TYPE && instr->srcs[j].type != IMM_FLOAT_TYPE){

					/* find the dst when these srcs are written */
					uint32_t dst_line = 0;
					for (int k = line_cond; k < instrs.size(); k++){
						cinstr_t * temp = instrs[k].first;
						bool found = false;
						for (int m = 0; m < temp->num_dsts; m++){
							if (temp->dsts[m].type == instr->srcs[j].type && temp->dsts[m].value == instr->srcs[j].value){
								dst_line = k;
								found = true;
								break;
							}
						}
						if (found) break;
					}
					ASSERT_MSG((dst_line != 0), ("ERROR: couldn't find the conditional destination\n"));

					Conc_Tree * cond_tree = new Conc_Tree();
					build_conc_tree(instr->srcs[j].value, instr->srcs[j].width, start_points, dst_line + 1, FILE_ENDING, cond_tree, instrs);
					cond_trees.push_back(cond_tree);

				}
				else{

					Conc_Tree * cond_tree = new Conc_Tree();
					Node * head;
					if (instr->srcs[j].type == IMM_INT_TYPE){
						head = new Conc_Node(instr->srcs[j].type, instr->srcs[j].value, instr->srcs[j].width, 0.0);
					}
					else{
						head = new Conc_Node(instr->srcs[j].type, 0, instr->srcs[j].width, instr->srcs[j].float_value);
					}
					cond_tree->set_head(head);
					cond_trees.push_back(cond_tree);
				}


			}

			/* now create the tree */
			Conc_Node * node = new Conc_Node(REG_TYPE, 150, 4, 0.0);
			node->operation = dr_logical_to_operation(instrs[line_jump].first->opcode);

			for (int j = 0; j < cond_trees.size(); j++){
				cond_trees[j]->change_head_node();
			}

			/* merge the trees together */
			Node * left = cond_trees[0]->get_head();
			Node * right = cond_trees[1]->get_head();

			node->srcs.push_back(left);
			left->prev.push_back(node);
			left->pos.push_back(0);

			node->srcs.push_back(right);
			right->prev.push_back(node);
			right->pos.push_back(1);

			/* Now we need to add the computational node */
			Node * comp_node = tree->get_head();
			Node * new_node = new Conc_Node(comp_node->symbol->type, comp_node->symbol->value, comp_node->symbol->width, 0.0);

			new_node->srcs.push_back(node);
			node->prev.push_back(new_node);
			node->pos.push_back(0);

			Conc_Tree * new_cond_tree = new Conc_Tree();

			new_cond_tree->set_head(new_node);

			tree->conditionals[i]->tree = new_cond_tree;

		}
		else{
			ASSERT_MSG((false), ("ERROR: conditionals for other instructions are not done\n"));
		}

	}

}


/************************************************************************/
/*  Tree clustering routines				                            */
/************************************************************************/

std::vector<Conc_Tree *> get_similar_trees(
	std::vector<mem_regions_t *> image_regions,
	uint32_t seed,
	uint32_t * stride,
	std::vector<uint32_t> start_points,
	int32_t start_trace,
	int32_t end_trace,
	vec_cinstr &instrs){


	vector<Conc_Tree *> nodes;

	/*ok we need find a set of random locations */
	mem_regions_t * random_mem_region = get_random_output_region(image_regions);
	uint64_t mem_location = get_random_mem_location(random_mem_region, seed);
	DEBUG_PRINT(("random mem location we got - %llx\n", mem_location), 1);
	*stride = random_mem_region->bytes_per_pixel;

	vector<uint64_t> nbd_locations;
	vector<int> base = get_mem_position(random_mem_region, mem_location);
	nbd_locations.push_back(mem_location);

	cout << "base : " << endl;
	for (int j = 0; j < base.size(); j++){
		cout << base[j] << ",";
	}
	cout << endl;

	/* build the expression tree for this node */
	Conc_Tree * main_tree = new Conc_Tree();
	build_conc_tree(mem_location, *stride, start_points, FILE_BEGINNING, end_trace, main_tree, instrs);
	nodes.push_back(main_tree);

	uint32_t nodes_needed = random_mem_region->dimensions + 2;
	int32_t sign = 1;
	int i = 0;

	vector<uint32_t> dims;
	for (int i = 0; i < random_mem_region->dimensions; i++){
		dims.push_back(0);
	}

	while (true) {

		vector<int> offset;
		uint32_t affected_dim = i % random_mem_region->dimensions;
		i++;

		if (nodes.size() >= nodes_needed) break;

		bool cont = false;
		for (int j = 0; j < affected_dim; j++){
			if (dims[j] < dims[affected_dim]){
				cont = true;
				break;
			}
		}

		for (int j = 0; j < dims.size(); j++){
			cout << dims[j] << ",";
		}
		cout << endl;

		if (cont){
			continue;
		}


		uint32_t next_value = 0;

		while (true){
			vector<int32_t> offset;
			for (int j = 0; j < base.size(); j++){
				if (j == affected_dim) { offset.push_back(sign); }
				else { offset.push_back(next_value++); }
			}

			cout << "searching for tree: " << nodes.size() + 1 << endl;
			for (int j = 0; j < base.size(); j++){
				cout << dec << (base[j] + offset[j]) << ",";
			}
			cout << endl;

			bool success;
			mem_location = get_mem_location(base, offset, random_mem_region, &success);

			if (success){
				Conc_Tree * created_tree = new Conc_Tree();
				build_conc_tree(mem_location, random_mem_region->bytes_per_pixel, start_points, FILE_BEGINNING, end_trace, created_tree, instrs);

				created_tree->print_tree(cout);
				cout << endl;
				main_tree->print_tree(cout);
				cout << endl;

				if (main_tree->are_trees_similar(created_tree)){
					nodes.push_back(created_tree);
					dims[affected_dim]++;
					sign = -1 * sign * (int)ceil((i + 1) / (double)random_mem_region->dimensions);
					cout << ceil((i + 1) / (double)random_mem_region->dimensions) << endl;
					cout << sign << endl;
					cout << i << endl;
					break;
				}
				else{
					delete created_tree;
				}
			}
			else{
				sign = -1 * sign * (int)ceil((i + 1) / (double)random_mem_region->dimensions);
				cout << sign << endl;
				break;
			}

		}


	}

	return nodes;


}


std::vector< std::vector <Conc_Tree *> > cluster_trees
		(std::vector<mem_regions_t *> mem_regions,
		std::vector<uint32_t> start_points,
		vec_cinstr &instrs,
		std::string output_folder){


	mem_regions_t * mem = get_random_output_region(mem_regions);

	vector<Conc_Tree *> trees;

	/*BUG - incrementing by +1 is not general; should increment by the stride */
	for (uint64 i = mem->start; i < mem->end; i++){
		DEBUG_PRINT(("building tree for location %llx - %u\n", i, i - mem->start), 2);
		Conc_Tree * tree = new Conc_Tree();
		tree->tree_num = i - mem->start;
		build_conc_tree(i, mem->bytes_per_pixel, start_points, FILE_BEGINNING, FILE_ENDING, tree, instrs);
		build_conc_trees_for_conditionals(start_points, tree, instrs);			
		trees.push_back(tree);
	}

	DEBUG_PRINT(("clustering trees\n"), 2);
	
	/* cluster based on the similarity */
	vector< vector<Conc_Tree *> > clustered_trees = categorize_trees(trees);
	
	/* cluster results */
	cout << "number of tree clusters : " << clustered_trees.size() << endl;

	/*for (int i = 0; i < clustered_trees.size(); i++){
	uint no_nodes = number_tree_nodes(clustered_trees[i][1]->get_head());
	DEBUG_PRINT(("printing to dot file...\n"), 2);
	ofstream conc_file(output_folder + "_conctree_" + to_string(i) + ".dot", ofstream::out);
	print_to_dotfile(conc_file, clustered_trees[i][1]->get_head(), no_nodes, 0);
	}*/

	/* get the divergence points */

	return clustered_trees;


}

/* categorize the trees based on */
vector< vector<Conc_Tree *> >  categorize_trees(vector<Conc_Tree * > trees){

	vector< vector<Conc_Tree * > > categorized_trees;

	while (!trees.empty()){
		vector<Conc_Tree* > similar_trees;
		similar_trees.push_back(trees[0]);
		trees.erase(trees.begin());
		for (int i = 0; i < trees.size(); i++){
			if (similar_trees[0]->are_trees_similar(trees[i])){
				similar_trees.push_back(trees[i]);
				trees.erase(trees.begin() + i--);
			}
		}
		categorized_trees.push_back(similar_trees);
	}

	return categorized_trees;

}

/************************************************************************/
/*  Building Abs_Trees                                                  */
/************************************************************************/

Abs_Tree* abstract_the_trees(vector<Conc_Tree *> cluster, uint32_t no_trees, uint32_t skip,
	vector<mem_regions_t *> &total_regions, vector<pc_mem_region_t *> &pc_mem){

	/* build the abs trees for the main computational tree and abstract it */
	vector<Abs_Tree *> abs_trees;
	vector<Tree *> trees;

	for (int j = 0; j < skip * no_trees; j+= skip){
		Abs_Tree  * abs_tree = new Abs_Tree();
		/* parameter identification here -> please check */
		//identify_parameters(cluster[j]->head, pc_mem);
		abs_tree->build_abs_tree_unrolled(cluster[j], total_regions);
		abs_trees.push_back(abs_tree);
		trees.push_back(abs_tree);
	}

	bool similar = Tree::are_trees_similar(trees);
	if (similar){

		DEBUG_PRINT(("the trees are similar, now getting the algebric filter....\n"), 1);
		Comp_Abs_Tree * comp_tree = new Comp_Abs_Tree();
		comp_tree->build_compound_tree_unrolled(abs_trees);
		comp_tree->abstract_buffer_indexes();
		Abs_Tree * final_tree = comp_tree->compound_to_abs_tree();
		return final_tree;
	}
	else{
		DEBUG_PRINT(("the trees are not similar; please check\n"), 1);
		return NULL;
	}


}




vector< pair<Abs_Tree *, bool > > get_conditional_trees(vector<Conc_Tree *> clusters, 
										uint32_t no_trees, 
										vector<mem_regions_t * > total_regions, 
										uint32_t skip, 
										vector<pc_mem_region_t * > &pc_mem){

	vector< pair<Abs_Tree *, bool > > cond_abs_trees;
	uint32_t number_conditionals = clusters[0]->conditionals.size();

	for (int i = 1; i < no_trees; i++){
		Conc_Tree * tree = clusters[i];
		ASSERT_MSG((tree->conditionals.size() == number_conditionals), ("ERROR: number of conditionals in similar trees different\n"));
	}

	for (int i = 0; i < clusters[0]->conditionals.size(); i++){
		Abs_Tree *  per_cond_tree;
		vector<Conc_Tree *> trees;
		for (int k = 0; k < skip * no_trees; k += skip){
			/* add the computational tree head (computational node should have been already put out) */
			trees.push_back(clusters[k]->conditionals[i]->tree);
		}

		Abs_Tree * cond_abs_tree = abstract_the_trees(trees, no_trees, 1, total_regions, pc_mem);
		
		cond_abs_trees.push_back(make_pair(cond_abs_tree,clusters[0]->conditionals[i]->taken));
	}

	return cond_abs_trees;

}

vector<Abs_Tree_Charac *> build_abs_trees(
	std::vector< std::vector< Conc_Tree *> > clusters,
	std::string folder,
	uint32_t no_trees,
	std::vector<mem_regions_t *> total_regions,
	uint32_t skip,
	std::vector<pc_mem_region_t *> &pc_mem){

	/* sanity print */
	for (int i = 0; i < clusters.size(); i++){

		Conc_Tree * tree = clusters[i][0];
		ofstream conc_file(folder + "_conc_comp_" + to_string(i) + ".dot", ofstream::out);
		tree->print_dot(conc_file,"cluster_conc",i);
		
		for (int j = 0; j < tree->conditionals.size(); j++){

			Conc_Tree * cond_tree = tree->conditionals[j]->tree;
			ofstream conc_file(folder + "_conc_cond_" + to_string(i) + "_" + to_string(j) + ".dot", ofstream::out);
			cond_tree->print_dot(conc_file,"cluster_conc_cond",j);
			
		}
	}

	
	vector<Abs_Tree_Charac *> tree_characs;

	/* for each cluster */
	for (int i = 0; i < clusters.size(); i++){

		/* get the abstract tree for the computational path */
		Abs_Tree * abs_tree = abstract_the_trees(clusters[i], no_trees, skip, total_regions, pc_mem);
		/* get the conditional trees*/
		abs_tree->conditional_trees = get_conditional_trees(clusters[i], no_trees, total_regions, skip, pc_mem);


		/* check if the trees are recursive*/
		if (abs_tree->is_tree_recursive()){  /* if yes, then try to get the reduction domain */

			ASSERT_MSG(false, ("build_abs_tree: not yet implemented\n"));
		}
		else{ /* if no, then populate pure functions */

			Abs_Tree_Charac * charac = new Abs_Tree_Charac();
			charac->tree = abs_tree;
			charac->is_recursive = false;
			tree_characs.push_back(charac);

		}
	
	}


	return tree_characs;


}
