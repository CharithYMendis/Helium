#include <sys\stat.h>
#include <string>
#include <stack>
#include <algorithm>
#include <list>

#include "halide/halide.h"
#include "trees/nodes.h"
#include "common_defines.h"

#include "utilities.h"



Halide_Program::Halide_Program()
{

}



/************************************************************************/
/* printing helpers                                                     */
/************************************************************************/

/*halide header and footer - in future move to a file and read the header from the file*/
string print_Halide_header(){
	return "#include <Halide.h>\n  #include <vector>\n  using namespace std;\n  using namespace Halide;\n  int main(){ \n";
}

string print_Halide_footer(){
	return "return 0;\n}";
}

/* halide supported types - Int, UInt , Float with width specified as the first parameter to the type */
/* does Halide support double? */
string print_Halide_type(uint width, bool sign, bool is_float){

	string sign_string = sign ? "" : "U"; 

	if (!is_float)
		return sign_string + "Int(" + to_string(width * 8) + ")";
	else
		return "Float";

}

/* arguments.push_back(<name>)*/
string print_Halide_argument(string vector, string argument){
	return vector + ".push_back(" + argument + ");";
}

string print_Halide_output_to_file(Abs_Node * node, string filename, uint32_t file_no, string vector){

	return node->mem_info.associated_mem->name + ".compile_to_file(\"" + filename + "_" + to_string(file_no) + "\"," + vector + ");";
}

/************************************************************************/
/*  Variable and function declarations Halide                           */
/************************************************************************/

/* ImageParam name(type,dim); */
string print_Halide_input_declaration(Abs_Node * node){

	//ASSERT_MSG((node->type == Abs_Node::INPUT_NODE || node->type == Abs_Node::INTERMEDIATE_NODE), ("ERROR: the node cannot be a output"));
	

	//cout << node->mem_info.associated_mem->name << " " << node->mem_info.associated_mem->trees_direction << endl;


	if ((node->mem_info.associated_mem->trees_direction & MEM_INPUT) != MEM_INPUT) return "";

	string in_string =  ((node->mem_info.associated_mem->trees_direction & MEM_OUTPUT) == MEM_OUTPUT) ? "_buf_in" : "" ;

	return "ImageParam " + node->mem_info.associated_mem->name + in_string + "(" 
		+ print_Halide_type(node->symbol->width, node->sign, node->is_double) + ","
		+ to_string(node->mem_info.dimensions) + ");" ;

}

/* Param<type> name("name"); */
string print_Halide_parameter_declaration(Abs_Node * node){

	ASSERT_MSG((node->type == Abs_Node::PARAMETER), ("ERROR: the node is not a parameter\n"));

	string ret = "Param<";

	if (node->is_double){
		ret += "double";
	}
	else{
		if (!node->sign) ret += "u";
		ret += "int" + to_string(node->symbol->width * 8) + "_t";
	}
	ret += "> ";

	ret += "p_" + to_string(node->para_num) + "(\"p_" + to_string(node->para_num) + "\");";

	return ret;

}

/* Func name;*/
string print_Halide_function_declaration(Abs_Node * node){
	return "Func " + node->mem_info.associated_mem->name + ";";
}

/* Var name;*/
string print_Halide_variable_declaration(string var){
	return "Var " + var + ";" ;
}

/************************************************************************/
/* Halide Program populations											*/
/************************************************************************/

void Halide_Program::populate_vars(uint32_t dim){
	
	string x = "x";
	for (int i = 0; i < dim; i++){
		vars.push_back(x + + "_" +  to_string(i)); 
	}

}

void Halide_Program::resolve_conditionals(){

	/* if there is no else; if assume it is coming from outside */

	/* BUG - need to handle all cases */

	for (int i = 0; i < funcs.size(); i++){

		/* check whether there is no statement with no conditionals */
		if (funcs[i]->pure_trees.size() == 1){
			if (funcs[i]->pure_trees[0]->conditional_trees.size() > 0){
				/* add a new tree output = output values coming from outside */
				Abs_Tree * existing_tree = funcs[i]->pure_trees[0];
				Abs_Tree * new_tree = new Abs_Tree();
				
				Abs_Node * new_head = new Abs_Node(*(Abs_Node *)existing_tree->get_head());
				new_head->operation = op_assign;
				new_head->minus = false;

				Abs_Node * assign_node = new Abs_Node(*(Abs_Node *)existing_tree->get_head());
				for (int j = 0; j < assign_node->mem_info.dimensions; j++){
					for (int k = 0; k < assign_node->mem_info.head_dimensions + 1; k++){
						if (j == k) assign_node->mem_info.indexes[j][k] = 1;
						else assign_node->mem_info.indexes[j][k] = 0;
					}
				}
				assign_node->minus = false;
				new_tree->set_head(new_head);
				new_head->add_forward_ref(assign_node); 
				funcs[i]->pure_trees.push_back(new_tree);
			}
		}
		
	}


}

Halide_Program::Func * Halide_Program::check_function(mem_regions_t * mem){

	for (int i = 0; i < funcs.size(); i++){

		if (funcs[i]->pure_trees.size() > 0){
			Abs_Node * node = static_cast<Abs_Node *>(funcs[i]->pure_trees[0]->get_head());
			if (node->mem_info.associated_mem == mem){
				return funcs[i];
			}
		}
		
		for (int j = 0; j < funcs[i]->reduction_trees.size(); j++){
			vector<Abs_Tree *> trees = funcs[i]->reduction_trees[j].second;
			for (int k = 0; k < trees.size(); k++){
				Abs_Node * node = static_cast<Abs_Node *>(trees[k]->get_head());
				if (node->mem_info.associated_mem == mem){
					return funcs[i];
				}
			}
		}
	}

	

	return NULL;

}

bool compare_tree(Abs_Tree * first, Abs_Tree * second){
	return first->conditional_trees.size() > second->conditional_trees.size();
}

void Halide_Program::sort_functions(){

	for (int i = 0; i < funcs.size(); i++){
		sort(funcs[i]->pure_trees.begin(), funcs[i]->pure_trees.end(), compare_tree);
		for (int j = 0; j < funcs[i]->reduction_trees.size(); j++){
			sort(funcs[i]->reduction_trees[j].second.begin(),
				funcs[i]->reduction_trees[j].second.end(), compare_tree);
		}
	
	}

	

}

void Halide_Program::populate_pure_funcs(Abs_Tree * tree){

	Abs_Node * node = static_cast<Abs_Node *>(tree->get_head());

	Func * func = check_function(node->mem_info.associated_mem);
	if (func == NULL){
		func = new Func();
		funcs.push_back(func);
	}

	func->pure_trees.push_back(tree);
	

}

void Halide_Program::populate_pure_funcs(std::vector<Abs_Tree *> trees){

	for (int i = 0; i < trees.size(); i++){
		populate_pure_funcs(trees[i]);
	}

}

int32_t Halide_Program::get_rdom_location(Func *func, RDom *rdom){

	for (int i = 0; i < func->reduction_trees.size(); i++){

		RDom * cur_rdom = func->reduction_trees[i].first;
		if (rdom->type == cur_rdom->type){
			if (rdom->type == INDIRECT_REF){
				if (rdom->red_node->mem_info.associated_mem ==
					cur_rdom->red_node->mem_info.associated_mem){
					return i;
				}
			}
			else{
				vector< pair<int32_t, int32_t> > first = cur_rdom->extents;
				vector< pair<int32_t, int32_t> > second = rdom->extents;

				ASSERT_MSG((first.size() == second.size()), ("ERROR: reduction domain dimensions for the same buffer should be the same"));

				bool similar = true;
				for (int j = 0; j < first.size(); j++){
					if (first[i].first != second[i].first || first[i].second != second[i].second){
						similar = false;
						break;
					}
				}

				if (similar){
					delete rdom;
					return i;
				}
			}
		}

	}

	return -1;



}

void Halide_Program::populate_red_funcs(Abs_Tree * tree, 
	std::vector< std::pair<int32_t, int32_t > > boundaries, Abs_Node * node){

	RDom * rdom = new RDom();
	if (node != NULL){
		rdom->red_node = node;
		rdom->type = INDIRECT_REF;
		cout << "indirect ref populated" << endl;
	}
	else{
		rdom->extents = boundaries;
		rdom->type = EXTENTS;
		cout << "extents populated" << endl;
	}

	Abs_Node * head = static_cast<Abs_Node *>(tree->get_head());
	Func * func = check_function(head->mem_info.associated_mem);
	if (func == NULL){
		func = new Func();
		funcs.push_back(func);
	}

	int32_t loc = get_rdom_location(func, rdom);
	if (loc != -1){
		func->reduction_trees[loc].second.push_back(tree);
		delete rdom;
	}
	else{
		vector<Abs_Tree *> tree_vec;
		tree_vec.push_back(tree);
		func->reduction_trees.push_back(make_pair(rdom, tree_vec));
	}

}

void Halide_Program::populate_paras_input_paras(bool para)
{
	vector<Abs_Tree *> trees;
	for (int i = 0; i < funcs.size(); i++){
		trees.insert(trees.end(),funcs[i]->pure_trees.begin(), funcs[i]->pure_trees.end());
		for (int j = 0; j < funcs[i]->reduction_trees.size(); j++){
			trees.insert(trees.end(), funcs[i]->reduction_trees[j].second.begin(),
				funcs[i]->reduction_trees[j].second.end());
		}
	}


	/*get the conditional trees as well*/
	int sz = trees.size();
	for (int i = 0; i < sz; i++){
		for (int j = 0; j < trees[i]->conditional_trees.size(); j++){
			trees.push_back(trees[i]->conditional_trees[j].first);
		}
	}

	vector<Abs_Node *> total;

	for (int i = 0; i < trees.size(); i++){
		
		vector<Abs_Node *> temp;
		temp = para ? trees[i]->retrieve_parameters() : trees[i]->get_buffer_region_nodes();
		total.insert(total.end(), temp.begin(), temp.end());
	}

	if (para){

		/* paras we can have duplicates */
		for (int i = 0; i < total.size(); i++){
			bool found = false;

			for (int j = 0; j < i; j++){
				if ((total[i]->symbol->type == total[j]->symbol->type) &&
					(total[i]->symbol->value == total[j]->symbol->value) &&
					(total[i]->symbol->width == total[j]->symbol->width)){
					param_match[total[i]->para_num] = total[j]->para_num;
					total.erase(total.begin() + i--);
					found = true;
					break;
				}
			}
			if (!found){
				param_match[total[i]->para_num] = total[i]->para_num;
			}
		}

		params = total;

	}
	else{

		for (int i = 0; i < total.size(); i++){
			for (int j = 0; j < total.size(); j++){
				if (i==j) continue;
				if (total[i]->mem_info.associated_mem == total[j]->mem_info.associated_mem){
					total.erase(total.begin() + i--);
					break;
				}
			}
		}

		/* now first populate the outputs */
		for (int i = 0; i < funcs.size(); i++){
			Abs_Node * output_node = (Abs_Node *)funcs[i]->pure_trees[0]->get_head();
			output.push_back(output_node);
			mem_regions_t * head_region = output_node->mem_info.associated_mem;
			head_region->trees_direction |= MEM_OUTPUT;
		}

		for (int i = 0; i < total.size(); i++){
			total[i]->mem_info.associated_mem->trees_direction |= MEM_INPUT;
			for (int j = 0; j < output.size(); j++){
				if (total[i]->mem_info.associated_mem == output[j]->mem_info.associated_mem){
					if (total[i]->is_node_indirect() != -1) total[i]->mem_info.associated_mem->trees_direction &= ~((uint32_t)MEM_INPUT);
						break;
				}
			}
		}

		inputs = total;

	}
}

void Halide_Program::populate_input_params(){

	populate_paras_input_paras(false);
	
}

void Halide_Program::populate_params(){

	populate_paras_input_paras(true);

}

void Halide_Program::populate_outputs(Abs_Node * node){
	output.push_back(node);
}

void Halide_Program::populate_halide_program(){

}



/************************************************************************/
/* casting and overlap node printing								    */
/************************************************************************/

string get_cast_string(Abs_Node * node, uint32_t sign){

	string ret = "";
	ret += "cast<";

	if (node->is_double){
		ret += "double>";
	}
	else{
		if (!sign) ret += "u";
		ret += "int" + to_string(node->symbol->width * 8) + "_t>";
	}

	return ret;

	
}

/* FO -> going from small to big (like ah -> eax) */
string Halide_Program::print_full_overlap_node(Abs_Node * node, Node * head, vector<string> vars){
	/* here, we will some times we need to use shifting and anding */
	Abs_Node * overlap = static_cast<Abs_Node *>(node->srcs[0]);

	uint32_t overlap_end = overlap->symbol->value + overlap->symbol->width;
	uint32_t node_end = node->symbol->value + node->symbol->width;

	/* BUG - overlap_end == node_end ? this is not always true if mem and reg values are*/

	
	string ret = "";

	if (node->symbol->width == overlap->symbol->width){ /* where the nodes are of reg and memory etc.*/
		ret += print_abs_tree(overlap, head, vars);
		return ret;
	}

	uint32_t mask = uint32_t(~0) >> (32 - node->symbol->width * 8);
	mask = mask > 65535 ? 65535 : mask;

	
	/*if(node->symbol->type == REG_TYPE && overlap->symbol->type == REG_TYPE && overlap_end != node_end){
		ret += " ( ( ";
		ret += print_abs_tree(overlap,head, vars);
		ret += " >> " + to_string((overlap_end - node_end) * 8) + " ) ";
		ret += " ) & " + to_string(mask);
	}
	else{*/
		ret += " ( ";
		ret += print_abs_tree(overlap, head, vars);
		ret += " ) & " + to_string(mask);
	//}

	return ret;

}

string Halide_Program::print_partial_overlap_node(Abs_Node * node, Node * head, vector<string> vars){

	string ret = "";

	/*for (int i = 0; i < node->srcs.size(); i++)
	{
		Abs_Node * overlap = static_cast<Abs_Node *>(node->srcs[i]);

		uint32_t overlap_end = overlap->symbol->value + overlap->symbol->width;
		uint32_t node_end = node->symbol->value + node->symbol->width;

		
		ret += "(";
		ret += print_abs_tree(overlap, head,vars);
		ret += " << " + to_string((node_end - overlap_end) * 8);
		ret += ")";

		if (i != node->srcs.size() - 1){
			ret+= " | ";
		}
	}*/
	return ret;
}

/************************************************************************/
/* Function and main Halide body printing	                            */
/************************************************************************/

void Halide_Program::print_halide_program(std::ostream &out, vector<string> red_variables){

	DEBUG_PRINT(("printing halide....\n"), 1);

	map<uint32_t, uint32_t>::iterator it;

	for (it = param_match.begin(); it != param_match.end(); it++){
		cout << it->first << " " << it->second << endl;
	}

	/*print the Halide header*/
	out << print_Halide_header() << endl;

	/****************** print declarations **********************/
	/* print Vars */
	for (int i = 0; i < vars.size(); i++){
		out << print_Halide_variable_declaration(vars[i]) << endl;
	}

	/* print InputParams */
	for (int i = 0; i < inputs.size(); i++){
		out << print_Halide_input_declaration(inputs[i]) << endl;
	}

	/* print Params */
	for (int i = 0; i < params.size(); i++){
		out << print_Halide_parameter_declaration(params[i]) << endl;
	}


	sort_functions();
	/* print Funcs */
	for (int i = 0; i < funcs.size(); i++){
		out << print_Halide_function_declaration(
			static_cast<Abs_Node *>(funcs[i]->pure_trees[0]->get_head())) << endl;
	}

	/***************** print the functions ************************/

	for (int i = 0; i < funcs.size(); i++){
		out << print_function(funcs[i], red_variables) << endl;
	}

	/***************finalizing - instructions for code generation ******/

	/* print argument population - params and input params */
	string arg = "arguments";
	out << "vector<Argument> " << arg << ";" << endl;
	for (int i = 0; i < params.size(); i++){
		out << print_Halide_argument(arg, "p_" + to_string(params[i]->para_num)) << ";" << endl;
	}
	for (int i = 0; i < inputs.size(); i++){
		out << print_Halide_argument(arg, inputs[i]->mem_info.associated_mem->name) << ";" << endl;
	}

	/* output to file */
	for (int i = 0; i < output.size(); i++){
		out << print_Halide_output_to_file(output[i], "halide_out", i, arg) << endl;
	}

	/* print the final halide footer */
	out << print_Halide_footer() << endl;

}

vector<string> get_reduction_index_variables(string rvar){

	vector<string> rvars;
	rvars.push_back(rvar + ".x");
	rvars.push_back(rvar + ".y");
	rvars.push_back(rvar + ".z");
	rvars.push_back(rvar + ".w");
	return rvars;

}

string print_expression_name(string expr_name, uint32_t conditional){

	return expr_name + "_" + to_string(conditional);

}

string print_select_statement(Halide_Program::Select_Expr * current_expr,Halide_Program::Select_Expr * next_expr){

	if (next_expr != NULL){ /* we have a false value*/
		return "Expr " + current_expr->name + " = select(" + current_expr->condition + "," + current_expr->truth_value + "," + next_expr->name + ");\n";
	}
	else{
		return "Expr " + current_expr->name + " = " + current_expr->truth_value + ";\n";
	}

	
}

Abs_Node * get_indirect_node(Abs_Node * node){
	
	return (Abs_Node *)node->srcs[0];
	
	/*Abs_Node * base = (Abs_Node *)node->srcs[0]->srcs[0];
	Abs_Node * index = (Abs_Node *)node->srcs[0]->srcs[1];
	if (base->type == Abs_Node::OPERATION_ONLY){
		return index;
	}
	else{
		return base;
	}*/

}

string print_output_func_def(Abs_Node * head, vector<string> vars){

	mem_regions_t * mem = head->mem_info.associated_mem;

	int32_t pos = head->is_node_indirect();
	bool indirect = (pos != -1);

	string ret = mem->name + "(";

	/* assume only one level of indirection */
	if (indirect){
		Abs_Node * indirect_node = get_indirect_node((Abs_Node *)head->srcs[pos]);
		ret += indirect_node->mem_info.associated_mem->name + "(";
		head = indirect_node;
	}

	for (int i = 0; i < head->mem_info.dimensions; i++){
		ret += vars[i];
		if (i == head->mem_info.dimensions - 1){
			ret += ")";
		}
		else{
			ret += ",";
		}
	}
	
	if (indirect){
		ret += ")";
	}

	return ret;
}

string Halide_Program::print_predicated_tree(vector<Abs_Tree *> trees, string expr_tag, vector<string> vars){

	vector<Select_Expr *> exprs;

	/* populate the expressions */
	for (int i = 0; i < trees.size(); i++){
		Abs_Node * head = static_cast<Abs_Node *>(trees[i]->get_head());
		Select_Expr * expr = new Select_Expr();
		expr->name = print_expression_name(head->mem_info.associated_mem->name + expr_tag, i);
		expr->condition = print_conditional_trees(trees[i]->conditional_trees, vars);
		expr->truth_value = print_abs_tree(trees[i]->get_head(),
			trees[i]->get_head(), vars);
		exprs.push_back(expr);
	}

	/* final print statements */
	vector<string> statements;

	for (int i = 0; i < exprs.size() - 1; i++){
		statements.push_back(print_select_statement(exprs[i], exprs[i + 1]));
	}

	statements.push_back(print_select_statement(exprs[exprs.size() - 1], NULL));
	

	string output = "";

	for (int i = statements.size() - 1; i >= 0; i--){
		output += statements[i];
	}

	/* finally update the final output location */
	output += print_output_func_def(
		static_cast<Abs_Node *>(trees[0]->get_head()),
		vars);

	Abs_Node * head_node = static_cast<Abs_Node *>(trees[0]->get_head());

	uint32_t clamp_max = min((uint32_t(~0)) >> (32 - head_node->symbol->width * 8),(uint32_t)65535);
	uint32_t clamp_min = 0;

	/* BUG - what to do with the sign?? */
	output += " = " + get_cast_string(head_node,false) + "( clamp(" + exprs[exprs.size() - 1]->name + "," + to_string(clamp_min) + "," + to_string(clamp_max) + ") );\n";

	return output;

}

string Halide_Program::print_pure_trees(Func * func){

	return print_predicated_tree(func->pure_trees, "_p_", vars);

}

string Halide_Program::print_rdom(RDom * rdom, vector<string> variables){

	string name = rvars[rvars.size() - 1];
	string ret = "RDom " + name + "(";
	cout << "rdom type " << rdom->type << endl;
	if (rdom->type == INDIRECT_REF){
		ret += rdom->red_node->mem_info.associated_mem->name;
	}
	else{
		for (int i = 0; i < rdom->abstract_indexes.size(); i++){
			for (int j = 0; j < rdom->abstract_indexes[i].size(); j++){
				ret += to_string(rdom->abstract_indexes[i][j]) + " * " + variables[j];
				if (j != rdom->abstract_indexes[i].size() - 1){
					ret += " + ";
				}
			}
			if (i != rdom->abstract_indexes.size()){
				ret += " , ";
			}
		}
	}
	ret += " );";
	return ret;

}

std::string Halide_Program::print_red_trees(Func * func, vector<string> red_variables){

	/* Assumption - If the RDom is the same, then the trees are different due to conditionals.
	If the RDom's are not the same, then those trees are computed without overlap */
	
	ASSERT_MSG((func->pure_trees.size() > 0), ("ERROR: reduction updates should have initial pure definitions\n"));

	if (func->reduction_trees.size() == 0) return "";

	string ret = "";
	for (int i = 0; i < func->reduction_trees.size(); i++){
		string name = "r_" + to_string(rvars.size());
		rvars.push_back(name);
		ret += print_rdom(func->reduction_trees[i].first, red_variables) + "\n";
		ret += print_predicated_tree(func->reduction_trees[i].second, "_r" + to_string(i) + "_", get_reduction_index_variables(name));
	
	}

	return ret;

}

std::string Halide_Program::print_function(Func * func, vector<string> red_variables){

	/* NOTE that red_variables those form the expression for a non-constant 
	reduction domain min and extents */

	/* anyway print the pure trees first; 
	even for reductions these would be initial definitions */
	string ret = "";

	ret += print_pure_trees(func);
	ret += print_red_trees(func, red_variables);

	return ret;
	
}


/* Need to revamp these routines based on how the trees are transformed */
/* very crude printing */
std::string Halide_Program::get_indirect_string(Abs_Node * node, Abs_Node * head, vector<string> vars){


	return print_abs_tree(node->srcs[0], head, vars);


	/*if (node->srcs[0]->operation == op_add){
		Abs_Node * base = (Abs_Node *)node->srcs[0]->srcs[0]; 
		Abs_Node * index = (Abs_Node *)node->srcs[0]->srcs[1];

		if (base->type == Abs_Node::OPERATION_ONLY){
			return print_abs_tree(index, head, vars);
		}
		else{
			return print_abs_tree(base, head, vars);
		}
	}

	return ""; */

}


std::string Halide_Program::print_abs_tree(Node * nnode, Node * head ,vector<string> vars){

	Abs_Node * node = static_cast<Abs_Node *>(nnode);

	string ret = "";
	
	if (node->minus){
		ret += "- (";
	}
	
	if (node->type == Abs_Node::OPERATION_ONLY){
		if (node->operation == op_full_overlap){
			ret += " ( ";
			ret += print_full_overlap_node(node, head, vars);
			ret += " ) ";
		}
		else if (node->operation == op_partial_overlap){
			ret += " ( ";
			ret += print_partial_overlap_node(node, head,  vars);
			ret += " ) ";
		}
		else if (node->operation == op_split_h){
			ret += " ( ";
			ret += print_abs_tree(node->srcs[0],head, vars);
			ret += " ) >> ( " + to_string(node->srcs[0]->symbol->width * 8 / 2) + ")";
		}
		else if (node->operation == op_split_l){
			ret += " ( ";
			ret += print_abs_tree(node->srcs[0],head, vars);
			ret += " ) & " + to_string((node->srcs[0]->symbol->width / 2) * 8);
		}
		else if (node->operation == op_indirect){
			ret += "(";
			ret += get_indirect_string(node, (Abs_Node *)head, vars);
			ret += ")";
		}
		else if (node->operation == op_call){
			ret += "(";
			ret += node->func_name + "(";
			for (int k = 0; k < node->srcs.size(); k++){
				Abs_Node * abs_nodes = (Abs_Node *)node->srcs[k];
				ret += print_abs_tree(abs_nodes, head, vars);
				if (k != node->srcs.size() - 1){
					ret += ",";
				}
			}
			ret += ")";
		}
		else if (node->srcs.size() == 1){
			ret += " " + node->get_symbolic_string(vars) + " ";
			ret += print_abs_tree(node->srcs[0],head, vars);
		}
		else{

			ret += "(";
			for (int i = 0; i < node->srcs.size(); i++){

				if (node->srcs[i]->symbol->width != node->symbol->width){
						ret += get_cast_string(node, false) + "(";
				}
				ret += print_abs_tree(node->srcs[i],head, vars);
				if (node->srcs[i]->symbol->width != node->symbol->width){
					ret += ")";
				}
				if (i != node->srcs.size() - 1){
					ret += " " + node->get_symbolic_string(vars) + " ";
				}
			}
			ret += ")";
		}
	}
	else if (node->type == Abs_Node::SUBTREE_BOUNDARY){
	
	}
	else {


		int32_t pos = node->is_node_indirect();
		bool indirect = (pos != -1);

		if (node != head){
			if (indirect){
				ret += node->mem_info.associated_mem->name ;
				ret += print_abs_tree(node->srcs[pos], head, vars); /* assumes that these nodes are at the leaves */
			}
			else{

				if (node->type == Abs_Node::PARAMETER){
					ret += "p_" + to_string(param_match[node->para_num]) + " ";
				}
				else{
					ret += node->get_symbolic_string(vars) + " ";
				}
			}
		}
		else{

			Node * indirect_node;
			if (indirect){
				indirect_node = node->srcs[pos];
				node->srcs.erase(node->srcs.begin() + pos);
			}

			if (node->operation != op_assign){  /* the node contains some other operation */
				uint32_t ori_type = node->type;
				node->type = Abs_Node::OPERATION_ONLY;
				ret += print_abs_tree(node, head, vars);
				node->type = ori_type;
			}
			else{
				ret += print_abs_tree(node->srcs[0], head, vars);
			}

			if (indirect){
				node->srcs.insert(node->srcs.begin() + pos, indirect_node);
			}

		}
		

	}

	if (node->minus){
		ret += ")";
	}

	return ret;
}

std::string Halide_Program::print_conditional_trees(std::vector< std::pair<Abs_Tree *, bool > > conditions, vector<string> vars){

	string ret = "";

	for (int i = 0; i < conditions.size(); i++){

		Abs_Node * node = static_cast<Abs_Node *>(conditions[i].first->get_head());
		/* because the head node is just the output node - verify this fact */
		ASSERT_MSG((node->srcs.size() == 1), ("ERROR: expected single source\n"));
		bool taken = conditions[i].second;

		if (!taken){
			ret += "! (";
		}

		ret += print_abs_tree(node->srcs[0], node, vars);
		if (i != conditions.size() - 1){
			ret += " && ";
		}

		if (!taken){
			ret += ")";
		}
	}

	return ret;


}


/************************************************************************/
/*     Reduction domain abstraction                                     */
/************************************************************************/


void print_dump_to_file(ofstream &file, mem_dump_regions_t * region){

	file << "uint8_t " << region->name << "[] = {";

	for (int i = 0; i < region->size; i++){

		file << to_string((uint8_t)region->values[i]);

		if (i != region->size - 1){
			file << ",";
		}


	}

	file << "};" << endl;

}

vector<mem_dump_regions_t *> Halide_Program::get_memory_regions(vector<string> memdump_files){

	struct mem_dump_t{
		char * buffer;
		uint32_t start;
		uint32_t end;
		bool write;
	};

	vector<mem_dump_t *> dump;

	for (int i = 0; i < memdump_files.size(); i++){

		ifstream file(memdump_files[i], ios::in | ios::binary);

		vector<string> parts = split(memdump_files[i], '_');
		uint32_t index = parts.size() - 2;
		bool write = parts[index][0] - '0';
		uint32_t size = strtoull(parts[index - 1].c_str(), NULL, 10);
		uint64_t base_pc = strtoull(parts[index - 2].c_str(), NULL, 16);

		uint64_t start = base_pc;
		uint64_t end = base_pc + size;

		struct stat results;
		char *  file_values;

		ASSERT_MSG(stat(memdump_files[i].c_str(), &results) == 0, ("file stat collection unsuccessful\n"));
		
		file_values = new char[results.st_size];
		file.read(file_values, results.st_size);

		dump.push_back(new mem_dump_t());
		dump[i]->start = start;
		dump[i]->end = end;
		dump[i]->buffer = file_values;
		dump[i]->write = write;

	}

	vector<mem_dump_regions_t *> dumps;

	// for the funcs get the values 
	for (int i = 0; i < funcs.size(); i++){
		Abs_Node * head = (Abs_Node *)funcs[i]->pure_trees[0]->get_head();
		uint64_t start = head->mem_info.associated_mem->start;
		uint64_t end = head->mem_info.associated_mem->end;

		for(int j=0; j< dump.size(); j++){
			if (dump[j]->write){
				if (start >= dump[j]->start && end <= dump[j]->end){
					mem_dump_regions_t * dump_region = new mem_dump_regions_t;
					dump_region->name = head->mem_info.associated_mem->name;
					dump_region->values = &dump[j]->buffer[start - dump[j]->start];
					dump_region->size = get_actual_size(head->mem_info.associated_mem);
					dumps.push_back(dump_region);
					break;
				}
			}
		}
	}

	// for all inputs
	for (int i = 0; i < inputs.size(); i++){

		uint64_t start = inputs[i]->mem_info.associated_mem->start;
		uint64_t end = inputs[i]->mem_info.associated_mem->end;

		for(int j=0; j< dump.size(); j++){
			if (!dump[j]->write){
				if (start >= dump[j]->start && end <= dump[j]->end){
					mem_dump_regions_t * dump_region = new mem_dump_regions_t;
					dump_region->name = inputs[i]->mem_info.associated_mem->name;
					dump_region->values = &dump[j]->buffer[start - dump[j]->start];
					dump_region->size = get_actual_size(inputs[i]->mem_info.associated_mem);
					dumps.push_back(dump_region);
					break;
				}
			}
		}
	}


	//for all dummy inputs
	for (int i = 0; i < funcs.size(); i++){
		if (funcs[i]->reduction_trees.size() > 0){

			for (int j = 0; j < funcs[i]->pure_trees.size(); j++){
				if (funcs[i]->pure_trees[j]->dummy_tree){

					Abs_Node * head = (Abs_Node *)funcs[i]->pure_trees[j]->get_head();
					uint64_t start = head->mem_info.associated_mem->start;
					uint64_t end = head->mem_info.associated_mem->end;

					for (int k = 0; k < dump.size(); k++){
						if (!dump[k]->write){
							if (start >= dump[k]->start && end <= dump[k]->end){
								mem_dump_regions_t * dump_region = new mem_dump_regions_t;
								dump_region->name = head->mem_info.associated_mem->name + "_in";
								dump_region->values = &dump[k]->buffer[start - dump[k]->start];
								dump_region->size = get_actual_size(head->mem_info.associated_mem);
								dumps.push_back(dump_region);
								break;
							}
						}
					}
				}
			}
		}
	}

	ofstream file(get_standard_folder("output") + "\\dump.h");
	for (int i = 0; i < dumps.size(); i++){
		print_dump_to_file(file, dumps[i]); 
	}

	return dumps;

}














