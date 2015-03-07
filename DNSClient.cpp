/*********************************************************
File Name:  DNSClient.cpp
Author:     Austin Brennan
Course:     CPSC 3600
Instructor: Sekou Remy
Due Date:   03/06/2015

File Description:
    See Readme.

List of Functions:
    See function declarations

*********************************************************/

#include "DNSClient.h"
using namespace std;

/* Function Declarations */
void checkFlag(int index, char *argv[]);
void stateProperUsageAndDie();
uint16_t getQueryID();
void buildQueryHeader(vector<uint16_t> *, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
void buildQueryBody(vector<uint16_t> *, char *);
void getLabelSizes(vector<uint8_t> *label_sizes, char *host_name);
void addBodyToDNSMessage(vector<uint8_t> *DNS_body, vector<uint16_t> *DNS_message);
void catchAlarm(int);
void processResponse(uint8_t *DNS_response, int respLen);
void addAliasToResponseString(uint8_t [], string *, unsigned int);
unsigned int skipName(uint8_t DNS_response[], unsigned int response_offset);
int processAnswer(uint8_t DNS_response[], unsigned int response_offset, uint16_t type, char *auth_string);
void addIPAddressToResponseString(uint8_t DNS_response[], string *response_string, unsigned int response_offset);

/* Global Variables */
unsigned int TIMEOUT_SECONDS = 5;
unsigned int RETRIES = 3;
unsigned short PORT = 53;
unsigned char QNAME_LEN;

int main (int argc, char *argv[]) {
    int sock;                        /* Socket descriptor */
    uint16_t query_id;
    uint16_t query_header_flags = 256;
    uint16_t query_header_qdcount = 1;
    uint16_t query_header_ancount = 0;
    uint16_t query_header_nscount = 0;
    uint16_t query_header_arcount = 0;
    struct sockaddr_in serverAddr;   /* server address */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sockaddr_in fromAddr;     /* Source address */
    char *DNSServIP;                    /* IP address of server */
    struct sigaction timeoutAction;  /* signal action for timeouts */
    vector<uint16_t> DNS_message;
    uint8_t DNS_response[MAX_BUFF];
    int respLen;


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

    timeoutAction.sa_handler = catchAlarm; /* calls catchAlarm if a timeout occurs */
    if (sigfillset(&timeoutAction.sa_mask) < 0) /* blocks everything in handler */
        dieWithError((char *)"sigfillset() failed");
    timeoutAction.sa_flags = 0;
    if (sigaction(SIGALRM, &timeoutAction, 0) < 0)
        dieWithError((char *)"sigaction() failed for SIGALRM");

    DNSServIP = argv[argc-2]; /* First arg: server IP address (dotted quad) */
    memmove(DNSServIP, DNSServIP + 1, strlen(DNSServIP));

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError((char *)"socket() failed");

    /* Construct the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));    /* Zero out structure */
    serverAddr.sin_family = AF_INET;                 /* Internet addr family */
    serverAddr.sin_addr.s_addr = inet_addr(DNSServIP);  /* Server IP address */
    serverAddr.sin_port   = htons(PORT);     /* Server port */

    /* Seed the random number */
    srand(time(NULL));
    /* Create Query ID */
    query_id = getQueryID();

    /* Build query to send to the DNS server */
    buildQueryHeader(&DNS_message, query_id, query_header_flags, query_header_qdcount,
              query_header_ancount, query_header_nscount, query_header_arcount);

    buildQueryBody(&DNS_message, argv[argc-1]);
    QNAME_LEN = strlen(argv[argc-1]) + 1;

    fromSize = sizeof(fromAddr);
    while (RETRIES != 0) {
        --RETRIES; // remove 1 retry attempt
        /* Send DNS query to the server */
        if (sendto(sock, DNS_message.data(), DNS_message.size()*sizeof(uint16_t), 0, 
                (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != ((int)DNS_message.size()*sizeof(uint16_t)))
            dieWithError((char *)"sendto() sent a different number of bytes than expected");
        
        alarm(TIMEOUT_SECONDS); // set timeout for TIMEOUT_SECONDS seconds
        respLen = recvfrom(sock, DNS_response, MAX_BUFF, 0, (struct sockaddr *) &fromAddr, &fromSize);

        /* check for timeout */
        if (respLen == -1) {
            if (errno == EINTR) { /* timeout occured */
                if (RETRIES == 0) /* if you are out of attempts die */
                    dieWithError((char *)"timeout retry attempt max reached");
                else 
                    continue; /* skip to next iteration of while loop */
            }
        }
        else
            break;

        /* reset the alarm */
        alarm(0);
    }

    /* process the response */
    processResponse(DNS_response, respLen);

	return 0;
}


/*  processResponse
    input       - DNS response, response length
    output      - none
    description - Processes the response and calls functions
                  that build and print the output.
*/
void processResponse(uint8_t DNS_response[], int respLen) {
    int i;
    char server_is_authoritative;
    char *auth_string;
    char recursion_available;
    uint16_t numResponses;
    uint16_t type;
    unsigned int response_offset = 12;

    recursion_available = (DNS_response[3] & 0x80) >> 7;
    if (recursion_available != 1) dieWithError((char *)"processResponse()"
        " failed - server does not support recuresive queries");

    server_is_authoritative = (DNS_response[2] & 0x04) >> 2;
    if (server_is_authoritative) auth_string = (char *)"auth";
    else auth_string = (char *)"nonauth";
    numResponses = (DNS_response[6] << 8) + DNS_response[7];
    if (numResponses == 0) {
        fprintf(stdout, "NOTFOUND");
        exit(0);
    }

    /* skip the type and class */
    response_offset = skipName(DNS_response, response_offset) + 5; 

    for (i = 0; i < numResponses; ++i) {
        response_offset = skipName(DNS_response, response_offset);
        type = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 4; // skip to time to live
        response_offset = processAnswer(DNS_response, response_offset, type, auth_string);
    }
}


/*  processAnswer
    input       - DNS response, a response offset, a type, and an authoritative description string
    output      - new response offset
    description - prints out the information for an answer in the DNS response message
*/
int processAnswer(uint8_t DNS_response[], unsigned int response_offset, uint16_t type, char *auth_string) {
    string response_string = "";
    int data_length = 0;
    uint32_t time_to_live = 0;
    char time_to_live_str[15];

    if (type == 0x0001) { /* this is A document */
        response_string.append("IP\t");
        time_to_live = (DNS_response[response_offset] << 24) + (DNS_response[response_offset + 1] << 16)
                     + (DNS_response[response_offset + 2] << 8) + (DNS_response[response_offset + 3]);
        response_offset += 4;
        data_length = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 2;
        addIPAddressToResponseString(DNS_response, &response_string, response_offset);
    }
    else if (type == 0x0005) { /* this is a CNAME */
        response_string.append("CNAME\t");
        time_to_live = (DNS_response[response_offset] << 24) + (DNS_response[response_offset + 1] << 16)
                     + (DNS_response[response_offset + 2] << 8) + (DNS_response[response_offset + 3]);
        response_offset += 4;
        data_length = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 2;

        addAliasToResponseString(DNS_response, &response_string, response_offset);
        response_string.push_back('\t');
    }
    else
        dieWithError((char *)"processResponse() failed - invalid response type");


    /* build printed  */
    sprintf(time_to_live_str, "%d", time_to_live);
    response_string.append(time_to_live_str);
    response_string.append("\t");
    response_string.append(auth_string);
    fprintf(stdout, "%s\n", response_string.c_str()); /* print the final string */

    return response_offset + data_length;
}


/*  addIPAddressToResponseString
    input       - DNS_response, response_string, response_offset
    output      - none
    description - appends the IP address to the response string
*/
void addIPAddressToResponseString(uint8_t DNS_response[], string *response_string, unsigned int response_offset) {
    int i;
    uint8_t IP_octet = 0;
    char IP_octet_str[10] = {0};

    IP_octet = DNS_response[response_offset++];
    
    for (i = 0; i < 3; ++i) {
        sprintf(IP_octet_str, "%d", IP_octet);
        response_string->append(IP_octet_str);
        response_string->push_back('.');
        IP_octet = DNS_response[response_offset++];
    }

    sprintf(IP_octet_str, "%d", IP_octet);
    response_string->append(IP_octet_str);

    response_string->push_back('\t');
}

/*  skipName
    input       - DNS_response and response_offset
    output      - new response_offset
    description - skips over name in an answer or query section of a DNS response and
                  returns a new offset that is directly after the previous name info
*/
unsigned int skipName(uint8_t DNS_response[], unsigned int response_offset) {
    while (DNS_response[response_offset] != 0x00) {
        // printf("\nresponse_offset: %d -> %x\n", response_offset, DNS_response[response_offset]);
        ++response_offset;
    }
    return response_offset;  // skip type and class
}


/*  addAliasToResponseString
    input       - DNS response, a response string, and response offset
    output      - none
    description - If the response answer is a CNAME, this function builds the alias name
*/
void addAliasToResponseString(uint8_t DNS_response[], string *response_string, unsigned int response_offset) {
    int i, size;
    if (DNS_response[response_offset] == 0xC0) { /* pointer */
        addAliasToResponseString(DNS_response, response_string, DNS_response[response_offset + 1]);
    }
    else if (DNS_response[response_offset] != 0x00) { /* end of name */
        size = DNS_response[response_offset++];
        for (i = 0; i < size; ++i) {
            response_string->push_back(DNS_response[response_offset++]);
        }
        response_string->push_back('.');
        addAliasToResponseString(DNS_response, response_string, response_offset);
    }
    else 
        return; /* end of name is reached - base case */
}


/*  catchAlarm
    input       - the function takes an int but we ignore it
    output      - none
    description - catches an alarm if a timeout occurs.
*/
void catchAlarm(int ignored) { return; } /* don't do anything in handler, just return */


/*  buildQueryHeader
    input       - DNS message and the header information
    output      - none
    description - appends header information to the DNS_message
*/
void buildQueryHeader(vector<uint16_t> *DNS_message, uint16_t query_id, uint16_t query_header_flags,
                uint16_t query_header_qdcount, uint16_t query_header_ancount,
                uint16_t query_header_nscount, uint16_t query_header_arcount) {

    DNS_message->push_back(htons(query_id));
    DNS_message->push_back(htons(query_header_flags));
    DNS_message->push_back(htons(query_header_qdcount));
    DNS_message->push_back(htons(query_header_ancount));
    DNS_message->push_back(htons(query_header_nscount));
    DNS_message->push_back(htons(query_header_arcount));
}


/*  buildQueryBody
    input       - DNS message and the host name
    output      - none
    description - parses the host name and appends the body of the query to the DNS_message
*/
void buildQueryBody(vector<uint16_t> *DNS_message, char * host_name) {
    vector<uint8_t> label_sizes; /* labels are the names of the subdomains (i.e. www, google, com)*/
    vector<uint8_t> DNS_body;
    int i, j;
    int index = 0;

    if (host_name == NULL)
        dieWithError((char *)"buildQueryBody() failed - host_name can't be NULL");

    getLabelSizes(&label_sizes, host_name);

    for (i = 0; i < (int)label_sizes.size(); ++i) {
        DNS_body.push_back(label_sizes[i]); /* length octet */
        for(j = 0; j < label_sizes[i]; ++j) {
                DNS_body.push_back(*(host_name + index)); /* character octet */
                ++index;
        }
        ++index; /* add 1 to index to remove periods '.' */
    }

    /* adding terminating zero length octet */
    DNS_body.push_back((uint8_t) 0);

    /* adding QTYPE of 0x0001*/
    DNS_body.push_back((uint8_t) 0);
    DNS_body.push_back((uint8_t) 1);

    /* adding QCLASS of 0x0001*/
    DNS_body.push_back((uint8_t) 0);
    DNS_body.push_back((uint8_t) 1);

    addBodyToDNSMessage(&DNS_body, DNS_message);
}


/*  addBodyToDNSMessage
    input       - query header information
    output      - none
    description - appends header information to the DNS_message
*/
void addBodyToDNSMessage(vector<uint8_t> *DNS_body, vector<uint16_t> *DNS_message) {
    int i;
    bool odd_octet = false; /* used for last word in message */
    uint16_t chunk_16bit = 0; /* used for splitting body into 16 bit chunks for htons */

    for(i = 0; i < (int)DNS_body->size(); i++) {
        if (odd_octet) {
            chunk_16bit = chunk_16bit << 8;
            /* put lower 8 bits of data into the 16 bit chunk */
            chunk_16bit += *(DNS_body->data() + i);

            /* push the 16 bit chunk of data onto the DNS_message */
            DNS_message->push_back(htons(chunk_16bit));
            chunk_16bit = 0; /* reset chunk_16bit */
        }
        else {
            /* put the first 8 bits of data in the 16 bit chunk */
            chunk_16bit += *(DNS_body->data() + i);
        }
        odd_octet = !odd_octet;
    }

    /* if there is one octet left, push it onto the DNS_message in a 16 bit chunk */
    if (odd_octet) {
        chunk_16bit = chunk_16bit << 8;
        DNS_message->push_back(htons(chunk_16bit));
    }
}


/*  getLabelSizes
    input       - empty label_sizes vector and a DNS query message
    output      - none
    description - puts label sizes into vector
*/
void getLabelSizes(vector<uint8_t> *label_sizes, char *host_name) {
    uint8_t count = 0;
    int i = 0;

    for(i = 0; i < (int)strlen(host_name); ++i) { /* process string until the end */
        if (*(host_name + i) == '.') {
            label_sizes->push_back(count); /* push the size onto the size vector */
            count = 0; /* reset count */
        }
        else
            count++;
    }
    label_sizes->push_back(count);
}


/*  getQueryID
    input       - none
    output      - random number
    description - generates a random ID for the query
*/
uint16_t getQueryID() {
    return rand() % MAX_RAND_NUMBER + 1;
}


/*  checkFlag
    input       - index and the arguments
    output      - none
    description - checks the optional flag and sets global variables accordingly
*/
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

