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

#include "utility/print_helper.h"

#include "utilities.h"

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
		//ASSERT_MSG(found, ("ERROR: dest not found within the instruction trace\n"));
	}



	if (end_trace == FILE_ENDING){
		for (int i = 0; i < start_points.size(); i++){
			if (start <= start_points[i]){
				end = start_points[i];
				break;
			}
		}
	}



	return make_pair(start, end);


}


void remove_reg_leaves(Conc_Tree * tree){

	//DEBUG_PRINT(("WARNING: unimplemented\n"), 1);

}

void build_conc_tree_helper(
	int start_trace,
	int end_trace,
	Conc_Tree * tree,
	vec_cinstr &instrs,
	vector<mem_regions_t *> &regions,
	vector<Func_Info_t *> &func_info){

	int32_t no_rinstrs;
	cinstr_t * instr;
	rinstr_t * rinstr;
	int32_t curpos = start_trace;

	vector<pair<uint32_t, uint32_t> > lines;

	//do the rest of expression tree building
	for (; curpos < end_trace; curpos++) {

		instr = instrs[curpos].first;
		rinstr = NULL;
		DEBUG_PRINT(("->line - %d\n", curpos), 4);
		DEBUG_PRINT(("%s\n", instrs[curpos].second->disassembly.c_str()), 4);
		rinstr = cinstr_to_rinstrs(instr, no_rinstrs, instrs[curpos].second->disassembly, curpos);
		if (debug_level >= 4){ print_rinstrs(log_file,rinstr, no_rinstrs); }

		bool updated = false;
		bool affected = false;
		for (int i = no_rinstrs - 1; i >= 0; i--){
			updated = tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos, regions, func_info);
			if (updated){
				affected = true;
				lines.push_back(make_pair(curpos, instr->pc));
			}
		}

		if (affected){  /* that is this instr affects the frontier */
			update_jump_conditionals(tree, instrs, curpos);
		}
		delete[] rinstr;
	}

}

void build_tree_intial_update(uint64_t destination, 
					   uint32_t stride, 
					   uint32_t start,
					   uint32_t end, 
					   Conc_Tree * tree, 
					   vec_cinstr &instrs, 
					   uint32_t original_start,
					   vector<mem_regions_t *> &regions,
					   vector<Func_Info_t *> &func_info){

	DEBUG_PRINT(("initial update tree building\n"), 2);

	rinstr_t * rinstr = NULL;
	cinstr_t * instr = NULL;
	int amount;
	int index = -1;
	int curpos = -1;

	/* get the first assignment to the destination */
	for (int i = end - 1; i >= start; i--){
		instr = instrs[i].first;
		bool found = false;
		for (int j = 0; j < instr->num_dsts; j++){
			operand_t opnd = instr->dsts[j];
			if (is_overlapped(destination, destination + stride - 1 , opnd.value, opnd.value + opnd.width - 1)){
				rinstr = cinstr_to_rinstrs_eflags(instr, amount, instrs[i].second->disassembly, i);
				for (int k = amount - 1; k >= 0; k--){
					if (rinstr[k].dst.value == opnd.value && rinstr[k].dst.width == opnd.width){
						found = true;
						index = k;
						break;
					}
				}
				break;
			}
		}
		if (found) {
			curpos = i;
			break;
		}

	}

	cout << start << " " << end << " " << curpos << endl;

	if (curpos == original_start){
		return;
	}


	ASSERT_MSG((index != -1), ("ERROR: could not find the destination or overlap\n"));

	for (int i = index; i >= 0; i--){
		tree->update_depandancy_backward(&rinstr[i], instr, instrs[curpos].second, curpos, regions, func_info);
	}
	delete[] rinstr;


	uint32_t start_trace = curpos + 1;
	uint32_t end_trace = end;
	
	ASSERT_MSG((index != -1), ("ERROR: should find end trace in start points\n"));


	/* ok now build the tree */
	
	build_conc_tree_helper(start_trace, end_trace, tree, instrs, regions, func_info);
	remove_reg_leaves(tree);
	

	/*ofstream conc_file(get_standard_folder("output") + "\\initial.dot");
	tree->number_tree_nodes();
	tree->print_dot(conc_file, "conc", 0);
	exit(0);*/

	

}

void build_dummy_initial_tree(Conc_Tree * initial_tree, Conc_Tree * red_tree, mem_regions_t * new_region){

	DEBUG_PRINT(("dummy initial tree\n"), 2);
	Conc_Node * node = (Conc_Node *)red_tree->get_head();
	mem_regions_t * region = node->region;
	ASSERT_MSG((region != NULL), ("ERROR: the head should have an output region\n"));

	Conc_Node * dup_node = new Conc_Node(node->symbol);
	initial_tree->set_head(dup_node);
	dup_node->operation = op_assign;

	Conc_Node * new_node = new Conc_Node(node->symbol);
	new_node->symbol->value = node->symbol->value - region->start + new_region->start;
	new_node->region = new_region;

	dup_node->add_forward_ref(new_node);

}


Conc_Tree * build_conc_tree(uint64_t destination,
								uint32_t stride,
								std::vector<uint32_t> start_points,
								int start_trace,
								int end_trace,
								Conc_Tree * tree,
								vec_cinstr &instrs,
								uint64_t farthest,
								vector<mem_regions_t *> &regions,
								vector<Func_Info_t *> &func_info
								){

	int32_t initial_endtrace = end_trace;
	
	DEBUG_PRINT(("build_tree_multi_func(concrete)....\n"), 3);

	/* get the initial starting and ending positions */
	uint curpos;

	pair<int32_t, int32_t> points = get_start_and_end_points(start_points, destination, stride, start_trace, end_trace, instrs);

	start_trace = points.first;
	end_trace = points.second;

	//cout << "from get start " << start_trace << " end " << end_trace << endl;

	if (end_trace != FILE_ENDING) { ASSERT_MSG((end_trace >= start_trace), ("ERROR: trace end should be greater than the trace start\n")); }
	else { end_trace = instrs.size(); }
	
	if (start_trace != FILE_BEGINNING){
		ASSERT_MSG((start_trace < instrs.size()), ("ERROR: trace start should be within the limits of the file"));
		start_trace = start_trace - 1;
	}
	else{ start_trace = 0; }

	uint32_t initial_start = start_trace;

	curpos = start_trace;

	cinstr_t * instr;
	rinstr_t * rinstr;
	int no_rinstrs;

	//now we need to read the next line and start from the correct destination
	bool dest_present = false;
	int index = -1;

	//major assumption here is that reg and mem 'value' fields do not overlap. This is assumed in all other places as well. can have an assert for this
	instr = instrs[curpos].first;
	DEBUG_PRINT(("starting from instr - %s\n", instrs[curpos].second->disassembly.c_str()), 3);
	rinstr = cinstr_to_rinstrs(instr, no_rinstrs, instrs[curpos].second->disassembly, curpos);

	for (int i = no_rinstrs - 1; i >= 0; i--){
		if (rinstr[i].dst.value == destination){
			ASSERT_MSG((rinstr[i].dst.type != IMM_FLOAT_TYPE) && (rinstr[i].dst.type != IMM_INT_TYPE), ("ERROR: dest cannot be an immediate\n"));
			index = i;
			dest_present = true;
			break;
		}
	}


	if (dest_present == false || index < 0) return NULL;

	ASSERT_MSG((dest_present == true) && (index >= 0), ("ERROR: couldn't find the dest to start trace\n")); //we should have found the destination

	/* build the initial part of the tree */
	for (int i = index; i >= 0; i--){
		
		tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos, regions, func_info);
	}
	delete[] rinstr;

	start_trace++;
	uint32_t start_to_initial = start_trace;
	index = -1;
	/* find the end_trace location in the start points */
	for (int i = 0; i < start_points.size(); i++){
		if (start_points[i] == end_trace) index = i;
	}

	for (int i = start_points.size() - 1; i >=0 ; i--){
		if (start_points[i] < start_to_initial){ start_to_initial = start_points[i]; break; }
	}

	if (start_to_initial == start_trace) start_to_initial = 0;

	if (index == -1) end_trace = instrs.size();

	ASSERT_MSG((index != -1), ("ERROR: should find end trace in start points\n"));


	/* ok now build the tree */
	while (start_trace != instrs.size() && start_trace != initial_endtrace){

			DEBUG_PRINT(("%d - %d\n",start_trace,end_trace),3);

			build_conc_tree_helper(start_trace, end_trace, tree, instrs, regions, func_info);
			remove_reg_leaves(tree);

			//start_trace = end_trace; - affects initial tree building
			//if (index + 1 < start_points.size()) end_trace = start_points[++index];
			break;

	}

	Conc_Tree * initial_tree = NULL;

	if (conctree_opt){
		tree->remove_assign_nodes();
		tree->remove_multiplication();
		tree->remove_po_nodes();
		tree->canonicalize_tree();
		tree->simplify_immediates();
		tree->remove_or_minus_1();
		tree->remove_identities();
		tree->number_parameters(regions);
		tree->recursive = false;
		tree->mark_recursive();
	}


	if (tree->recursive){
		initial_tree = new Conc_Tree();
		/* ok we need to build a tree for the initial update definition */

		build_tree_intial_update(destination, stride, start_to_initial, end_trace, initial_tree, instrs, initial_start, regions, func_info);

		if (initial_tree->get_head() == NULL){
			/* ok, there is no initial first create a new region if the dummy region is not built */
			mem_regions_t * region = get_mem_region(farthest, regions);
			if (region == NULL){
				/* dummy region */
				Conc_Node * conc_node = (Conc_Node *)tree->get_head();
				region = new mem_regions_t(*conc_node->region);
				region->start = farthest;
				region->end = conc_node->region->end - conc_node->region->start + farthest;
				region->name = conc_node->region->name + "_in";
				/* need a check for integer overflow !!! */
				regions.push_back(region);
			}

			/* dummy tree */
			build_dummy_initial_tree(initial_tree, tree, region);
			initial_tree->dummy_tree = true;
		}

		if (conctree_opt){
			initial_tree->remove_assign_nodes();
			initial_tree->remove_multiplication();
			initial_tree->remove_po_nodes();
			initial_tree->canonicalize_tree();
			initial_tree->simplify_immediates();
			initial_tree->number_parameters(regions);
		}
	}
	
	/*if (conctree_opt){
		tree->remove_assign_nodes();
		tree->remove_multiplication();
		tree->remove_po_nodes();
		tree->canonicalize_tree();
		tree->simplify_immediates();
		tree->remove_or_minus_1();
		tree->number_parameters(regions);
	}*/


	if (debug_tree){

		DEBUG_PRINT(("--------debug tree printing-----------\n"), 2);

		ofstream file(get_standard_folder("output") + "\\tree_" + to_string(destination) + ".dot");
		tree->number_tree_nodes();
		tree->print_dot(file, "tree", 0);
		tree->num_nodes = 0;
		DEBUG_PRINT(("number of conditionals : %d\n", tree->conditionals.size()), 2);

		Abs_Tree * abs_tree = new Abs_Tree();
		abs_tree->build_abs_tree_unrolled(tree, regions);

		if (abstree_opt){
			abs_tree->verify_minus();
			abs_tree->convert_sub_nodes();
			abs_tree->canonicalize_tree();
			abs_tree->simplify_minus();
			abs_tree->remove_redundant_nodes();
			abs_tree->canonicalize_tree();
		}

		ofstream abs_file(get_standard_folder("output") + "\\abs_tree_" + to_string(destination) + ".dot");
		abs_tree->number_tree_nodes();
		abs_tree->print_dot(abs_file, "abs_tree", 0);
		abs_tree->num_nodes = 0;

		if (initial_tree != NULL){
			DEBUG_PRINT(("initial_tree built\n"), 2);
			ofstream initial_file(get_standard_folder("output") + "\\tree_" + to_string(destination) + "_initial.dot");
			initial_tree->number_tree_nodes();
			initial_tree->print_dot(initial_file, "initial_tree", 0);
			DEBUG_PRINT(("number of conditionals : %d\n", initial_tree->conditionals.size()), 2);
		}

	}

	DEBUG_PRINT(("build_tree_multi_func(concrete) - done\n"), 3);

	return initial_tree;

}


/* conc tree building routine */
void build_conc_tree_single_func(uint64_t destination,
					uint32_t stride,
					std::vector<uint32_t> start_points,
					int start_trace,
					int end_trace,
					Conc_Tree * tree,
					vec_cinstr &instrs,
					vector<mem_regions_t *> &regions,
					vector<Func_Info_t *> &func_info){

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


	if (debug_level >= 4){ print_rinstrs(log_file,rinstr, no_rinstrs); }

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
			tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos, regions, func_info);
		}
		else{
			tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, NULL, curpos, regions, func_info);
		}
	}

	curpos++;

	vector<pair<uint32_t, uint32_t> > lines;

	//do the rest of expression tree building
	for (; curpos < end_trace; curpos++) {

		instr = instrs[curpos].first;
		rinstr = NULL;
		DEBUG_PRINT(("->line - %d\n", curpos), 4);
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

			if (debug_level >= 4){ print_rinstrs(log_file,rinstr, no_rinstrs); }
			bool updated = false;
			bool affected = false;
			for (int i = no_rinstrs - 1; i >= 0; i--){
				if (instrs[curpos].second != NULL){
					updated = tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, instrs[curpos].second, curpos, regions, func_info);
				}
				else{
					updated = tree->update_depandancy_backward(&rinstr[i], instrs[curpos].first, NULL, curpos, regions, func_info);
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
	tree->number_parameters(regions);
	tree->remove_assign_nodes();
	tree->canonicalize_tree();

}

/* this updates conditional statements that are affecting a particular tree */
void update_jump_conditionals(Conc_Tree * tree,
							  vec_cinstr &instrs, 
							  uint32_t pos){

	cinstr_t * instr = instrs[pos].first;
	Static_Info *  static_info = instrs[pos].second;
	vector< pair<Jump_Info *, bool> > input_dep_conditionals = static_info->conditionals;
	
	DEBUG_PRINT(("checking for conditionals attached - instr %d\n", instr->pc), 4);

	for (int i = 0; i < input_dep_conditionals.size(); i++){

		Jump_Info * jump_info = input_dep_conditionals[i].first;
		bool taken = input_dep_conditionals[i].second;
		//get the line number for this jump info
		uint32_t line_cond = 0;
		uint32_t line_jump = 0;


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

		if (jump_info->target_pc != jump_info->fall_pc){ 
			ASSERT_MSG((actual_taken == taken), ("ERROR: branch direction information is inconsistent\n"));
		}
		else{ /* for sbbs and adcs */
			taken = actual_taken;
		}


		bool is_there = false;
		for (int j = 0; j < tree->conditionals.size(); j++){
			if ((jump_info == tree->conditionals[j]->jump_info) &&
				(line_cond == tree->conditionals[j]->line_cond) &&
				(taken == tree->conditionals[j]->taken) &&
				(line_jump == tree->conditionals[j]->line_jump)){
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
	std::ifstream &file,
	vector<mem_regions_t *> &regions){

}


void create_dependancy(Node * src, Node * dest){

	src->srcs.push_back(dest);
	dest->prev.push_back(src);
	dest->pos.push_back(src->srcs.size() - 1);

}

void build_conc_trees_for_conditionals(
	std::vector<uint32_t> start_points,
	Conc_Tree * tree,
	vec_cinstr &instrs,
	uint64_t farthest,
	vector<mem_regions_t *> &regions,
	vector<Func_Info_t *> &func_info){


	DEBUG_PRINT(("build conc tree for conditionals.....\n"), 3);

	for (int i = 0; i < tree->conditionals.size(); i++){

		Jump_Info * info = tree->conditionals[i]->jump_info;
		uint32_t line_cond = tree->conditionals[i]->line_cond;
		uint32_t line_jump = tree->conditionals[i]->line_jump;

		cinstr_t * instr = instrs[line_cond].first;

		vector<Conc_Tree *> cond_trees;

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
				build_conc_tree(instr->srcs[j].value, instr->srcs[j].width, start_points, dst_line + 1, FILE_ENDING, cond_tree, instrs, farthest, regions, func_info);
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

		/* common things */

		/* remove assign nodes head */
		for (int j = 0; j < cond_trees.size(); j++){
			cond_trees[j]->change_head_node();
		}

		/* get the computational node */
		Node * comp_node = tree->get_head();
		Node * new_head_node = new Conc_Node(comp_node->symbol->type, comp_node->symbol->value, comp_node->symbol->width, 0.0);

		if (instr->opcode == OP_cmp){

			/* now create the tree */
			Conc_Node * node = new Conc_Node(REG_TYPE, 150, 4, 0.0);
			node->operation = dr_logical_to_operation(instrs[line_jump].first->opcode);

			/* merge the trees together */
			Node * left = cond_trees[0]->get_head();
			Node * right = cond_trees[1]->get_head();

			create_dependancy(node, left);
			create_dependancy(node, right);

			create_dependancy(new_head_node, node);
			Conc_Tree * new_cond_tree = new Conc_Tree();
			new_cond_tree->set_head(new_head_node);
			tree->conditionals[i]->tree = new_cond_tree;

		}


		else if (instr->opcode == OP_test){

			Conc_Node * head_node = new Conc_Node(REG_TYPE, 150, 4, 0.0);
			head_node->operation = dr_logical_to_operation(instrs[line_jump].first->opcode);

			/* create an and node */
			Conc_Node * and_node = new Conc_Node(REG_TYPE, 150, 4, 0.0);
			and_node->operation = op_and;

			Node * left = cond_trees[0]->get_head();
			Node * right = cond_trees[1]->get_head();

			create_dependancy(and_node, left);
			create_dependancy(and_node, right);

			create_dependancy(head_node, and_node);
			create_dependancy(head_node, new Conc_Node(IMM_INT_TYPE, 0, 4, 0.0)); /* immediate zero node */

			create_dependancy(new_head_node, head_node);
			Conc_Tree * new_cond_tree = new Conc_Tree();
			new_cond_tree->set_head(new_head_node);
			tree->conditionals[i]->tree = new_cond_tree;


		}

		else{
			ASSERT_MSG((false), ("ERROR: conditionals for other instructions are not done - %s\n", dr_operation_to_string(instr->opcode).c_str()));
		}

	}

}


/************************************************************************/
/*  Tree clustering routines				                            */
/************************************************************************/

std::vector<Conc_Tree *> get_similar_trees(
	std::vector<mem_regions_t *> image_regions,
	std::vector<mem_regions_t *> &total_regions,
	uint32_t seed,
	uint32_t * stride,
	std::vector<uint32_t> start_points,
	int32_t start_trace,
	int32_t end_trace,
	uint64_t farthest,
	vec_cinstr &instrs,
	vector<Func_Info_t *> &func_info){


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
	Conc_Tree * initial_tree = build_conc_tree(mem_location, *stride, start_points, FILE_BEGINNING, end_trace, main_tree, instrs, farthest, total_regions, func_info);
	nodes.push_back(main_tree);
	if (initial_tree != NULL) nodes.push_back(initial_tree);

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
				initial_tree = build_conc_tree(mem_location, random_mem_region->bytes_per_pixel, start_points, FILE_BEGINNING, end_trace, created_tree, instrs, farthest, total_regions, func_info); 
				created_tree->print_tree(cout);
				cout << endl;
				main_tree->print_tree(cout);
				cout << endl;

				if (main_tree->are_trees_similar(created_tree)){
					nodes.push_back(created_tree);
					if (initial_tree != NULL) nodes.push_back(initial_tree);
					dims[affected_dim]++;
					sign = -1 * sign * (int)ceil((i + 1) / (double)random_mem_region->dimensions);
					cout << ceil((i + 1) / (double)random_mem_region->dimensions) << endl;
					cout << sign << endl;
					cout << i << endl;
					break;
				}
				else{
					delete initial_tree;
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


vector< vector<int32_t> > get_index_list(mem_regions_t * mem){

	bool finished = false;

	vector< vector<int32_t> > ret;

	vector<int32_t> current_index;

	for (int i = 0; i < mem->dimensions; i++){
		current_index.push_back(0);
	}

	while (!finished){
		/* do stuff */
		ret.push_back(current_index);

		finished = true;
		for (int i = 0; i < mem->dimensions; i++){
			if (current_index[i] < mem->extents[i] - 1){
				current_index[i]++;
				for (int j = 0; j < i; j++){
					current_index[j] = 0;
				}
				finished = false;
				break;
			}
		}

	}

	return ret;


}

std::vector< std::vector <Conc_Tree *> > cluster_trees
		(std::vector<mem_regions_t *> mem_regions,
		std::vector<mem_regions_t *> &total_regions,
		std::vector<uint32_t> start_points,
		vec_cinstr &instrs,
		uint64_t farthest,
		std::string output_folder,
		std::vector<Func_Info_t *> &func_info){



	DEBUG_PRINT(("Building trees for all locations in the output and clustering\n"), 2); 

	mem_regions_t * mem = get_random_output_region(mem_regions);

	vector<Conc_Tree *> trees;
	vector< vector<int32_t> > indexes = get_index_list(mem);
	vector<int32_t> offset = indexes[0];
	bool success = true;

	//for (int i = indexes.size() - 1; i >= 0; i--){
	//for (int i = 0; i < indexes.size()/8; i++){

	int i;
	if (mem->start > mem->end){
		i = indexes.size() - 1;
	}
	else{
		i = 0;
	}
	bool done = false;
	uint32_t count = 0;

	while (!done){

		uint64_t location = get_mem_location(indexes[i], offset, mem, &success);
		ASSERT_MSG(success, ("ERROR: getting mem location error\n"));

		DEBUG_PRINT(("building tree for location %llx\n", location, i), 3);
		for (int j = 0; j < mem->dimensions; j++){
			DEBUG_PRINT(("%d,", indexes[i][j]), 3);
		}
		DEBUG_PRINT(("."), 2);
		DEBUG_PRINT(("\n"),3);

		Conc_Tree * tree = new Conc_Tree();
		tree->tree_num = i;
		Conc_Tree * initial_tree = build_conc_tree(location, mem->bytes_per_pixel, start_points, FILE_BEGINNING, FILE_ENDING, tree, instrs, farthest, total_regions, func_info);
		build_conc_trees_for_conditionals(start_points, tree, instrs, farthest, total_regions, func_info);
		trees.push_back(tree);
		if (initial_tree != NULL){
			build_conc_trees_for_conditionals(start_points, initial_tree, instrs, farthest, total_regions, func_info);
			trees.push_back(initial_tree);
		}

		count++;
		if (mem->start > mem->end){
			i--;
		}
		else{
			i++;
		}

		if (count == indexes.size()/fraction) done = true;
	}
	DEBUG_PRINT(("\n"), 2);

	/*BUG - incrementing by +1 is not general; should increment by the stride */
	/*for (uint64 i = mem->start; i < mem->end; i+= mem->bytes_per_pixel){
		DEBUG_PRINT(("building tree for location %llx - %u\n", i, i - mem->start), 2);
		Conc_Tree * tree = new Conc_Tree();
		tree->tree_num = i - mem->start;
		Conc_Tree * initial_tree = build_conc_tree(i, mem->bytes_per_pixel, start_points, FILE_BEGINNING, FILE_ENDING, tree, instrs, farthest, total_regions);
		build_conc_trees_for_conditionals(start_points, tree, instrs, farthest, total_regions);
		trees.push_back(tree);
		if (initial_tree != NULL){
			build_conc_trees_for_conditionals(start_points, initial_tree, instrs, farthest, total_regions);
			trees.push_back(initial_tree);
		}
		//if (i == mem->start + mem->bytes_per_pixel * 100) exit(0); 
	}*/

	

	DEBUG_PRINT(("clustering trees\n"), 2);

	for (int i = 0; i < trees.size(); i++){
		if (trees[i]->get_head() == NULL) trees.erase(trees.begin() + i--);
	}
	
	/* cluster based on the similarity */
	vector< vector<Conc_Tree *> > clustered_trees = categorize_trees(trees);

	if (clustered_trees.size() == trees.size()){ /* so each tree is a seperate cluster; assume that all the trees are in a single cluster and this would fail is similarity checking in abstrees */
		clustered_trees.clear();
		clustered_trees.push_back(trees);
	}
	
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


vector<Tree *> get_linearly_independant_trees(vector<Conc_Tree *> cluster, vector<mem_regions_t *> regions){

	vector<Tree *> ret;
	mem_regions_t * region = get_mem_region(cluster[0]->get_head()->symbol->value, regions);
	vector<int32_t> pos;

	for (int i = 0; i < cluster.size(); i++){
		pos = get_mem_position(get_mem_region(cluster[i]->get_head()->symbol->value, regions), cluster[i]->get_head()->symbol->value);
		if (pos[0] != 0 && pos[0] != -1){
			ret.push_back(cluster[i]); break;
		}
		
	}
	

	vector<vector<int32_t> > all_pos;
	all_pos.push_back(pos);
	//pos[0] = 0;

	vector<int32_t> next = pos;
	bool backwards = region->start > region->end;

	for (int i = 0; i < pos.size(); i++){

		uint32_t extents = region->extents[i];
		vector<int32_t> now_regions = next;

		bool found = false;
		now_regions[i] = backwards ? region->extents[i]: -1;
		int j = backwards ? now_regions[i] - 1: now_regions[i] + 1;

		while (!found && j >= 0 && j < region->extents[i]){
				bool done = false;
				now_regions[i] = backwards ? now_regions[i] - 1 : now_regions[i] + 1;
				if (now_regions[i] == next[i]) continue;

				cout << "regions " << now_regions[i] << endl;
				for (int k = 0; k < cluster.size(); k++){
					vector<int32_t> now = get_mem_position(region, cluster[k]->get_head()->symbol->value);
					if (now_regions == now){
						ret.push_back(cluster[k]); done = true; found = true;
						all_pos.push_back(now_regions);
						if (ret.size() % 2 == 0) next = now_regions;
						break;
					}
				}
				if (done) break;
			
				if (backwards){
					j--; if (j == 0) found = true;
				}
				else{
					j++; if (j == region->extents[i]) found = true;
				}
		}

		if (ret.size() % 2 == 0) i--;

	}

	DEBUG_PRINT((" positions of abs trees in equations \n"), 2);
	for (int i = 0; i < all_pos.size(); i++){
		vector<int32_t> temp = all_pos[i];
		for (int j = 0; j < temp.size(); j++){
			DEBUG_PRINT(("%d,", temp[j]), 2);
		}
		DEBUG_PRINT(("\n"), 2);
	}

	ASSERT_MSG(ret.size() > region->dimensions, ("ERROR: not enought equations\n"));

	

	return ret;
}

Abs_Tree* abstract_the_trees(vector<Conc_Tree *> cluster, uint32_t no_trees, uint32_t skip,
	vector<mem_regions_t *> &total_regions, vector<pc_mem_region_t *> &pc_mem){

	/* build the abs trees for the main computational tree and abstract it */
	vector<Abs_Tree *> abs_trees;
	vector<Tree *> trees;


	//vector<Tree *> ind_trees = get_linearly_independant_trees(cluster, total_regions);
	int count = 0;
	srand(4);
	for (int j = 0; ; j += (skip  + rand() % 10 + 1 ) ){
		Abs_Tree  * abs_tree = new Abs_Tree();
		//identify_parameters(cluster[j]->head, pc_mem);
		abs_tree->build_abs_tree_unrolled(cluster[j], total_regions);

		if (abstree_opt){
			abs_tree->verify_minus();
			abs_tree->convert_sub_nodes();
			abs_tree->canonicalize_tree();
			abs_tree->simplify_minus();
			abs_tree->remove_redundant_nodes();
			abs_tree->canonicalize_tree(); 
		}

		abs_trees.push_back(abs_tree);
		trees.push_back(abs_tree);
		cout << "sedd " << j << endl;
		count++;
		if (count == no_trees) break;
	} 

	
	/*for (int j = 0; j < ind_trees.size(); j++){
		Abs_Tree  * abs_tree = new Abs_Tree();
		//identify_parameters(cluster[j]->head, pc_mem);
		abs_tree->build_abs_tree_unrolled((Conc_Tree *)ind_trees[j], total_regions);

		if (abstree_opt){
			abs_tree->verify_minus();
			abs_tree->convert_sub_nodes();
			abs_tree->canonicalize_tree();
			abs_tree->simplify_minus();
			abs_tree->remove_redundant_nodes();
			abs_tree->canonicalize_tree();
		}

		abs_trees.push_back(abs_tree);
		trees.push_back(abs_tree);
	}*/

	bool similar = Tree::are_trees_similar(trees);
	if (similar){

		DEBUG_PRINT(("the trees are similar, now getting the algebric filter....\n"), 1);
		Comp_Abs_Tree * comp_tree = new Comp_Abs_Tree();
		comp_tree->build_compound_tree_unrolled(abs_trees);

		ofstream comp_file(get_standard_folder("output") + "\\comp_tree.dot");
		comp_tree->number_tree_nodes();
		comp_tree->print_dot(comp_file, "comp", 1);

		comp_tree->abstract_buffer_indexes();
		Abs_Tree * final_tree = comp_tree->compound_to_abs_tree();
		return final_tree;
	}
	else{
		DEBUG_PRINT(("the trees are not similar; please check\n"), 1);

		for (int i = 0; i < trees.size(); i++){
			ofstream file(get_standard_folder("output") + "\\abs_tree_not_sim" + to_string(i) + ".dot");
			trees[i]->number_tree_nodes();
			trees[i]->print_dot(file, "abs", i);
		}
		ASSERT_MSG(false, ("abs trees are not similar - aborting\n"));

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

		LOG(log_file, "cluster - " << i << endl);
		mem_regions_t * cluster_region = get_mem_region(clusters[i][0]->get_head()->symbol->value, total_regions);
		for (int j = 0; j < clusters[i].size(); j++){
			vector<int32_t> pos = get_mem_position(cluster_region, clusters[i][j]->get_head()->symbol->value);
			for (int k = 0; k < pos.size(); k++){
				LOG(log_file,pos[k] << ",");
			}
			LOG(log_file,endl);
		}


		Conc_Tree * tree = clusters[i][0];
		ofstream conc_file(folder + "\\conc_comp_" + to_string(i) + ".dot", ofstream::out);
		tree->print_dot(conc_file,"cluster_conc",i);
		
		for (int j = 0; j < tree->conditionals.size(); j++){

			Conc_Tree * cond_tree = tree->conditionals[j]->tree;
			cond_tree->number_tree_nodes();
			ofstream conc_file(folder + "\\conc_cond_" + to_string(i) + "_" + to_string(j) + ".dot", ofstream::out);
			cond_tree->print_dot(conc_file,"cluster_conc_cond",j);
			
		}
	}

	
	vector<Abs_Tree_Charac *> tree_characs;

	/* for each cluster */
	for (int i = 0; i < clusters.size(); i++){

		DEBUG_PRINT(("cluster %d\n", i), 2);

		/* get the abstract tree for the computational path */
		uint32_t skip_trees = skip;
		while (no_trees * skip_trees >= clusters[i].size()){
			skip_trees--;
			if (skip_trees == 0) break;
		}

		if (skip_trees == 0){
			DEBUG_PRINT(("WARNING: not enough trees for cluster %d\n", i), 2);
			continue;
		}


		Abs_Tree * abs_tree = abstract_the_trees(clusters[i], no_trees, skip_trees, total_regions, pc_mem);
		/* get the conditional trees*/
		abs_tree->conditional_trees = get_conditional_trees(clusters[i], no_trees, total_regions, skip_trees, pc_mem);

		ofstream abs_file(folder + "\\symbolic_tree_" + to_string(i) + ".dot", ofstream::out);
		uint32_t max_dimensions = abs_tree->get_maximum_dimensions();
		abs_tree->number_tree_nodes();
		abs_tree->print_dot_algebraic(abs_file, "alg", 0, get_vars("x", max_dimensions));


		/* check if the trees are recursive*/
		if (abs_tree->recursive){  /* if yes, then try to get the reduction domain */

			Abs_Tree_Charac * charac = new Abs_Tree_Charac();
			/* ok now determine whether this is indirect */

			Node * indirect_node = NULL;
			Node * head = abs_tree->get_head();
			int32_t pos = head->is_node_indirect();
			if (pos != -1){
				indirect_node = head->srcs[pos];
			}

			charac->red_node = NULL;
			if (indirect_node != NULL){
				Abs_Node * abs_node = abs_tree->find_indirect_node((Abs_Node *)indirect_node);
				charac->red_node = abs_node;
				charac->gaps_in_rdom = false;
			}
			else{
				
				Abs_Tree * first_tree = new Abs_Tree();
				first_tree->build_abs_tree_unrolled(clusters[i][0], total_regions);
				Abs_Tree * last_tree = new Abs_Tree();
				last_tree->build_abs_tree_unrolled(clusters[i][clusters[i].size() - 1], total_regions);

				Abs_Node * first_head = (Abs_Node *)first_tree->get_head();
				Abs_Node * last_head = (Abs_Node *)last_tree->get_head();

				for (int j = 0; j < first_head->mem_info.dimensions ; j++){
					DEBUG_PRINT(("%d %d\n", first_head->mem_info.pos[j], last_head->mem_info.pos[j]), 2);
				}

				bool gap = false;
				for (int j = 0; j < clusters[i].size() - 1; j++){
					if (clusters[i][j + 1]->tree_num - clusters[i][j]->tree_num != 1){
						gap = true; break;
					}
				}

				if (!gap){ /* get the abs pos of the first and last trees */
					//DEBUG_PRINT(("no gaps\n"), 2);
					for (int j = 0; j < first_head->mem_info.dimensions; j++){
						charac->extents.push_back(make_pair(first_head->mem_info.pos[j], last_head->mem_info.pos[j]));
					}
					charac->gaps_in_rdom = false;
				}
				else{
					charac->gaps_in_rdom = true;

					for (int j = 0; j < first_head->mem_info.dimensions; j++){
						charac->extents.push_back(make_pair(0, first_head->mem_info.associated_mem->extents[j] - 1));
					}
				}
			}

			DEBUG_PRINT(("red tree built\n"), 2);

			charac->tree = abs_tree;
			charac->is_recursive = true;
			tree_characs.push_back(charac);

		}
		else{ /* if no, then populate pure functions */

			DEBUG_PRINT(("pure tree built\n"), 2);

			Abs_Tree_Charac * charac = new Abs_Tree_Charac();
			charac->tree = abs_tree;
			charac->is_recursive = false;
			tree_characs.push_back(charac);

		}
	}



	/* now let's check for recursive trees whether there are opportunities with dummy trees - this is to avoid multi-image solving; in reality this part can be removed */
	for (int i = 0; i < tree_characs.size(); i++){

		Abs_Tree_Charac * charac = tree_characs[i];
		if (charac->is_recursive && charac->red_node == NULL){

			Abs_Node * node = (Abs_Node *)charac->tree->get_head();
			uint32_t size = get_region_size(node->mem_info.associated_mem);

			uint32_t rdom_size = 1;
			for (int j = 0; j < charac->extents.size(); j++){
				rdom_size *= charac->extents[j].second - charac->extents[j].first + 1;
			}

			if (size == rdom_size){

				for (int j = 0; j < tree_characs.size(); j++){
					Abs_Tree_Charac * now_charac = tree_characs[j];
					Abs_Node * now_node = (Abs_Node *)now_charac->tree->get_head();
					if (!now_charac->is_recursive && (now_node->mem_info.associated_mem == node->mem_info.associated_mem) && now_charac->tree->dummy_tree){
						DEBUG_PRINT(("dummy tree red charac found\n"), 2);
						charac->red_node = (Abs_Node *)now_node->srcs[0];
						break;
					}
				}
			}
		}

	}




	return tree_characs;


}
