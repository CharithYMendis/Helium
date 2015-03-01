 #ifndef _HALIDEBACKEND_H
 #define _HALIDEBACKEND_H
 
#include <stdint.h>
#include <vector>
#include <ostream>

#include "halide/halide.h"
#include "trees/trees.h"
#include "memory/memregions.h"


/* main halide program for generation Halide code - we also need a helper halide module to setup buffer_t for patching */
class Halide_Program {

	/* need to model the Halide constructs that we are using */

public:

	/* RDoms are not supported as of yet */

	/* only pure parameters entering the function */
	struct Param{
		bool sign;
		uint32_t width;
		uint32_t dimension;
	};

	/* input, output and intermediate states that can be modelled as a Halide function*/
	/* when ever there are reduction trees, then pure definitions will act as the initial updates of the function */
	enum RDom_type {
		INDIRECT_REF,
		EXTENTS,
	};
	
	
	struct RDom{
		std::string name;
		RDom_type type;
		Abs_Node * red_node;
		std::vector< std::pair<int32_t, int32_t> > extents;

		std::vector< std::vector<int32_t> > abstract_indexes;
	};



	struct Func {
		std::vector< std::pair< RDom *, std::vector<Abs_Tree *> > > reduction_trees;
		std::vector<Abs_Tree *> pure_trees; /* for each conditional branch */
		uint32_t variations;
	};

	/* select expressions for input dependent conditionals */
	struct Select_Expr {
		std::string name;
		std::string condition;
		std::string truth_value;
	};

	
	std::vector<Abs_Node *> inputs;
	std::vector<Abs_Node *> params;
	std::vector<std::string> vars;
	std::vector<std::string> rvars;

	std::vector<Func *> funcs;
	std::vector<Abs_Node *> output; /* this is used to populate the arguments string */

	
public:

	/* methods to manipulate and fill the Halide program */
	Halide_Program();

	/* Halide program population */
	void populate_vars(uint32_t dim);
	
	void populate_pure_funcs(Abs_Tree * tree);
	void populate_pure_funcs(std::vector<Abs_Tree *> trees); 
	void populate_red_funcs(Abs_Tree * tree,
		std::vector< std::pair<int32_t, int32_t > > boundaries, Abs_Node * node);
	
	void populate_input_params();
	void populate_params();
	
	void populate_outputs(Abs_Node * node);

	void populate_halide_program();

	/* Main halide program printing function */
	void print_halide_program(std::ostream &file, std::vector<std::string> red_variables);


private:

	/* private populating functions */
	void populate_paras_input_paras(bool which);

	/* printing functions */
	std::string print_function(Func * func, std::vector<std::string> red_variables);
	std::string print_pure_trees(Func * func);
	std::string print_red_trees(Func * func, std::vector<std::string> red_variables);
	std::string print_predicated_tree(std::vector<Abs_Tree *> trees, std::string expr_tag, std::vector<std::string> vars);

	/* print out sub parts of the Halide function */
	std::string print_abs_tree(Node * node, Node * head, std::vector<string> vars);
	std::string print_conditional_trees(std::vector< std::pair<Abs_Tree *, bool > > conditions,
		std::vector<string> vars);

	/* printing full and partial overlap nodes */
	std::string print_full_overlap_node(Abs_Node * node, Node * head, std::vector<string> vars);
	std::string print_partial_overlap_node(Abs_Node * node, Node * head, std::vector<string> vars);

	/* auxiliary functions */
	Func *check_function(mem_regions_t * mem);
	void sort_functions();
	void populate_input_params(Abs_Node * node);
	std::string print_rdom(RDom * rdom, std::vector<std::string> variables);
	int32_t get_rdom_location(Func *func, RDom *rdom);




};





#endif