**********************************************************************************************************
	Title: SmallSH
	Author: Sean Hinds
	Date: 03/03/18
	Description: This shell program, written in C, supports 3 built in commands: cd, status, and exit.
			All other commands are executed through Unix system calls. This shell supports
			background commands, and input and output redirection. 
**********************************************************************************************************

To compile:

	$: gcc engine.c commandLoop.c stack.c -o smallsh

	OR

	$: make

	(makefile included)


To run:

	$: ./smallsh


Disable background commands:

	(smallsh) $: ^Z


Kill shell foreground child process:

	(smallsh) $: ^C


Comment:

	(smallsh) $: # ...comment...
