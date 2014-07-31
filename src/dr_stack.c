#include "dr_stack.h"


bool stack_init(stack_t ** stack, uint capacity, void (*free_data_func) (void *)){

	*stack = (stack_t *)dr_global_alloc(sizeof(stack_t));

	(*stack)->head = -1;
	(*stack)->vector = (drvector_t *)dr_global_alloc(sizeof(drvector_t));

	return drvector_init((*stack)->vector, capacity, false, free_data_func);

}

void stack_delete(stack_t * stack){
	stack->vector->entries = stack->head + 1; /* curtail the number of entries of the actual stack entries; the user is responsible for freeing out of range entries */
	drvector_delete(stack->vector);
	dr_global_free(stack->vector, sizeof(drvector_t));
	dr_global_free(stack, sizeof(stack_t));
}

void * stack_pop(stack_t * stack){

	if (stack->head == -1) return NULL; 

	void * ret = drvector_get_entry(stack->vector, stack->head);

	

	if (ret != NULL){
		stack->head--;
	}

	

	return ret;

}

void * stack_peek(stack_t * stack){
	if (stack->head == -1) return NULL;

	return drvector_get_entry(stack->vector, stack->head);

}



bool stack_push(stack_t * stack, void * data){

	if (drvector_set_entry(stack->vector, stack->head + 1, data)){
		stack->head++;

		

		return true;
	}

	return false;

}



/***********stack test routines*************/

typedef struct _test_t {
	uint value;
}test_t;

void test_clean(void * data){
	dr_global_free(data, sizeof(test_t));
}

void test_stack(){

	
	int i = 0;
	stack_t * stack;
	test_t * item;

	dr_printf("starting to test stack implementation\n");

	if (stack_init(&stack, 20, test_clean)){
		dr_printf("stack init successful\n");
	}
	else{
		dr_printf("stack init failed\n");
		dr_abort();
	}

	for (i = 0; i < 10; i++){
		item = dr_global_alloc(sizeof(test_t));
		item->value = i;
		stack_push(stack, item);
	}

	/* pop 5 items and check for values */

	for (int i = 0; i < 5; i++){
		item = (test_t *)stack_pop(stack);
		dr_printf("expected - %d; got - %d\n", 9 - i, item->value);
	}

	/* pop other 5 items as well */

	for (int i = 0; i < 5; i++){
		stack_pop(stack);
	}

	item = stack_pop(stack);
	if (item == NULL) { dr_printf("got null as expected\n"); }


	stack_delete(stack);

}