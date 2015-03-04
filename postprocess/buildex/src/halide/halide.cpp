
#include <string>
#include <stack>
#include <algorithm>
#include <list>

#include "halide/halide.h"
#include "trees/nodes.h"
#include "common_defines.h"



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

	ASSERT_MSG((node->type == Abs_Node::INPUT_NODE || node->type == Abs_Node::INTERMEDIATE_NODE), ("ERROR: the node cannot be a input"))

	return "ImageParam " + node->mem_info.associated_mem->name + "(" 
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
		ret += "int" + to_string(node->symbol->width) + "_t";
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

Halide_Program::Func * Halide_Program::check_function(mem_regions_t * mem){

	for (int i = 0; i < funcs.size(); i++){
		Abs_Node * node = static_cast<Abs_Node *>(funcs[i]->pure_trees[0]->get_head());
		if (node->mem_info.associated_mem == mem){
			return funcs[i];
		}
		
		for (int j = 0; j < funcs[i]->reduction_trees.size(); j++){
			vector<Abs_Tree *> trees = funcs[i]->reduction_trees[j].second;
			for (int k = 0; k < trees.size(); k++){
				node = static_cast<Abs_Node *>(trees[k]->get_head());
				if (node->mem_info.associated_mem == mem){
					return funcs[i];
				}
			}
		}
	}

	

	return NULL;

}

bool compare_tree(Abs_Tree * first, Abs_Tree * second){
	return first->conditional_trees.size() < second->conditional_trees.size();
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
	if (node == NULL){
		rdom->red_node = node;
		rdom->type = INDIRECT_REF;
	}
	else{
		rdom->extents = boundaries;
		rdom->type = EXTENTS;
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
		trees = funcs[i]->pure_trees;
		for (int j = 0; j < funcs[i]->reduction_trees.size(); j++){
			trees.insert(trees.end(), funcs[i]->reduction_trees[j].second.begin(),
				funcs[i]->reduction_trees[j].second.begin());
		}
	}

	for (int i = 0; i < trees.size(); i++){
		vector<Abs_Node *> new_input;
		vector<Abs_Node *> compare;

		new_input = para ? trees[i]->retrieve_parameters() : trees[i]->retrieve_input_nodes();
		compare = para ? params : inputs;

		for (int k = 0; k < new_input.size(); k++){
			for (int j = 0; j < compare.size(); j++){
				if (compare[i]->mem_info.associated_mem
					== new_input[k]->mem_info.associated_mem){
					break;
				}
			}

			if (para) params.push_back(new_input[k]);
			else inputs.push_back(new_input[k]);
		}

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

	if (overlap_end == node_end){
		ret += " ( ";
		ret += print_abs_tree(overlap, head, vars);
		ret += " ) & " + to_string(node->symbol->width * 8);
	}
	else{
		ret += " ( ( ";
		ret += print_abs_tree(overlap,head, vars);
		ret += " >> " + to_string((overlap_end - node_end) * 8) + " ) ";
		ret += " ) & " + to_string((node->symbol->width * 8));
	}

	return ret;

}

string Halide_Program::print_partial_overlap_node(Abs_Node * node, Node * head, vector<string> vars){

	string ret = "";

	for (int i = 0; i < node->srcs.size(); i++)
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
	}
	return ret;
}

/************************************************************************/
/* Function and main Halide body printing	                            */
/************************************************************************/

void Halide_Program::print_halide_program(std::ostream &out, vector<string> red_variables){

	DEBUG_PRINT(("printing halide....\n"), 1);

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

string print_output_func_def(Abs_Node * head, vector<string> vars){
	mem_regions_t * mem = head->mem_info.associated_mem;
	string ret = mem->name + "(";
	for (int i = 0; i < head->mem_info.dimensions; i++){
		ret += vars[i];
		if (i == head->mem_info.dimensions - 1){
			ret += ")";
		}
		else{
			ret += ",";
		}
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

	output += " = " + exprs[exprs.size() - 1]->name + ";\n";

	return output;

}

string Halide_Program::print_pure_trees(Func * func){

	return print_predicated_tree(func->pure_trees, "_p_", vars);

}

string Halide_Program::print_rdom(RDom * rdom, vector<string> variables){

	string name = rvars[rvars.size() - 1];
	string ret = "RDom " + name + "(";
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
		ret += print_rdom(func->reduction_trees[0].first, red_variables) + "\n";
		ret += print_predicated_tree(func->reduction_trees[0].second, "_r" + to_string(i) + "_", get_reduction_index_variables(name));
	
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

std::string Halide_Program::print_abs_tree(Node * nnode, Node * head ,vector<string> vars){

	Abs_Node * node = static_cast<Abs_Node *>(nnode);

	string ret = "";
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

		if (node != head){
			ret += node->get_symbolic_string(vars) + " ";
		}
		else{
			if (node->operation != op_assign){  /* the node contains some other operation */
				uint32_t ori_type = node->type;
				node->type = Abs_Node::OPERATION_ONLY;
				ret += print_abs_tree(node, head, vars);
				node->type = ori_type;
			}
			else{
				ret += print_abs_tree(node->srcs[0], head, vars);
			}
			
		}

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












