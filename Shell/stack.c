/****************************************************************************************************
 *	Title: Simple Integer Stack
 *	Author: Sean Hinds
 *	Date: 03/03/18
 *	Description: Implementation of a simple linked-list stack data structure to store integers. Useful for
 *			storing process IDs of subprocesses within a main process.
 * *************************************************************************************************/


#include "stack.h"


/* allocate memory for stack and initialize data members */

int initStack(struct Stack** stackAddr) {

	//printf("initStack\n"); fflush(stdout);

	(*stackAddr) = (struct Stack* ) malloc(sizeof(struct Stack));
	(*stackAddr)->size = 0;
	(*stackAddr)->top = NULL;

}


/* push an integer to the stack, allocating a new stacknode */

void pushStack(struct Stack* stack, int val) {
	
	//printf("pushStack\n"); fflush(stdout);

	struct StackNode* node = malloc(sizeof(struct StackNode));
	node->val = val;
	node->next = stack->top;
	stack->top = node;

}


/* pop and return the top of the stack */

int popStack(struct Stack* stack) {

	int res = stack->top->val;
	
	if (stack->top) {
		struct StackNode* garbage;
		garbage = stack->top;
		stack->top = stack->top->next;
		free(garbage);
	}
	
	return res;

}


/* delete an item from the stack by value, NOT necessarily the top node */

void deleteFromStack(struct Stack* stack, int val) {

	struct StackNode* ptr = stack->top;
	struct StackNode* prev = stack->top;
	while (ptr != NULL) {
		if (ptr->val = val) {
			if (ptr = stack->top) {
				stack->top = ptr->next;
			}
			else {
				prev->next = ptr->next;
			}
			free(ptr);
			return;
		}
		else {
			prev = ptr;			
			ptr = ptr->next;
		}
	}

}


/* print the stack */

void printStack(struct Stack* stack) {

	//printf("printstack\n"); fflush(stdout);

	struct StackNode* ptr = stack->top;
	while (ptr != NULL) {
		printf("child %d\n", ptr->val);
		fflush(stdout);
		ptr = ptr->next;
	}

}


/* deallocate the stack */

void dumpStack(struct Stack* stack) {

	struct StackNode* garbage;
	while (stack->top != NULL) {
		garbage = stack->top;
		stack->top = stack->top->next;
		free(garbage);
	}
	if (stack) {
		free(stack);
	}

}
