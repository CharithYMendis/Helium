#ifndef _STACK_EXALGO_H
#define _STACK_EXALGO_H

#include "dr_api.h"
#include "drvector.h"

typedef struct _stack_t {

	drvector_t * vector;
	int head;

} stack_t;


bool stack_init(stack_t ** stack, uint capacity, void(*free_data_func) (void *));
void stack_delete(stack_t * stack);
void * stack_pop(stack_t * stack);
void * stack_peek(stack_t * stack);
bool stack_push(stack_t * stack, void * data);


/* testing function */
void test_stack();


#endif