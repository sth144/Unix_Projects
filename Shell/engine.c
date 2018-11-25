/**************************************************************************************************************************
 *	Title: smallsh engine
 *	Author:Sean Hinds
 *	Date: 02/19/18
 *	Description: main driver for the smallsh program. 
 *
 *			read Stephan Brennan's tutorial at <https://brennan.io/2015/01/16/write-a-shell-in-c/> 
 *			prior to writing this application.
 * ***********************************************************************************************************************/

#include "commandLoop.h"

int main(int argc, char** argv) {

	/* command function takes program through user prompt, user input, and command execution */

	commandLoop();

	return EXIT_SUCCESS;

}
