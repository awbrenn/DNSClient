/*********************************************************
File Name:  ErrorFunctions.cpp
Author:     Austin Brennan
Course:     CPSC 3600
Instructor: Sekou Remy
Due Date:   03/06/2015

File Description:
Ths file contains a routine called DieWithError.
The routine prints an error message and exits the program.

List of routines:
	DieWithError
	stateProperUsageAndDie

*********************************************************/

#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */
#include <string.h>

using namespace std;

/*  DieWithError
    input        - takes an error message
    output       - none
    description: - this routine prints an error message
                    and exits the program.
*/
extern void dieWithError(char *error_message) {
    fprintf(stderr, "ERROR\t%s\n", error_message);
    exit(1);
}


/*  stateProperUsageAndDie
    input        - none
    output       - none
    description: - states the proper useage and calls dieWithError
*/
void stateProperUsageAndDie() {
    dieWithError((char *)"USAGE ERROR\n\nProper Use:\ndnsq [-t <time>] "
                         "[-r <retries>] [-p <port>] @<svr> <name>");
}
