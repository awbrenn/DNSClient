#include "DNSClient.h"
using namespace std;

/* Function Declarations */
void checkFlag(int index, char *argv[]);
void stateProperUsageAndDie();

/* Global Variables */
unsigned int TIMEOUT_SECONDS = 5;
unsigned int RETRIES = 3;
unsigned short PORT = 53;

int main (int argc, char *argv[]) {
    // int sock;                        /* Socket descriptor */
    // struct sockaddr_in serverAddr;   /* server address */
    // struct sockaddr_in fromAddr;     /* Source address */
    // struct hostent *thehost;         /* Hostent from gethostbyname() */
    // unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *DNSServIP;                    /* IP address of server */
    struct sigaction timeoutAction;  /* signal action for timeouts */


    if (argc == 3) // zero optional flags set
        ;// do nothing
    else if (argc == 5) // one optional flags set
        checkFlag(1, argv);
    else if (argc == 7) { // two optional flags set
        checkFlag(1, argv);
        checkFlag(3, argv);
    }
    else if (argc == 9) { // three optional flags set
        checkFlag(1, argv);
        checkFlag(3, argv);
        checkFlag(5, argv);
    }
    else
       stateProperUsageAndDie();

    // printf("\n%d, %d, %d\n", TIMEOUT_SECONDS, RETRIES, PORT);

    if (sigfillset(&timeoutAction.sa_mask) < 0) /* blocks everything in handler */
        dieWithError((char *)"sigfillset() failed");
    timeoutAction.sa_flags = 0;
    if (sigaction(SIGALRM, &timeoutAction, 0) < 0)
        dieWithError((char *)"sigaction() failed for SIGALRM");

    DNSServIP = argv[argc-2]; /* First arg: server IP address (dotted quad) */
    memmove(DNSServIP, DNSServIP + 1, strlen(DNSServIP));
    // cout << DNSServIP << "yada" << endl;

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

	return 0;
}


void checkFlag(int index, char *argv[]) {
            if (strcmp("-t", argv[index]) == 0)
                TIMEOUT_SECONDS = (unsigned int) atoi(argv[index+1]);
            else if (strcmp("-r", argv[index]) == 0)
                RETRIES = (unsigned int) atoi(argv[index+1]);
            else if (strcmp("-p", argv[index]) == 0)
                PORT = (unsigned short) atoi(argv[index+1]);
            else
                stateProperUsageAndDie();
}

