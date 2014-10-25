/*given a file and some other parameters this will build the expression tree*/
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "fileparser.h"
#include "build_tree.h"
#include "defines.h"
#include "expression_tree.h"
#include "canonicalize.h"
#include "memregions.h"
#include "print_dot.h"

using namespace std;



void cinstr_convert_reg(cinstr_t * instr){

	for (int i = 0; i < instr->num_srcs; i++){
		if ((instr->srcs[i].type == REG_TYPE) && (instr->srcs[i].value > DR_REG_ST7)) instr->srcs[i].value += 8;
		reg_to_mem_range(&instr->srcs[i]);
	}

	for (int i = 0; i < instr->num_dsts; i++){
		if (instr->dsts[i].type == REG_TYPE && (instr->dsts[i].value > DR_REG_ST7)) instr->dsts[i].value += 8;
		reg_to_mem_range(&instr->dsts[i]);
	}

}


void print_vector(vector<pair<uint32_t,uint32_t> > lines, vector<disasm_t *> disasm){
	for (int i = 0; i < lines.size(); i++){
		vector<string> disasm_string = get_disasm_string(disasm,lines[i].second);
		
		cout << lines[i].first << " - " << lines[i].second;
		if (disasm_string.size()>0){
			cout << " " << disasm_string[0];
		}
		cout << endl;
 	}
}


Expression_tree * create_tree_for_dest(uint64_t dest, uint32_t stride, ifstream &instrace_file, vector<uint32_t> start_points,
	int32_t start_trace, int32_t end_trace, vector<disasm_t *> disasm){

	DEBUG_PRINT(("building tree for %llx...\n", dest), 2);

	instrace_file.clear();
	instrace_file.seekg(0, instrace_file.beg);

	if (start_trace == FILE_BEGINNING){
		start_trace = go_to_line_dest(instrace_file, dest, stride);
	}

	instrace_file.clear();
	instrace_file.seekg(0, instrace_file.beg);

	if (start_trace == 0) return NULL;

	Expression_tree * conc_tree = new Expression_tree();

	if (end_trace == FILE_ENDING){
		for (int i = 0; i < start_points.size(); i++){
			if (start_trace <= start_points[i]){
				end_trace = start_points[i];
				break;
			}
		}
	}

	build_tree(dest, start_trace, end_trace, instrace_file, conc_tree, disasm);
	//order_tree(conc_tree->get_head());

	instrace_file.clear();
	instrace_file.seekg(0, instrace_file.beg);

	return conc_tree;

}


vector<uint32_t> get_instrace_startpoints(vec_cinstr &instrs, uint32_t pc){

	vector<uint32_t> start_points;
	int line = 0;

	for (int i = 0; i < instrs.size(); i++){
		cinstr_t * instr = instrs[i].first;
		if (instr != NULL){
			if (instr->pc == pc){
				start_points.push_back(i + 1);
			}
		}
	}

	return start_points;

}

pair<int32_t, int32_t> get_start_and_end_points(vector<uint32_t> start_points,uint64_t dest,uint32_t stride, uint32_t start_trace, uint32_t end_trace,vec_cinstr &instrs){

	int32_t start = start_trace;
	int32_t end = end_trace;

	if (start_trace == FILE_BEGINNING){
		bool found = false;
		for (int i = 0; i < instrs.size(); i++){
			for (int j = 0; j < instrs[i].first->num_dsts; i++){
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


void build_tree(uint64 destination, uint32_t stride, vector<uint32_t> start_points, int start_trace, int end_trace, Expression_tree * tree, vec_cinstr &instrs, vector<disasm_t *> disasm){

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
		ASSERT_MSG((start_trace < instrs.size()),("ERROR: trace start should be within the limits of the file"));
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


	ASSERT_MSG((instr != NULL), ("ERROR: you have given a line no beyond this file\n"));

	if (debug && debug_level >= 2){
		if (instrs[curpos].second != NULL){
			cout << *instrs[curpos].second << endl;
		}
	}

	if (instrs[curpos].second != NULL){
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, *instrs[curpos].second , curpos);
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
		if (instrs[curpos].second  != NULL){
			tree->update_frontier(&rinstr[i], instr->pc, *instrs[curpos].second, curpos);
		}
		else{
			tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
		}
	}

	curpos++;

	vector<pair<uint32_t, uint32_t> > lines;

	//do the rest of expression tree building
	for(; curpos < end_trace; curpos++) {

		instr = instrs[curpos].first; 
		rinstr = NULL;
		DEBUG_PRINT(("->line - %d\n", curpos), 3);
		if (instr != NULL){


			if (debug && debug_level >= 4){
				if (instrs[curpos].second != NULL){
					cout << *instrs[curpos].second << endl;
				}
			}

			if (instrs[curpos].second != NULL){
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, *instrs[curpos].second, curpos);
			}
			else{
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
			}

			if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs); }
			bool updated = false;
			for (int i = no_rinstrs - 1; i >= 0; i--){
				if (instrs[curpos].second != NULL){
					updated = tree->update_frontier(&rinstr[i], instr->pc, *instrs[curpos].second, curpos);
				}
				else{
					updated = tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
				}
				if (updated) lines.push_back(make_pair(curpos, instr->pc));
			}
		}

		delete[] rinstr;

	}

	DEBUG_PRINT(("build_tree(concrete) - done\n"), 2);
	//print_vector(lines,disasm);
	order_tree(tree->get_head());
	


}


void build_tree(uint64 destination, int start_trace, int end_trace, ifstream &file, Expression_tree * tree, vector<disasm_t *> disasm){

	DEBUG_PRINT(("build_tree(concrete)....\n"), 2);

	if (end_trace != FILE_ENDING)
		ASSERT_MSG((end_trace >= start_trace), ("trace end should be greater than the trace start\n"));

	uint curpos = 0;
	
	if (start_trace != FILE_BEGINNING){
		go_to_line(start_trace, file);
		curpos = start_trace - 1;
	}

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;


	//now we need to read the next line and start from the correct destination
	bool dest_present = false;
	int index = -1;

	//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well. can have an assert for this

	instr = get_next_from_ascii_file(file);
	

	ASSERT_MSG((instr != NULL), ("ERROR: you have given a line no beyond this file\n"));
	cinstr_convert_reg(instr);

	vector<string> string_disasm = get_disasm_string(disasm, instr->pc);
	if (debug && debug_level >= 2){
		for (int i = 0; i < string_disasm.size(); i++){
			cout << string_disasm[i] << endl;
		}
	}

	curpos++;
	if (string_disasm.size()>0){
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, string_disasm[0], curpos);
	}
	else{
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
	}

	

	if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs);}
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
		if (string_disasm.size() > 0){
			tree->update_frontier(&rinstr[i], instr->pc, string_disasm[0],curpos);
		}
		else{
			tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
		}
	}

	vector<pair<uint32_t,uint32_t> > lines;

	//do the rest of expression tree building
	while (!file.eof()){
		instr = get_next_from_ascii_file(file);
		rinstr = NULL;
		//print_cinstr(instr);
		curpos++;
		DEBUG_PRINT(("->line - %d\n", curpos), 3);
		if (instr != NULL){
			cinstr_convert_reg(instr);

			vector<string> string_disasm = get_disasm_string(disasm, instr->pc);
			if (debug && debug_level >= 4){
				for (int i = 0; i < string_disasm.size(); i++){
					cout << string_disasm[i] << endl;
				}
			}

			if (string_disasm.size() > 0){
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, string_disasm[0], curpos);
			}
			else{
				rinstr = cinstr_to_rinstrs(instr, no_rinstrs, "not captured\n", curpos);
			}
			
			if (debug_level >= 4){ print_rinstrs(rinstr, no_rinstrs); }
			bool updated = false;
			for (int i = no_rinstrs - 1; i >= 0; i--){
				if (string_disasm.size() > 0){
					updated = tree->update_frontier(&rinstr[i], instr->pc,string_disasm[0], curpos);
				}
				else{
					updated = tree->update_frontier(&rinstr[i], instr->pc, "not captured\n", curpos);
				}
				if(updated) lines.push_back(make_pair(curpos,instr->pc));
			}
		}

		if ( (end_trace != FILE_ENDING) && (curpos == end_trace)){
			break;
		}

		delete instr;
		delete[] rinstr;

	}

	DEBUG_PRINT(("build_tree(concrete) - done\n"), 2);
	//print_vector(lines,disasm);
	order_tree(tree->get_head());


}


bool are_conc_trees_similar(Node * first, Node * second){


	if ((first->symbol->type != second->symbol->type) || (first->operation != second->operation)){
		return false;
	}


	/*check whether all the nodes have same number of sources*/
	if (first->srcs.size() != second->srcs.size()){
		return false;
	}
	

	/* recursively check whether the src nodes are similar*/
	bool ret = true;
	for (int i = 0; i < first->srcs.size(); i++){
		if (!are_conc_trees_similar(first->srcs[i], second->srcs[i])){
			ret = false;
			break;
		}
	}

	return ret;

}

vector< vector <Expression_tree *> > cluster_trees(vector<mem_regions_t *> mem_regions,vector<uint32_t> start_points,vec_cinstr &instrs, vector<disasm_t *> disasm, string output_folder){

	mem_regions_t * mem = get_random_output_region(mem_regions);

	vector<Expression_tree *> trees;

	for (uint64 i = mem->start; i < mem->end; i++){
		DEBUG_PRINT(("building tree for location %llx - %u\n", i, i - mem->start), 2); 
		Expression_tree * tree = new Expression_tree();
		tree->number = i - mem->start;
		build_tree(i, mem->bytes_per_pixel, start_points, FILE_BEGINNING, FILE_ENDING, tree, instrs, disasm);
		trees.push_back(tree);
	}

	DEBUG_PRINT(("clustering trees\n"), 2);
	/* cluster based on the similarity */
	vector< vector<Expression_tree *> > clustered_trees = categorize_trees(trees);
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

vector< vector<Expression_tree *> >  categorize_trees(vector<Expression_tree * > trees){

	vector< vector<Expression_tree * > > categorized_trees;

	while (!trees.empty()){
		vector< Expression_tree* > similar_trees;
		Node * first = trees[0]->get_head();
		similar_trees.push_back(trees[0]);
		trees.erase(trees.begin());
		for (int i = 0; i < trees.size(); i++){
			if (are_conc_trees_similar(first, trees[i]->get_head())){
				similar_trees.push_back(trees[i]);
				trees.erase(trees.begin() + i--);
			}
		}
		categorized_trees.push_back(similar_trees);
	}

	return categorized_trees;

}

vector<Expression_tree *> get_similar_trees(vector<mem_regions_t *> image_regions, uint32_t seed, uint32_t * stride, vec_cinstr &instrs,
	vector<uint32_t> start_points, int32_t end_trace, vector<disasm_t *> disasm){

	vector<Expression_tree *> nodes;

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
	Expression_tree * main_tree = new Expression_tree();
	build_tree(mem_location, random_mem_region->bytes_per_pixel, start_points, FILE_BEGINNING,end_trace,main_tree,instrs,disasm);
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
				Expression_tree * created_tree = new Expression_tree();
				build_tree(mem_location, random_mem_region->bytes_per_pixel, start_points, FILE_BEGINNING, end_trace, created_tree, instrs, disasm);
			
				print_node_tree(created_tree->get_head(), cout);
				cout << endl;
				print_node_tree(main_tree->get_head(), cout);
				cout << endl;

				if (are_conc_trees_similar(main_tree->get_head(), created_tree->get_head())){
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
