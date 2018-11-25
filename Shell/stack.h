/***********************************************************************************************
 *	Title: Stack Data Declarations
 *	Author: Sean Hinds
 *	Date: 03/02/18
 * 	Description: Function signatures and struct definitions to be used in an implementation
 * 			of a linked-list stack data structure to store integers
 * ********************************************************************************************/


#include <stdlib.h>
#include <stdio.h>	

#ifndef STACK_H
#define STACK_H

struct StackNode {

	int val;
	struct StackNode* next;

};

struct Stack {
	
	int size;
	struct StackNode* top;

};

int initStack(struct Stack** );
void pushStack(struct Stack* , int);
int popStack(struct Stack* );
void deleteFromStack(struct Stack* , int);
void printStack(struct Stack* );
void dumpStack(struct Stack* );

#endif
