 #ifndef INSTRACE
 #define INSTRACE
 
#include "defines.h"

 /*instrumentation routines*/
void instrace_init(client_id_t id, 
				const char * arguments);
void instrace_exit_event(void);
dr_emit_flags_t instrace_bb_instrumentation(void *drcontext, void *tag, instrlist_t *bb,
                instr_t *instr, bool for_trace, bool translating,
                void *user_data);
dr_emit_flags_t
instrace_bb_analysis(void *drcontext, void *tag, instrlist_t *bb,
                  bool for_trace, bool translating,
                  OUT void **user_data);
dr_emit_flags_t
instrace_bb_app2app(void *drcontext, void *tag, instrlist_t *bb,
                 bool for_trace, bool translating);
void instrace_thread_init(void *drcontext);
void instrace_thread_exit(void *drcontext);

#define DEBUG_INSTRACE
 
 /* typdefs and defines */

/*
	destination and source encoding
	
	seperate field to signify reg or mem

	reg (type & 0b11 = 0)
	---
	eax,...,edi - set according to the reg number (reg_id I think)
	register width - is encoded in the type information
	8 bit high - 8th bit high
	8 bit low - 7th bit high
	16 bit - 6th bit high
	32 bit - 5th bit high
	64 bit - 4th bit high

	mem  (type & 0b11 = 1)
	---
	absolute mem address (virtual) - assuming we are only tracking one process's dependancies - aliasing??
	memory width - will be stored in the location position -> actual location will not be updated statically.
	mem_addr will not be stored statically, but will be carried dynamically in instr_trace_t data structure

	immediate  (type & 0b11 = 2)
	---------
	the location field will have the value of it

*/

#define WIDTH_8L				(1<<7)
#define WIDTH_8H				(1<<6)
#define WIDTH_16				(1<<5)
#define WIDTH_32				(1<<4)
#define WIDTH_64				(1<<3)

#define GET_TYPE(x)				( (x) & 0x3 )
#define SET_TYPE(x,type)		x = type  
#define REG_TYPE				(uchar)0
#define MEM_TYPE				(uchar)1
#define IMM_TYPE				(uchar)2


#define OPCODE_COUNT	1098

#define DEBUG_STATIC


/*
	struct for keeping operand information
*/

typedef struct _operand_t {

	unsigned char type;
	unsigned char width;
	unsigned int location;

} operand_t;


/*
	destination struct - contains the operands as well as the operation performed on the value stored at this destination.
*/

#define MAX_SRCS 2

/*
 info that can be statically determined - will assume a canonical form 2/1 srcs and 1 dest
*/

typedef struct _static_info_t {
    
	unsigned char operation;
	unsigned char src_num;
	unsigned char dest_num;
	operand_t src[MAX_SRCS];
	operand_t dst;
	app_pc pc;

	//these are for debug only
#ifdef DEBUG_INSTRACE
	char disassembly[SHORT_STRING_LENGTH];
	uchar phase;
#endif

} static_info_t;

 
 #endif

