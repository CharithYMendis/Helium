/*given a file and some other parameters this will build the expression tree*/
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "analysis/tree_analysis.h"
#include "common_defines.h"
#include "utility/fileparser.h"
#include "analysis/x86_analysis.h"
#include "defines.h"

using namespace std;

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
			tree->update_depandancy_backward(&rinstr[i], instr->pc, instrs[curpos].second->disassembly, curpos);
		}
		else{
			tree->update_depandancy_backward(&rinstr[i], instr->pc, "not captured\n", curpos);
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
					updated = tree->update_depandancy_backward(&rinstr[i], instr->pc, instrs[curpos].second->disassembly, curpos);
				}
				else{
					updated = tree->update_depandancy_backward(&rinstr[i], instr->pc, "not captured\n", curpos);
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
	remove_po_node(tree->get_head(), tree->get_head(), NULL, 0);
	order_tree(tree->get_head());

}

/* this updates conditional statements that are affecting a particular tree */
void update_jump_conditionals(Conc_Tree * tree,
							  vec_cinstr &instrs, 
							  uint32_t pos){

	cinstr_t * instr = instrs[pos].first;
	Static_Info *  static_info = instrs[pos].second;
	vector< pair<Jump_Info *, bool> > input_dep_conditionals = static_info->conditionals;
	
	DEBUG_PRINT(("checking for conditionals attached - instr %d\n", instr->pc), 2);

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
	vec_cinstr &instrs,
	std::vector<Static_Info *> static_info){


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

			for (int i = 0; i < cond_trees.size(); i++){
				cond_trees[i]->change_head_node();
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


