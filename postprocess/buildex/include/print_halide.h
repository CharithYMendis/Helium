#ifndef _HALIDE_BACKEND_H
#define _HALIDE_BACKEND_H

#include "tree_transformations.h"
#include "build_mem_instrace.h"
#include "build_mem_dump.h"
#include <vector>
#include "build_abs_tree.h"
#include "defines.h"

string get_abs_node_string(Abs_node * node);

/* main halide program for generation Halide code - we also need a helper halide module to setup buffer_t for patching */
class Halide_program {

	/* need to model the Halide constructs that we are using */

public:

	/* RDoms are not supported as of yet */

	/* only pure parameters entering the function */
	struct parameter{
		bool sign;
		uint width;
		uint dimension;
	};

	/* input, output and intermediate states that can be modelled as a Halide function*/
	struct function {
		vector<Abs_node *> nodes; 
		bool recursive;
	};

	Abs_node * head;
	vector<function *> funcs;

public:

	/* methods to manipulate and fill the Halide program */
	Halide_program(Abs_node * head);
	void get_input_params(Abs_node * node);
	void seperate_to_Funcs(Abs_node * node, vector<Abs_node *> &stack);
	void print_halide(ostream &out);
	void print_seperated_funcs();
	


private:

	function * check_function(mem_regions_t * mem);
	void print_function(function* func, ostream &out);
	void print_input_params(Abs_node * node, ostream &out, vector<uint> &values, vector<string> &inputs);

	void print_abs_tree_in_halide(Abs_node* node, Abs_node* head, ostream &out);
	void print_full_overlap_string(Abs_node * node, Abs_node* head, ostream &out);
	void print_partial_overlap_string(Abs_node * node, Abs_node* head, ostream &out);

	
	


};


#endif