#ifndef _OUTPUT_EXALGO_H
#define _OUTPUT_EXALGO_H

/* all data structures which are outputted to files should reside here for use of other 
    clients or post processing passes */

#define MAX_SRCS 4
#define MAX_DSTS 4

#define REG_TYPE		0
#define MEM_STACK_TYPE	1
#define MEM_HEAP_TYPE	2
#define IMM_FLOAT_TYPE	3
#define IMM_INT_TYPE	4
#define DEFAULT_TYPE	5

typedef unsigned int uint;
typedef unsigned long long uint64;
	
typedef struct _operand_t {

	uint type;
	uint width;
	union {
		uint64 value;
		float float_value;
	};
	struct _operand_t * addr;
} operand_t;


typedef struct _output_t {

	uint opcode;
	uint num_srcs;
	uint num_dsts;
	operand_t srcs[MAX_SRCS];
	operand_t dsts[MAX_DSTS];
	uint eflags;
	uint pc;

} output_t;



#endif


