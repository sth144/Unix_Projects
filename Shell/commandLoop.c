/*******************************************************************************************************
 *	Title: Command Loop Implementation for Smallsh Shell
 *	Author: Sean 
 *	Date: 03/03/18
 *	Description: Implements the functionality of the command prompt loop in smallsh program. Written
 *			in C, this program accepts user input as a set of arguments. Supports built in
 *			commands cd, status, and exit, otherwise uses process forking and the execvp() 
 *			function to execute commands using the UNIX kernal
 * ****************************************************************************************************/


#include "commandLoop.h"


/* function names and pointers for builtin commands */
int numBuiltins = 3;
char* builtinNames[] = {"exit", "cd", "status"};
int (*builtinFuncs[])(char** ) = {&shExit, &shCd, &shStatus};

/* global variables to track exit status of last command and currently running subprocesses */
int lastCommandStatus, lastCommandSignal;
struct Stack* processStack;

/* set foreground only mode and track whether last command was background */
int allowBG = 1;
int lastCommandIsBG;

/* declare sigaction structs */
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};
struct sigaction SIGCHLD_action = {0};
 

/* commandLoop() function will be called in the engine program to prompt user continuously using a while loop */

void commandLoop() {

	/* set signal handlers for parent process */

        SIGINT_action.sa_handler = SIG_IGN;	// ignore SIGINT by default in partent
        sigfillset(&SIGINT_action.sa_mask);            
        SIGINT_action.sa_flags = SA_RESTART;

        sigaction(SIGINT, &SIGINT_action, NULL);

        SIGTSTP_action.sa_handler = toggleBG;	// SIGTSTP reprogrammed to toggle foreground only mode
        sigfillset(&SIGTSTP_action.sa_mask);
        SIGTSTP_action.sa_flags = SA_RESTART;

        sigaction(SIGTSTP, &SIGTSTP_action, NULL);

        SIGCHLD_action.sa_handler = checkOnChildren;
        sigfillset(&SIGCHLD_action.sa_mask);
        SIGCHLD_action.sa_flags = SA_RESTART;

        sigaction(SIGCHLD, &SIGCHLD_action, NULL);

	/* variables for grabbing user input */

	int argc, stat, i;	
	char** args;
	char* input;

	/* allocate memory for the process stack */
	int init = initStack(&processStack);

	/* continuously prompt user */

	do {

		printf(": "); fflush(stdout);
		
		/* grab user input, count arguments, and execute those arguments */

		args = getArgs(input);	
		argc = countArgs(args);
		stat = execArgs(argc, args);
		
		/* free allocated args array */
			
		if (args != NULL) {free(args);}
		if (input != NULL) {/*printf("input not null\n"); free(input);*/} // troublemaker?

	} while (stat);	

	/* dump the stack, free memory allocated to the stack structure */

	dumpStack(processStack);

}


/* getArgs takes user input as a pointer to char, parses arguments,  and returns an array of pointers to char */

char** getArgs(char* input) {

	/* get a line of input from user */

	size_t bfrsize = 0;
	size_t chars;

	chars = getline(&input, &bfrsize, stdin);

	/* expand $$ into PID anywhere it is encountered */

	char* toSearch = malloc(sizeof(input));		// remaining buffer to search
	char* ptr = input;
	while (ptr = strstr(ptr, "$$")) {
		memset(toSearch, 0, sizeof(toSearch));
		strcpy(toSearch, ptr + (2 * sizeof(char)));
		/* sprintf the PID and toSearch to input wherever ptr is pointing */
		sprintf(ptr, "%d%s", getpid(), toSearch);
		ptr += (2 * sizeof(char));
	}
	free(toSearch);

	char** args = malloc(bfrsize * sizeof(char* ));
	const char* delims = " \n\t\a\r";		// strtok will parse by these delimeters
	char* arg;
	
	if (input[0] != '#') {
	
		/* tokenize input string into args */
	
		arg = strtok(input, delims);
	
		int i = 0; 
		while (arg != NULL) {
			
			args[i] = arg;
			i++;

			if (i == bfrsize) {
				args = realloc(args, bfrsize * sizeof(char* ));
			}

			/* next token */
			arg = strtok(NULL, delims);

		}
		args[i] = NULL;
	}
	else {
		args[0] = NULL;
	}

	return args;

}


/* countArgs() counts the number of arguments user entered */

int countArgs(char** args) {

	int i = 0;
	while (args[i] != NULL) {
		i++;
	}
	return i;

}


/* execArgs executes user entered command either by calling a built in shell function or a Unix system call through execvp() */

int execArgs(int argc, char** args) {

	int i;

	if (args[0] == NULL) { return 1; }		// User entered nothing

	lastCommandIsBG = 0;				// Reset lastCommandIsBG

	/* set background/foreground status of subprocess based on program state and user command  */

	if (strcmp(args[argc - 1], "&") == 0) {
		//printf("background process\n"); fflush(stdout);
		if (allowBG) {lastCommandIsBG++;}
		args[argc - 1] = NULL;
		argc--;
	}

	/* check if user entered a built-in command */

	for (i = 0; i < numBuiltins; i++) {

		/* check if arg[0] matches the name of a built in command */

		if (strcmp(args[0], builtinNames[i]) == 0) {
				
			/* A built in command will be executed. return status of executed built in command */
			return builtinFuncs[i](args);

		}

	}

	/* if none of the above returns caught, fork the new child process from parent */

	int stat = -5, dontFork = 0;
	pid_t newPid, wPid;

	/* check for IO redirection */

	int stdIn = dup(0);
	int stdOut = dup(1);
	int devNull = open("/dev/null", 0);
	
	int newIn;
	int newOut;
	int redirectInput;
	int redirectOutput;

	char* redirectInputPath;
	char* redirectOutputPath;

	struct redirect* ioIsRedirected;
	ioIsRedirected = checkIORedirection(args);

	redirectInput = ioIsRedirected[0].status;
	redirectOutput = ioIsRedirected[1].status;

	/* setup IO redirection */

	if (redirectInput) {
		newIn = open(ioIsRedirected[0].path, O_RDONLY | O_CREAT, 0644);
		if (newIn < 0) { printf("cannot open %s for input\n", ioIsRedirected[0].path); 
			fflush(stdout); dontFork++; lastCommandStatus = 1; lastCommandSignal = -5; return 1; }
	}
	if (redirectOutput) {
		newOut = open(ioIsRedirected[1].path, O_APPEND | O_TRUNC | O_WRONLY | O_CREAT, 0644);
		if (newOut < 0) { printf("cannot open %s for output\n: ", ioIsRedirected[1].path);
			fflush(stdout); dontFork++; lastCommandStatus = 1; lastCommandSignal = -5; return 1; }
	}
	
	/* verify that program should fork a new process. Fork */

	newPid = 0;
	if (!dontFork) {
		newPid = fork();	
	
		/* IO redirection happens here */
	
		if (redirectInput) { int redirectInputCommand = dup2(newIn, 0); }

		if (redirectOutput) { int redirectOutputCommand = dup2(newOut, 1); }

		for (int i = 0; i < 2; i++) { free(ioIsRedirected[i].path); }
		free(ioIsRedirected);

		/* call execvp() in child process using a switch */

		switch(newPid) {
	
			case 0:
			
				/* fork successful, execute this  within child process */

			        /* if child is foreground, overwrite SIGINT_action. Child will respond to SIGINT */

				if (!lastCommandIsBG) {
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigaction(SIGTSTP, &SIGTSTP_action, NULL);

			       		SIGINT_action.sa_handler = toggleBG;
        				sigaction(SIGINT, &SIGINT_action, NULL);
				} else {
					/* background command, if not specified redirect IO to/from /dev/null */
					printf("background pid is %d\n: ", getpid()); fflush(stdout); 
					if (!redirectInput) { dup2(devNull, 0); }
					if (!redirectOutput) { dup2(devNull, 1); } 
				}
        		
				if (execvp(args[0], args) == -1) {
					/* execvp should not return from child process */
					perror(args[0]);
					exit(1);
				}
				break;
	
			case -1:

				/* fork unsuccessful, throw error (parent process) */
			
				perror("fork unsuccessful");
				break;		

			default:
	
				/* execute this clause within parent process */

				//printf("I am the parent %d\n", getpid()); fflush(stdout);

				/* push child pid to the process stack */
	
				pushStack(processStack, newPid);
				//printf("stack right now\n"); fflush(stdout); printStack(processStack);
		
				if (!lastCommandIsBG) {
					do {
						wPid = waitpid(newPid, &stat, 0);
						//printf("wifsignaled is %d\n", WIFSIGNALED(stat));
						//printf("wifexited is %d\n", WIFEXITED(stat));
					} while (!WIFEXITED(stat) && !WIFSIGNALED(stat)); // macros

					if (WIFSIGNALED(stat) && (WTERMSIG(stat) == 2)) 
						{ lastCommandSignal = 2; lastCommandStatus = -5; shStatus(NULL); }
		
					else if (WIFEXITED(stat)) {lastCommandStatus = WEXITSTATUS(stat); lastCommandSignal = -5;}
					else if (WIFSIGNALED(stat)) {lastCommandSignal = WTERMSIG(stat); lastCommandStatus = -5;}
					/* child has exited, delete from process stack */

					deleteFromStack(processStack, newPid);			
					//printStack(processStack);

				} 					
				break;

		}
	}

	/* check for io redirection and restore IO */
	
	if (redirectInput) { dup2(stdIn, 0); close(newIn); }
	if (redirectOutput) { dup2(stdOut, 1); close(newOut); }
	close(devNull);

	return 1;

}


/* built in command handler functions */


/* shExit() exits the shell process after killing all subprocesses */

int shExit(char** args) {
	
	//printf("shexit\n");

	/* check for background processes */

	//printf("checking on the children\n");

	int killPid;
	struct StackNode* ptr = processStack->top;
	while (ptr != NULL) {
		
		killPid = ptr->val;
		ptr = ptr->next;
		//printf("I have no code of ethics. I just love killin'!\n"); fflush(stdout);
		kill(killPid, SIGKILL);		

	}

	exit(0);

}


/* shCd changes working directory. If no additional argument supplied, changes working directory to environment HOME directory */

int shCd(char** args) {
	
	//printf("shcd\n");
	
	if (args[1] == NULL) {
		chdir(getenv("HOME"));
	}
	else {
		chdir(args[1]);
	}	

	return 1;

}


/* shStatus prints EITHER the exit status of or signal which terminated last child process of shell */

int shStatus(char** args) {	

	//printf("shstatus\n");
	//if (strcmp(args[0], "sigint") == 0) { printf("terminated by signal %d\n", lastCommandSignal); fflush(stdout); }	

	if (lastCommandStatus != -5) { printf("exit value %d\n", lastCommandStatus); fflush(stdout); }
	else if (lastCommandSignal != -5) { printf("terminated by signal %d\n", lastCommandSignal); fflush(stdout); }
	return 1;

}


/* check args to see if IO was redirected */

struct redirect* checkIORedirection(char** args) {

	// printf("checking IO redirection\n");
	/* use redirect struct to track two boolean variables which describe whether input and output are redirected,
  		as well as the path of redirection */

	struct redirect* results = malloc(sizeof(struct redirect) * 2);

	for (int i = 0; i < 2; i++) {
		results[i].status = 0;
		results[i].path = malloc(sizeof(char ) * 255);
	}

	int i = 0, j = 0, k = 0;

	/* while there are args to parse */

	while (args[i] != NULL) {

		/* look for redirection operators */

		if (args[i] != NULL && strcmp(args[i], "<") == 0) {
			
			results[0].status++;			// set input as redirected
			strcpy(results[0].path, args[i + 1]);
			j = i;
			k = i + 2;

			/* overwrite redirection operator in args array */

			while (args[k] != NULL) {
				strcpy(args[j], args[k]);
				j++;
				k++;
			}
			
			args[j] = NULL; args[j + 1] = NULL;
			
		}

		if (args[i] != NULL && strcmp(args[i], ">") == 0) {
			
			results[1].status++;			// set output as redirected
			strcpy(results[1].path, args[i + 1]);
			j = i;
			k = i + 2;
			while (args[k] != NULL) {
				strcpy(args[j], args[k]);
				j++;
				k++;
			}
			args[j] = NULL; args[j + 1] = NULL;

		}

		i++;	

	}

	return results;

}


/* checkOnChildren() will be called by the shell process through a sigaction which handles SIGCHLD */

void checkOnChildren() {

	/* check for background processes */

	//printf("checking on the children\n");

	int wPid, stat = -5;
	struct StackNode* ptr = processStack->top;
	while (ptr != NULL) {
		//printf("checking a background (?) process\n"); fflush(stdout);
		wPid = waitpid(ptr->val, &stat, WNOHANG);
		ptr = ptr->next;
		if (wPid > 0 && WIFEXITED(stat)) {
			/* informative message */
			printf("background pid %d is done: exit value %d\n: ", wPid, WEXITSTATUS(stat)); fflush(stdout);
			/* child has exited, delete from process stack */	
			deleteFromStack(processStack, wPid);						
		}
	}
	

}


/* toggle normal and foreground-only modes, will be called by a sigaction which handles SIGTSTP */

void toggleBG() {

	if (allowBG) {
		printf("Entering foreground-only mode (& is now ignored)\n: "); fflush(stdout);
		allowBG = 0;
	} else {
		printf("Exiting foreground-only mode\n: "); fflush(stdout);
		allowBG = 1; 
	}

}
