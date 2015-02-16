 #ifndef _NODE_H
 #define _NODE_H
 
#include  "..\..\..\dr_clients\include\output.h"
#include <vector>
#include <stdint.h>

#define NODE_RIGHT	1
#define NODE_LEFT	2
#define NODE_NONE   3

#define MAX_REFERENCES 10

//using namespace std;


/* static information storage for dynamically executed instructions - this data structure will summarize 
all the static information found about dynamically executed instructions */

class Static_Info {

public:

	enum Instr_type {

		INPUT = 1 << 0,
		INPUT_DEPENDENT_DIRECT = 1 << 1,
		INPUT_DEPENDENT_INDIRECT = 1 << 2,
		CONDITIONAL = 1 << 3,
		LOOP = 1 << 4,
		OUTPUT = 1 << 5,

	};

	class Jump_Info {

		uint32_t cond_pc; // eflags set by this pc
		uint32_t target_pc; // the target pc of this jump
		uint32_t fall_pc;   // the fall-through pc 
		uint32_t merge_pc;  // the merge point for the taken and not taken paths

		// example lines in the instruction trace for taken and notTaken jump conditionals
		uint32_t taken; 
		uint32_t not_taken;

	};

	uint32_t module_no; // module for this instruction (we encode as integers)
	uint32_t pc; // the program counter value for this instruction - for a jump instruction this is the jump pc
	std::string disassembly; // disassembly string

	Instr_type type; // type of the instruction
	Jump_Info * jump_info; // for loop and conditional constructs
	

	Static_Info();
	~Static_Info();



};
 
 class Node{

	public:

		/* the main identification material for the node */
		int operation;
		bool sign; // indicates whether the operation is signed
		operand_t * symbol;  // operand information for this node - this is concrete

		std::vector<Node *> srcs; // forward references also srcs of the destination
		std::vector<Node *> prev; // keep the backward references
		std::vector<uint> pos;	 // position of the parent node's srcs list for this child 

		
		uint32_t pc;
		uint32_t line; /*variable for debugging*/

		/*auxiliary variables*/
		int order_num;
		int para_num; 
		bool is_para;
		bool is_double;
		
	public:
		
		virtual void print_node(std::ostream &out) = 0;

		/* following routines will have concrete implementations; but can be overloaded */
		/* tree transformation routines */

		/* (node -> ref) => (node) */
		bool remove_forward_ref(Node *ref);
		/* (ref -> node) => (node) */
		void remove_backward_ref(Node *ref);
		/* node => (node->ref) */
		void add_forward_ref(Node * ref);
		/* (dst -> node -> src)  => (dst -> src) */
		void remove_intermediate_ref(Node * dst, Node *src);
		/*safely delete*/
		void safely_delete(Node * head);
		/* removes all srcs */
		void remove_forward_all_srcs();

		/* node canonicalizing routines */
 private:
		/* congregate nodes with associative operations */
		void congregate_node();
		/* order the srcs of the node */
		void order_node(); 
		
 public:
		/* canonicalize node */
		void canonicalize_node();

};

 /* condensed list of rewrite rules */

 /* abs node types */


 class Conc_Node : public Node {

 public:
	 Conc_Node(operand_t * symbol);
	 Conc_Node(uint32_t type, uint64_t value, uint32_t width, float float_value);
	 Conc_Node();
	 ~Conc_Node();

	 void print_node();
 };



 /* type and processing explanation

 abs_nodes types and processing are determined by the Node symbol types and the mem_regions types, therefore, we can see as building the abs tree
 as a unification of the information extracted by the instrace(building the concrete tree) and memdump(locating image locations and properties). Further
 abstraction can be performed on this tree.

 type(Abs_node) <- unify ( type(Node->symbol) , type(mem_region) )

 Next, we assume that the bottom of the tree should be Heap Nodes (image dependancies), Stack or Regs(parameters) or immediate ints, immediate floats

 if (Node->symbol) is heap
 *check for associated memory region -> this should be non null
 *get the x,y,c co-ordinates of the symbol value and store in the struct mem_info
 *mem_info struct
 -> associated mem  - mem_region
 -> dimensions - the dimensionality of the mem_region -> this depends on the image layout
 -> indexes - this carries the final abstracted out index coefficients when we express the filter algebrically
 -> pos - this lifts the flat memory address into coordinates in the dimensional space
 *type will depend on whether the associated mem region is input, output or intermediate

 if (Node->symbol is reg or stack)
 *parameter? Need to identify statically
 *operation_only? If not parameter

 if(Node->symbol immed int or float)

 subtree boundary - check for similar subtrees that assign to intermediate nodes

 */

 class Abs_Node : public Node {

 public:

	 enum Abs_Type {

		OPERATION_ONLY,
		INPUT_NODE,
		OUTPUT_NODE,
		INTERMEDIATE_NODE,
		IMMEDIATE_INT,
		IMMEDIATE_FLOAT,
		PARAMETER,
		UNRESOLVED_SYMBOL,
		SUBTREE_BOUNDARY,
	 
	 };


	 /* some type information about the node - what type of node is this */
	 uint type;
	 
	struct {
		mem_regions_t * associated_mem;
		uint32_t dimensions;
		int ** indexes; /* 2-dimensional array */
		int * pos;
	} mem_info;
	 

	 //add the copy constructor when needed
	 Abs_Node();
	 ~Abs_Node();
	 void print_node();

 };

 class Comp_Abs_Node : public Node {

 public:
	 std::vector<Abs_Node *> nodes;
	 void print_node();

 };


 /* trees and their routines */

 class Tree{

 private:
	 Node * head;

	 /* internal simplification routines */
	 /* fill as they are written */

 public:
	 
	 virtual void print_tree(std::ostream &file);
	 Node * get_head();

	 //all the tree transformations goes here
	 void canonicalize_tree();

	 /* for tree simplification -> each type of tree will have its own serialization routines */
	 virtual string serialize_tree() = 0;
	 virtual void construct_tree(string stree) = 0;

	 /* tree simplification routine */
	 void simplify_tree();

 };


 //this class has builds up the expression
 class Conc_Tree : public Tree {

 private:

	 // data structure for keeping the hash table bins
	 struct frontier_t {   
		 Node ** bucket;
		 int amount;
	 };
	 

	 frontier_t * frontier;  //this is actually a hash table keeping pointers to the Nodes already allocated
	 vector<uint> mem_in_frontier; // memoization structures for partial mem and reg writes and reads 

	 int generate_hash(operand_t * opnd);
	 Node * search_node(operand_t * opnd);
	 void add_to_frontier(int hash, Node * node);
	 Node * create_or_get_node(operand_t * opnd);
	 void remove_from_frontier(operand_t * opnd);


	 void get_full_overlap_nodes(std::vector<Conc_Node *> &nodes, operand_t * opnd);
	 void split_partial_overlaps(std::vector < std::pair <Conc_Node *, std::vector<Conc_Node *> > > &nodes, operand_t * opnd, uint32_t hash);
	 void get_partial_overlap_nodes(std::vector< std::pair<Conc_Node *, std::vector<Conc_Node *> > > &nodes, operand_t * opnd);


 public:
	
	 /* this governs which conditionals affect the tree -> needed for predicate tree generation */
	struct conditional_t {

		 jump_info_t * jumps;
		 uint32_t line_cond; // this is the cond_pc location 
		 uint32_t line_jump;
		 Conc_Tree * tree; // we need to have two expressions for a single conditional
		 bool taken;

	 };

	 vector<conditional_t *> conditionals;

	 Conc_Tree();
	 ~Conc_Tree();

	 bool update_depandancy_backward(rinstr_t * instr, uint32_t pc, std::string disasm, uint32_t line);
	 bool update_dependancy_forward(rinstr_t * instr, uint32_t pc, std::string disasm, uint32_t line);

	 //virtuals
	 void print_tree(std::ostream &file);
	 string serialize_tree();
	 void construct_tree(string stree);

 };



 /* Abs tree */
 class Abs_Tree : public Tree {

 public:

	 Abs_Tree();
	 ~Abs_Tree();

	 void build_abs_tree(Conc_Tree * tree, std::vector<mem_regions_t *> &mem_regions);
	 void seperate_intermediate_buffers();
	 static bool are_abs_trees_similar(std::vector<Abs_Tree *> abs_trees);

	 //virtuals
	 void print_tree(std::ostream &file);
	 string serialize_tree();
	 void construct_tree(string stree);

 };

 /* compound abs tree */
 class Comp_Abs_tree{

 public:
	 Comp_Abs_node * head;

	 Comp_Abs_tree();

	 /* all are static methods actually */
	 void build_compound_tree(Comp_Abs_node * comp_node, vector<Abs_node *> abs_nodes);
	 static void abstract_buffer_indexes(Comp_Abs_node * comp_node);
	 Abs_node * compound_to_abs_tree();

 private:

	 static void abstract_buffer_indexes(Comp_Abs_node * head, Comp_Abs_node * node);
	 string serialize_tree();
	 void construct_tree(string stree);

 };





#endif