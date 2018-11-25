/***************************************************************************************
 *	Title: Command Loop Header File
 *	Author: Sean Hinds
 *	Date: 03/02/18
 *	Description: Function signatures and struct data definitions for the command
 *			loop implementation as part of the smallsh shell program.
 * ************************************************************************************/


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stack.h"

#ifndef COMMAND_LOOP_H
#define COMMAND_LOOP_H

struct redirect {

        int status;
        char* path;

};


void commandLoop();
char** getArgs(char* );
int execArgs(int, char** );
int countArgs(char** );

int shExit(char** );
int shCd(char** );
int shStatus(char** );

struct redirect* checkIORedirection(char** );
void checkOnChildren();
void termForeground();
void toggleBG();

#endif
