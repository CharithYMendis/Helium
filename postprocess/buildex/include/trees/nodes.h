 #ifndef _NODES_H
 #define _NODES_H
 
#include  "..\..\..\dr_clients\include\output.h"
#include <vector>
#include <stdint.h>

#include "memory/memregions.h"

#define NODE_RIGHT	1
#define NODE_LEFT	2
#define NODE_NONE   3

#define MAX_REFERENCES 10

 
 class Node{

	public:

		
		int operation;   /* the operation of this node */
		bool sign; /* indicates whether the operation is signed */
		operand_t * symbol;  /* operand information for this node - this is concrete */

		std::vector<Node *> srcs;  /* forward references also srcs of the destination */
		std::vector<Node *> prev; /* keep the backward references */
		std::vector<uint> pos;	 /* position of the parent node's srcs list for this child */

		
		uint32_t pc;
		uint32_t line; /*variable for debugging*/

		/*auxiliary variables*/
		int order_num;
		int para_num; 
		bool is_para;
		bool is_double;

		bool visited; /* should clear this after completing the traversal */
		
	public:
		
		/* constructors */
		Node();
		Node(const Node& node);
		virtual ~Node() = 0;

		/* virtual functions */
		virtual std::string get_node_string() = 0;
		virtual std::string get_dot_string() = 0;
		virtual std::string get_simpl_string() = 0;

		/* following routines will have concrete implementations; but can be overloaded */
		/* tree transformation routines */

		uint32_t remove_forward_ref(Node *ref); /* (node -> ref) => (node) */
		uint32_t remove_backward_ref(Node *ref); /* (ref -> node) => (node) */
		void add_forward_ref(Node * ref); /* node => (node->ref) */
		void remove_intermediate_ref(Node * dst, Node *src); /* (dst -> node -> src)  => (dst -> src) */
		void change_ref(Node * dst, Node * src); /* (dst -> ref) => (dst -> src) */
		
		void safely_delete(Node * head); /*safely delete*/
		void remove_forward_all_srcs(); /* removes all srcs */

		/* checking properties of nodes */
		virtual bool are_nodes_similar(Node * node) = 0;
		static bool are_nodes_similar(std::vector<Node *> nodes);
		int32_t is_node_indirect();
	

		/* node canonicalizing routines */
	public:
		bool remove_backward_ref_single(Node *ref);
		bool remove_forward_ref_single(Node * ref);
		bool congregate_node(Node * head); /* congregate nodes with associative operations */
		void order_node();  /* order the srcs of the node */
		


};

/* Conc_Node */


 class Conc_Node : public Node {

 public:

	 mem_regions_t * region;

	 Conc_Node(operand_t * symbol);
	 Conc_Node(uint32_t type, uint64_t value, uint32_t width, float float_value);
	 Conc_Node(operand_t * symbol, std::vector<mem_regions_t *> &region);
	 Conc_Node();
	 ~Conc_Node();

	 std::string get_node_string();
	 std::string get_dot_string();
	 std::string get_simpl_string();

	 bool are_nodes_similar(Node * node);


 };

 

 /* Abs_Node */

 /* type and processing explanation

 abs_nodes types and processing are determined by the Node symbol types and the mem_regions types, therefore, we can see as building the abs tree as a unification of the information extracted by the instrace(building the concrete tree) and memdump(locating image locations and properties). Further
 abstraction can be performed on this tree.

 type(Abs_node) <- unify ( type(Node->symbol) , type(mem_region) )

 Next, we assume that the bottom of the tree should be Heap Nodes (image dependancies), Stack or Regs(parameters) or immediate ints, immediate floats

 []if (Node->symbol) is heap
 *check for associated memory region -> this should be non-null
 *get the x,y,c co-ordinates of the symbol value and store in the struct mem_info
 *mem_info struct
 -> associated mem  - mem_region
 -> dimensions - the dimensionality of the mem_region -> this depends on the image layout
 -> indexes - this carries the final abstracted out index coefficients when we express the filter algebrically
 -> pos - this lifts the flat memory address into coordinates in the dimensional space
 *type will depend on whether the associated mem region is input, output or intermediate

 []if (Node->symbol is reg or stack)
 *parameter? Need to identify statically
 *operation_only? If not parameter

 []if(Node->symbol immed int or float)

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
		uint32_t head_dimensions;
		int ** indexes; /* 2-dimensional array */
		int * pos;
	} mem_info;
	 

	 //add the copy constructor when needed
	 Abs_Node();
	 Abs_Node(Conc_Node * head, Conc_Node * conc_node, std::vector<mem_regions_t *> &mem_regions);
	 ~Abs_Node();

	 std::string get_symbolic_string(std::vector<std::string> vars);
	 std::string get_node_string();
	 std::string get_dot_string();
	 std::string get_dot_string(std::vector<std::string> vars);
	 std::string get_simpl_string();

	 bool are_nodes_similar(Node * node);


 private:
	 std::string get_mem_string(std::vector<std::string> vars);
	 std::string get_mem_string();
	 std::string get_immediate_string(std::vector<std::string> vars);
 };

 
 /* Comp_Abs_Node */
 
 class Comp_Abs_Node : public Node {

 public:

	 Comp_Abs_Node();
	 Comp_Abs_Node(Node * node);
	 Comp_Abs_Node(std::vector<Abs_Node *> nodes);
	 std::vector<Abs_Node *> nodes;

	 std::string get_node_string();
	 std::string get_dot_string();
	 std::string get_simpl_string();

	 bool are_nodes_similar(Node * node);


 };



#endif