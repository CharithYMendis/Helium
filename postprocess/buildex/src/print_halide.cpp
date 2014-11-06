#include "print_halide.h"
#include "print_common.h"
#include "build_abs_tree.h"
#include <string>
#include <stack>
#include <algorithm>
#include <list>

string get_abs_node_string(Abs_node * node);

/* lots of helper functions - halide specific only */

/*halide header and footer - in future move to a file and read the header from the file*/
string get_halide_header(){
	return "#include <Halide.h>\n  #include <vector>\n  using namespace std;\n  using namespace Halide;\n  int main(){ \n";
}

string get_halide_footer(){
	return "return 0;\n}";
}

/* halide supported types - Int, UInt , Float with width specified as the first parameter to the type */
string get_string_type(uint width, bool sign, bool is_float){

	string sign_string = sign ? "" : "U"; 

	if (!is_float)
		return sign_string + "Int(" + to_string(width * 8) + ")";
	else
		return "Float(" + to_string(width * 8) + ")";

}

/* ImageParam name(type,dim) */
string get_input_definition_string(Abs_node * node){

	return "ImageParam " + node->mem_info.associated_mem->name + "(" 
		+ get_string_type(node->width, node->sign, node->type == IMMEDIATE_FLOAT) + ","
		+ to_string(node->mem_info.dimensions) + ");" ;

}

string get_parameter_definition_string(Abs_node * node){

	return "ImageParam " + get_abs_node_string(node)  + "("
		+ get_string_type(node->width, node->sign, node->type == IMMEDIATE_FLOAT)
		+  "1);";

}

string get_function_string(Halide_program::function * func){
	Abs_node * node = func->nodes[0];
	return "Func " + node->mem_info.associated_mem->name + ";";
}


/* get memory in the form of mem(x,y,z) etc. */
string get_mem_string(Abs_node * node, vector<string> vars){

	string ret = node->mem_info.associated_mem->name + "(";
	for (int i = 0; i < node->mem_info.dimensions; i++){

		bool first = true;
		for (int j = 0; j < node->mem_info.dimensions ; j++){

	
			if (node->mem_info.indexes[i][j] == 1){
				if (!first){
					ret += "+";
				}
				ret += vars[j];
				first = false;
				
			}
			else if (node->mem_info.indexes[i][j] != 0){
				if (!first){
					ret += "+";
				}
				ret += "(" + to_string(node->mem_info.indexes[i][j]) + ")" + " * " + vars[j];
				first = false;
				
			}

		}

		if (node->mem_info.indexes[i][node->mem_info.dimensions] != 0){
			if (!first){
				ret += "+";
			}
			ret += to_string(node->mem_info.indexes[i][node->mem_info.dimensions]);
		}

		if (i != node->mem_info.dimensions - 1){
			ret += ",";
		}
		else{
			
			ret += ")";
		}
		
	}

	return ret;

}

vector<string> get_vars(uint dim){
	vector<string> ret;
	string x = "x";
	for (int i = 0; i < dim; i++){
		ret.push_back(x + + "_" +  to_string(i)); 
	}

	return ret;
}

string get_abs_node_string(Abs_node * node){

	if ((node->type == INPUT_NODE) || (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)){
		return get_mem_string(node, get_vars(node->mem_info.dimensions));
	}
	else{
		return abs_node_to_string(node);
	}

}

string get_output_to_file_string(Abs_node * node, string filename, uint file_no,vector<string> params){

	string ret = "vector<Argument> args;\n";
	for (int i = 0; i < params.size(); i++){
		ret += "args.push_back(" + params[i] + ");\n";
	}

	return ret + node->mem_info.associated_mem->name + ".compile_to_file(\"" + filename + "_" + to_string(file_no) + "\",args);";
}

void Halide_program::print_full_overlap_string(Abs_node * node, Abs_node * head, ostream &out){
	/* here, we will some times we need to use shifting and anding */
	Abs_node * overlap = node->srcs[0];

	uint overlap_end = overlap->range + overlap->width;
	uint node_end = node->range + node->width;

	//if (overlap_end == node_end){
		out << " ( "; 
		print_abs_tree_in_halide(overlap, head, out);
		out << " ) & " <<  ( uint32_t(~0) >> (32 - node->width * 8) ) ;
	//}
	/*else{
		out << " ( ( ";
		print_abs_tree_in_halide(overlap, head, out);
		out << " >> " << (overlap_end - node_end) * 8  << " ) ";
		out << " ) & " << (node->width * 8);
	}*/

}

void Halide_program::print_partial_overlap_string(Abs_node * node, Abs_node * head, ostream &out){

	for (int i = 0; i < node->srcs.size(); i++)
	{
		Abs_node * overlap = node->srcs[i];

		uint overlap_end = overlap->range + overlap->width;
		uint node_end = node->range + node->width;

		out << "(";
		print_abs_tree_in_halide(overlap, head, out);
		out << " << " << (node_end - overlap_end) * 8 ;
		out << ")";

		if (i != node->srcs.size() - 1){
			out << " | ";
		}
	}
}

string get_cast_string(uint32_t width, uint32_t sign){
	
	string ret = "";
	ret += "cast<";
	if (!sign) ret += "u";
	ret += "int" + to_string(width * 8) + "_t>";
	return ret;
}

void Halide_program::print_abs_tree_in_halide(Abs_node* node, Abs_node * head, ostream &out){


	if (node->type == OPERATION_ONLY){
		if (node->operation == op_full_overlap){
			out << " ( ";
			print_full_overlap_string(node, head, out);
			out << " ) ";
		}
		else if (node->operation == op_partial_overlap){
			out << " ( ";
			print_partial_overlap_string(node, head, out);
			out << " ) ";
		}
		else if (node->operation == op_split_h){
			out << " ( ";
			print_abs_tree_in_halide(node->srcs[0], head, out);
			out << " ) >> ( " << to_string(node->srcs[0]->width * 8 / 2) << ")";
		}
		else if (node->operation == op_split_l){
			out << " ( ";
			print_abs_tree_in_halide(node->srcs[0], head, out);
			out << " ) & " << to_string((node->srcs[0]->width / 2) * 8);
		}
		else if (node->srcs.size() == 1){
			out << " " << get_abs_node_string(node) << " ";
			print_abs_tree_in_halide(node->srcs[0], head, out);
		}
		else{

			out << "(";
			for (int i = 0; i < node->srcs.size(); i++){

				if (node->srcs[i]->width < node->width){
					out << get_cast_string(node->width, false) << "(";
				}
				print_abs_tree_in_halide(node->srcs[i], head, out);
				if (node->srcs[i]->width < node->width){
					out << ")";
				}
				if (i != node->srcs.size() - 1){
					out << " " << get_abs_node_string(node) << " ";
				}
			}
			out << ")";
		}
	}
	else if (node->type == SUBTREE_BOUNDARY){
		return;
	}
	else {
		out << " " << get_abs_node_string(node) << " ";
		/* head node is special and this should be printed out in a special manner */
		if (node == head){
			if (node->operation != op_assign){  /* the node contains some other operation */
				out << " = ";
			}
			uint ori_type = node->type;
			node->type = OPERATION_ONLY;
			print_abs_tree_in_halide(node, head, out);
			node->type = ori_type;
		}

	}


}

string Halide_program::get_full_overlap_string(Abs_node * node, Abs_node * head){

	string ret = "";
	Abs_node * overlap = node->srcs[0];

	uint overlap_end = overlap->range + overlap->width;
	uint node_end = node->range + node->width;

	//if (overlap_end == node_end){
	ret += " ( ";
	ret += get_abs_tree_string(overlap, head);
	ret += " ) & " + to_string((uint32_t(~0) >> (32 - node->width * 8)));
	//}
	/*else{
	out << " ( ( ";
	print_abs_tree_in_halide(overlap, head, out);
	out << " >> " << (overlap_end - node_end) * 8  << " ) ";
	out << " ) & " << (node->width * 8);
	}*/

	return ret;

}

string Halide_program::get_partial_overlap_string(Abs_node * node, Abs_node * head){

	string ret = "";

	for (int i = 0; i < node->srcs.size(); i++)
	{
		Abs_node * overlap = node->srcs[i];

		uint overlap_end = overlap->range + overlap->width;
		uint node_end = node->range + node->width;

		ret += "(";
		ret += get_abs_tree_string(overlap, head);
		ret += " << " + to_string((node_end - overlap_end) * 8);
		ret += ")";

		if (i != node->srcs.size() - 1){
			ret += " | ";
		}
	}

	return ret;


}

string Halide_program::get_abs_tree_string(Abs_node * node, Abs_node * head){

	string ret = "";

	if (node->type == OPERATION_ONLY){


		if (node->operation == op_full_overlap){
			ret += " ( ";
			ret += get_full_overlap_string(node, head);
			ret += " ) ";
		}
		else if (node->operation == op_partial_overlap){
			ret += " ( ";
			ret += get_partial_overlap_string(node, head);
			ret += " ) ";
		}
		else if (node->operation == op_split_h){
			ret += " ( ";
			ret += get_abs_tree_string(node->srcs[0], head);
			ret += " ) >> ( " + to_string(node->srcs[0]->width * 8 / 2) + ")";
		}
		else if (node->operation == op_split_l){
			ret += " ( ";
			ret += get_abs_tree_string(node->srcs[0], head);
			ret +=  " ) & " + to_string((node->srcs[0]->width / 2) * 8);
		}
		else if (node->srcs.size() == 1){
			ret += " " + get_abs_node_string(node) + " ";
			ret += get_abs_tree_string(node->srcs[0], head);
		}
		else{

			ret += "(";
			for (int i = 0; i < node->srcs.size(); i++){

				if (node->srcs[i]->width < node->width){
					ret += get_cast_string(node->width, false) + "(";
				}
				ret += get_abs_tree_string(node->srcs[i], head);
				if (node->srcs[i]->width < node->width){
					ret += ")";
				}
				if (i != node->srcs.size() - 1){
					ret += " " + get_abs_node_string(node) + " ";
				}
			}
			ret += ")";
		}
	}
	else if (node->type == SUBTREE_BOUNDARY){
		return ret;
	}
	else {


		ret += " " + get_abs_node_string(node) + " ";
		/* head node is special and this should be printed out in a special manner */
		if (node == head){
			if (node->operation != op_assign){  /* the node contains some other operation */
				ret += " = ";
			}
			uint ori_type = node->type;
			node->type = OPERATION_ONLY;
			ret += get_abs_tree_string(node, head);
			node->type = ori_type;
		}

	}

	return ret;

}

/* main print functions for the halide module */


void Halide_program::print_function(function* func, ostream &out){


	Abs_node * node = func->nodes[0];

	/* for the first node set indexes to zero -> this will make it print correctly and finally restore */
	uint ** indexes = new uint *[node->mem_info.dimensions];
	for (int i = 0; i < node->mem_info.dimensions; i++){
		indexes[i] = new uint[node->mem_info.dimensions + 1];
	}

	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions + 1; j++){
			indexes[i][j] = node->mem_info.indexes[i][j];
			if (i == j){
				node->mem_info.indexes[i][j] = 1;
			}
			else{
				node->mem_info.indexes[i][j] = 0;
			}

		}
	}
	

	print_abs_tree_in_halide(node, node, out);

	/* restore back the indexes */
	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions + 1; j++){
			node->mem_info.indexes[i][j] = indexes[i][j];
		}
	}

	/* clean up */
	for (int i = 0; i < node->mem_info.dimensions; i++){
		delete[] indexes[i];
	}
	delete indexes;

	out << ";" << endl;

}


/* the public and direct helper methods of Halide program  */


Halide_program::Halide_program(){

}

Halide_program::Halide_program(Abs_node * head){
	this->head = head;
}

void Halide_program::print_input_params(Abs_node * node, ostream &out, vector<uint> &values, vector<string> &inputs){

	/*assume that all these input parameters are of single dimension*/

	if (node->type == PARAMETER){
		if (find(values.begin(), values.end(), node->value) == values.end()){
			out << get_parameter_definition_string(node) << endl;
			inputs.push_back(get_abs_node_string(node));
			values.push_back(node->value);
		}
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_input_params(node->srcs[i], out, values, inputs);
	}
}

bool is_function_recursive(vector<Abs_node *> &stack, Abs_node* node){

	ASSERT_MSG(((node->type == INPUT_NODE) || (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)), ("ERROR: the node should be a memory node\n"));

	for (int i = 0; i < stack.size(); i++)
	{
		ASSERT_MSG((stack[i]->type != INPUT_NODE), ("ERROR: input nodes should be immediately removed from the stack at the exit\n"));
		if ((node->type == stack[i]->type) && (node->mem_info.dimensions == stack[i]->mem_info.dimensions)){
			bool same = (stack[i]->mem_info.associated_mem == node->mem_info.associated_mem);
			if (same) return true;
		}
	}

	return false;
}



Halide_program::function * Halide_program::check_function(mem_regions_t * mem){

	for (int i = 0; i < funcs.size(); i++){
		Abs_node * node = funcs[i]->nodes[0];
		if (node->mem_info.associated_mem == mem){
			return funcs[i];
		}
	}

	return NULL;

}

void Halide_program::find_recursive_funcs(){

}

void Halide_program::register_funcs(Abs_node * comp_node, vector< pair<Abs_node *, bool > > cond_nodes){

	Halide_program::function * func = check_function(comp_node->mem_info.associated_mem);
	if (func == NULL){
		func = new Halide_program::function();
		funcs.push_back(func);
	}


	uint32_t size_cond = cond_nodes.size();
	uint32_t insert_at = 0;
	bool inserted = false;
	for (int i = 0; i < func->nodes.size(); i++){
		if (size_cond > func->conditional_nodes[i].size()){
			func->nodes.insert(func->nodes.begin() + i, comp_node);
			func->conditional_nodes.insert(func->conditional_nodes.begin() + i, cond_nodes);
			insert_at = i;
			inserted = true;
			break;
		}
	}

	if (!inserted){
		func->nodes.push_back(comp_node);
		func->conditional_nodes.push_back(cond_nodes);
	}
	
}

void Halide_program::register_inputs(Abs_node * node){

	if ((node->type == INPUT_NODE)){
		Halide_program::function * func = check_function(node->mem_info.associated_mem);
		if (func == NULL){
			func = new Halide_program::function();
			func->nodes.push_back(node);
			inputs.push_back(func);
		}
	}
	for (int i = 0; i < node->srcs.size(); i++){
		register_inputs(node->srcs[i]);
	}
}


string Halide_program::get_conditional_string(vector< pair<Abs_node *, bool> > conditions){

	string ret = "";

	for (int i = 0; i < conditions.size(); i++){
		ASSERT_MSG((conditions[i].first->srcs.size() == 1), ("ERROR: expected single source\n"));
		Abs_node * node = conditions[i].first;
		bool taken = conditions[i].second;

		if (!taken){
			ret += "! (";
		}

		ret += get_abs_tree_string(conditions[i].first->srcs[0], conditions[i].first);
		if (i != conditions.size() - 1){
			ret += " && ";
		}

		if (!taken){
			ret += ")";
		}
	}

	return ret;


}

string get_output_func_string(Halide_program::function * func, vector<string> vars){
	mem_regions_t * mem = func->nodes[0]->mem_info.associated_mem;
	string ret = mem->name + "(";
	for (int i = 0; i < vars.size(); i++){
		ret += vars[i];
		if (i == vars.size() - 1){
			ret += ")";
		}
		else{
			ret += ",";
		}
	}
	return ret;

}

string get_expression_name(mem_regions_t * region, uint32_t function, uint32_t conditional){

	return region->name + "_" + to_string(function) + "_" + to_string(conditional);

}

struct Expression{
	string name;
	string condtions;
	string tree;

};


vector<string> get_expression_strings(vector<Expression *> exprs){

	vector<string> ret;

	/* printing out the expressions */
	for (int i = 0; i < exprs.size() - 1 ; i++){
		string ret_string = "";
		ASSERT_MSG((exprs[i]->condtions != ""), ("ERROR: if there are more than one node then there should be conditionals\n"));
		ret_string += "Expr " + exprs[i]->name + " = select(" + exprs[i]->condtions + "," + exprs[i]->tree + "," + exprs[i + 1]->name + ");";
		ret.push_back(ret_string);
	}

	uint32_t fin = exprs.size() - 1;
	ret.push_back("Expr " + exprs[fin]->name + " = " + exprs[fin]->tree + ";");
	return ret;

}


bool compareNoCase(string first, string second)
{
	ULONG i = 0;
	while ((i < first.length()) && (i < second.length()))
	{
		if (tolower(first[i]) < tolower(second[i])) return true;
		else if (tolower(first[i]) > tolower(second[i])) return false;
		i++;
	}

	if (first.length() < second.length()) return true;
	else return false;
}


void get_parameters_from_tree(Abs_node * tree, vector<string> &para){


	if (tree->type == PARAMETER){

		bool found = false;
		string para_string = "p_" + to_string(tree->value); 
		para.push_back(para_string);
	}

	for (int i = 0; i < tree->srcs.size(); i++){
		get_parameters_from_tree(tree->srcs[i], para); 
	}
	
}


vector<string> get_parameters_from_functions(vector<Halide_program::function * > func){


	vector<string> paras;

	for (int i = 0; i < func.size(); i++){
		for (int j = 0; j < func[i]->nodes.size(); j++){

			get_parameters_from_tree(func[i]->nodes[j], paras);

			for (int k = 0; k < func[i]->conditional_nodes[j].size(); k++){
				get_parameters_from_tree(func[i]->conditional_nodes[j][k].first, paras);
			}

		}
	}

	cout << paras.size() << endl;
	for (int i = 0; i < paras.size(); i++){
		cout << paras[i] << endl;
	}

	return paras;

}


void Halide_program::print_halide_v2(ostream &out){

	DEBUG_PRINT(("printing halide....\n"), 1);

	/*print the Halide header*/
	out << get_halide_header() << endl;

	vector<Halide_program::function *> outputs;

	/* print the vars declarations - get a random func and then print out the vars for the dimension of it */
	vector<string> vars = get_vars(funcs[0]->nodes[0]->mem_info.dimensions);
	for (int i = 0; i < vars.size(); i++){
		out << "Var " << vars[i] << ";" << endl;
	}

	/* print the input parameters */
	vector<uint> params;
	vector<string> param_strings;


	vector<string> paras = get_parameters_from_functions(funcs); 
	for (int i = 0; i < paras.size(); i++){
		out << "Param<uint32_t> " << paras[i] << ";" << endl;
	}
	//print_input_params(head, out, params, param_strings);


	/* these are only declarartions */
	/*print the function declarations*/
	for (int i = funcs.size() - 1; i >= 0; i--){
		Abs_node * node = funcs[i]->nodes[0];
		if ((node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)){
			out << get_function_string(funcs[i]) << endl;
			DEBUG_PRINT(("function node - %s\n", node->mem_info.associated_mem->name.c_str()), 1);
		}
		outputs.push_back(funcs[i]);
	}

	/* print the input declarations */
	vector<string> inputs_s;

	for (int i = 0; i < inputs.size(); i++){
		Abs_node * node = inputs[i]->nodes[0];

		bool found = false;
		for (int j = 0; j < i; j++){
			Abs_node * comp = inputs[j]->nodes[0];
			if (comp->mem_info.associated_mem == node->mem_info.associated_mem){
				found = true;
				break;
			}
		}

		if (found) continue;

		//DEBUG_PRINT(("input param node - %s\n", node->mem_info.associated_mem->name.c_str()), 1);
		inputs_s.push_back(get_input_definition_string(node));
		param_strings.push_back(node->mem_info.associated_mem->name);
	}



	for (int i = 0; i < inputs_s.size(); i++){
		out << inputs_s[i] << endl;
	}

	
	

	/*print the actual function definitions - assumes the abs_nodes are stored based on the number of conditional statements*/
	for (int i = funcs.size() - 1; i >= 0; i--){

		function * func = funcs[i];
		mem_regions_t * mem = func->nodes[0]->mem_info.associated_mem;
		vector<Expression *> exprs;


		/* print out the function start and say expression e */
		for (int j = 0; j < func->nodes.size(); j++){
			Abs_node * tree_node = func->nodes[j];
			vector< pair<Abs_node *, bool > >  conditionals = func->conditional_nodes[j];
			Expression * expr = new Expression();
			expr->name = get_expression_name(mem, i, j);
			cout << expr->name << endl;
			expr->condtions = get_conditional_string(conditionals);
			cout << expr->condtions << endl;
			expr->tree = get_abs_tree_string(tree_node->srcs[0], tree_node);
			cout << expr->tree << endl;
			exprs.push_back(expr);
		}


		/* printing out the expressions */
		vector<string> expr_strings = get_expression_strings(exprs); 

		for (int j = expr_strings.size() - 1; j >= 0; j--){
			out << expr_strings[j] << endl;
		}

		out << get_output_func_string(func, vars) << " = " << "cast<uint8_t>( " << exprs[0]->name << ") ;" << endl;

	}

	/* for params */
	for (int i = 0; i < paras.size(); i++){
		out << "args.push_back(" << paras[i] << ");" << endl;
	}

	/*output to file the functions*/
	for (int i = 0; i < outputs.size(); i++){
		out << get_output_to_file_string(outputs[i]->nodes[0], "halide_out", i, param_strings) << endl;
	}



	/*print the halide footer*/
	out << get_halide_footer() << endl;

}





void Halide_program::seperate_to_Funcs(Abs_node * node, vector<Abs_node *> &stack){


	bool pushed = false;

	if ((node->type == INPUT_NODE) || (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)){


		cout << get_mem_string(node, get_vars(node->mem_info.dimensions)) << endl;

		Halide_program::function * func = check_function(node->mem_info.associated_mem);
		if (!is_function_recursive(stack, node)){
			if (func == NULL)
			{
				func = new Halide_program::function();
				func->recursive = false;
				funcs.push_back(func);
			}
		}
		else{
			func->recursive = true;
		}

		func->nodes.push_back(node);
		pushed = true;
		stack.push_back(node);

	}

	for (int i = 0; i < node->srcs.size(); i++){
		seperate_to_Funcs(node->srcs[i], stack);
	}

	if (pushed){
		stack.pop_back();
	}

}

void Halide_program::print_seperated_funcs(){

	cout << " Number of Halide functions - " << funcs.size() << endl;

	for (int i = 0; i < funcs.size(); i++){
		Abs_node * node = funcs[i]->nodes[0];
		cout << i + 1 << ": " << node->mem_info.associated_mem->name << " " ;
		if (funcs[i]->recursive){
			cout << "recursive";
		}
		cout << endl;
		cout << " no of nodes - " << funcs[i]->nodes.size() << endl;
		for (int j = 0; j < funcs[i]->nodes.size(); j++){
			cout << get_mem_string(funcs[i]->nodes[j], get_vars(funcs[i]->nodes[j]->mem_info.dimensions)) << endl;
		}
		cout << "------------------------------------------" << endl;
	}


}

void Halide_program::print_halide(ostream &out){

	DEBUG_PRINT(("printing halide....\n"), 1);

	/*print the Halide header*/
	out << get_halide_header() << endl;

	vector<Halide_program::function *> outputs;

	/* print the vars declarations - get a random func and then print out the vars for the dimension of it */
	vector<string> vars = get_vars(funcs[0]->nodes[0]->mem_info.dimensions);
	for (int i = 0; i < vars.size(); i++){
		out << "Var " << vars[i] << ";" << endl;
	}

	/* print the input parameters */
	vector<uint> params;
	vector<string> param_strings;

	print_input_params(head, out, params, param_strings);

	/*print the function declarations*/
	for (int i = funcs.size() - 1; i >= 0; i--){
		Abs_node * node = funcs[i]->nodes[0];
		if ((node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)){
			out << get_function_string(funcs[i]) << endl;
			DEBUG_PRINT(("function node - %s\n",node->mem_info.associated_mem->name.c_str()),1);
			outputs.push_back(funcs[i]); /* remember this should be only outputs? but we have included intermediates as well */
		}
		else{
			DEBUG_PRINT(("input param node - %s\n", node->mem_info.associated_mem->name.c_str()), 1);
			out << get_input_definition_string(node) << endl;
			param_strings.push_back(node->mem_info.associated_mem->name);
		}
	}



	/*print the actual function definitions*/
	for (int i = funcs.size() - 1; i >= 0; i--){
		Abs_node * node = funcs[i]->nodes[0];
		if ( (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE) ){
			print_function(funcs[i],out); 
		}
	}

	/*output to file the functions*/
	for (int i = 0; i < outputs.size(); i++){
		out << get_output_to_file_string(outputs[i]->nodes[0], "halide_out", i, param_strings) << endl;
	}

	/*print the halide footer*/
	out << get_halide_footer() << endl;

}





