/*********************************************************
File Name:  valueGuessGame.h
Author:     Austin Brennan
Course:     CPSC 3600
Instructor: Sekou Remy
Due Date:   January 30th, 2015

File Description:
Ths file contains a single routine called DieWithError.
The routine prints an error message and exits the program.

List of routines:
	DieWithError

*********************************************************/

#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */

/*	DieWithError
	input			- takes an error message
	output			- none
	description:	This routine prints an error message
					and exits the program.
*/
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
