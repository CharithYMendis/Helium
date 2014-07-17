#include "print_halide.h"
#include "print_common.h"
#include "build_abs_tree.h"
#include <string>
#include <stack>

string get_abs_node_string(Abs_node * node);

/* lots of helper functions - halide specific only */

/*halide header and footer - in future move to a file and read the header from the file*/
string get_halide_header(){
	return "#include \"Halide.h\"\n int main(){ \n";
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
		+ get_string_type(node->width, node->sign, node->type == IMMEDIATE_FLOAT) 
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

	
	cout << node->mem_info.associated_mem->name << endl;

	string ret = node->mem_info.associated_mem->name + "(";
	for (int i = 0; i < node->mem_info.dimensions; i++){
		for (int j = 0; j < node->mem_info.dimensions ; j++){

			cout << node->mem_info.indexes[i][j] << " ";

			if (node->mem_info.indexes[i][j] == 1){
				ret +=  vars[j] + " + ";
			}
			else if (node->mem_info.indexes[i][j] != 0){
				ret += "(" + to_string(node->mem_info.indexes[i][j]) + ")" + " * " + vars[j] + "+";
			}

		}

		cout << node->mem_info.indexes[i][node->mem_info.dimensions] << endl;

		ret += to_string(node->mem_info.indexes[i][node->mem_info.dimensions]);

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

string get_output_to_file_string(Abs_node * node, string filename, uint file_no){
	return node->mem_info.associated_mem->name + ".output_to_file(" + filename + "_" + to_string(file_no) + ");";
}





/* main print functions for the halide module */

void Halide_program::print_abs_tree_in_halide(Abs_node* node, ostream &out){

	/*currently I donot handle FO and POs */
	if (node->type == OPERATION_ONLY){
		if (node->srcs.size() == 1){
			out << " " << get_abs_node_string(node) << " ";
			print_abs_tree_in_halide(node->srcs[0],out);
		}
		else if (node->srcs.size() == 2){
			out << " ( ";
			print_abs_tree_in_halide(node->srcs[0], out);
			out << " " << get_abs_node_string(node) << " ";
			print_abs_tree_in_halide(node->srcs[1], out);
			out << " ) ";
		}
		else{
			/* this should be a full or a partial overlap*/
		}
	}
	else if(node->type == SUBTREE_BOUNDARY){
		return;
	}
	else {
		out << " " << get_abs_node_string(node) << " ";
	}
	

}

void Halide_program::print_function(function* func, ostream &out){

	print_abs_tree_in_halide(func->nodes[0],out);
	out << ";" << endl;

}


/* the public and direct helper methods of Halide program  */

Halide_program::Halide_program(Abs_node * head){
	this->head = head;
}

bool is_function_recursive(vector<Abs_node *> &stack, Abs_node* node){

	ASSERT_MSG(((node->type == INPUT_NODE) || (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)), ("ERROR: the node should be a memory node\n"));

	for (int i = 0; i < stack.size(); i++)
	{
		if ((node->type == stack[i]->type) && (node->mem_info.dimensions == stack[i]->mem_info.dimensions)){
			bool same = true;
			for (int j = 0; j < stack[i]->mem_info.dimensions; j++){
				if (stack[i]->mem_info.pos[j] != node->mem_info.pos[j]){
					same = false;
					break;
				}
			}
			if (same) return true;
		}
	}

	return false;
}

void Halide_program::print_input_params(Abs_node * node, ostream &out, vector<uint> &values){

	/*assume that all these input parameters are of single dimension*/

	if (node->type == PARAMETER){
		if (find(values.begin(), values.end(), node->value) == values.end()){
			out << get_parameter_definition_string(node) << endl;
			values.push_back(node->value);
		}
	}

	for (int i = 0; i < node->srcs.size(); i++){
		print_input_params(node->srcs[i], out, values);
	}
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

void Halide_program::seperate_to_Funcs(Abs_node * node, vector<Abs_node *> stack){


	bool pushed = false;

	if ((node->type == INPUT_NODE) || (node->type == OUTPUT_NODE) || (node->type == INTERMEDIATE_NODE)){

		Halide_program::function * func = check_function(node->mem_info.associated_mem);
		if (!is_function_recursive(stack, node)){
			if (func == NULL)
			{
				func = new Halide_program::function();
				func->nodes.push_back(node);
				func->recursive = false;
			}
			else{
				func->nodes.push_back(node);
			}
			stack.push_back(node);
			pushed = true;
		}
		else{
			func->recursive = true;
		}
	}

	for (int i = 0; i < node->srcs.size(); i++){
		seperate_to_Funcs(node->srcs[i], stack);
	}

	if (pushed){
		stack.pop_back();
	}

}

void Halide_program::print_halide(ostream &out){


	/*print the Halide header*/
	out << get_halide_header() << endl;

	vector<Halide_program::function *> outputs;

	/* print the vars declarations - get a random func and then print out the vars for the dimension of it */
	vector<string> vars = get_vars(funcs[0]->nodes[0]->mem_info.dimensions);
	for (int i = 0; i < vars.size(); i++){
		out << "Vars " << vars[i] << ";" << endl;
	}


	/*print the function declarations*/
	for (int i = funcs.size() - 1; i >= 0; i--){
		Abs_node * node = funcs[i]->nodes[0];
		if (node->type == OUTPUT_NODE || node->type == INTERMEDIATE_NODE){
			out << get_function_string(funcs[i]) << endl;
			if (node->type == OUTPUT_NODE){
				outputs.push_back(funcs[i]);
			}
		}
		else{
			out << get_input_definition_string(node) << endl;
		}
	}

	/*print the actual function definitions*/
	for (int i = funcs.size() - 1; i >= 0; i--){
		Abs_node * node = funcs[i]->nodes[0];
		if (node->type == OUTPUT_NODE || node->type == INTERMEDIATE_NODE){
			print_function(funcs[i],out); 
		}
	}

	/*output to file the functions*/
	for (int i = 0; i < outputs.size(); i++){
		out << get_output_to_file_string(outputs[i]->nodes[0], "halide_out_", i) << endl;
	}

	/*print the halide footer*/
	out << get_halide_footer() << endl;
}





