/*********************************************************
File Name:  valueGuessGame.h
Author:     Austin Brennan
Course:     CPSC 3600
Instructor: Sekou Remy
Due Date:   January 30th, 2015


File Description:
This file contains most of my includes and some defined
constants.

*********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     /* for memset() */
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>      /* for getHostByName() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <signal.h>


#define MAX_RAND_NUMBER 1000000000 /* maximum value that the random number can be */
#define MAX_BUFF 11     /* Longest string to server */

void dieWithError(char *errorMessage);  /* External error handling function */
void stateProperUsageAndDie();
