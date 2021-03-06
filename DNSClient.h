/*********************************************************
File Name:  DNSClient.h
Author:     Austin Brennan
Course:     CPSC 3600
Instructor: Sekou Remy
Due Date:   03/06/2015


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
#include <stdint.h>


#define MAX_RAND_NUMBER 65535 /* maximum value that the random number can be
								 for a 16 bit unsigned integer */
#define MAX_BUFF 1000     /* Longest string to server */

void dieWithError(char *errorMessage);  /* External error handling function */
void stateProperUsageAndDie();
