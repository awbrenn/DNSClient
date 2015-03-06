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
    struct hostent *the_host;         /* Hostent from gethostbyname() */
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

    timeoutAction.sa_handler = catchAlarm; /* callse catchAlarm if a timeout occurs */
    if (sigfillset(&timeoutAction.sa_mask) < 0) /* blocks everything in handler */
        dieWithError((char *)"sigfillset() failed");
    timeoutAction.sa_flags = 0;
    if (sigaction(SIGALRM, &timeoutAction, 0) < 0)
        dieWithError((char *)"sigaction() failed for SIGALRM");

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
        dieWithError((char *)"socket() failed");

    /* Construct the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));    /* Zero out structure */
    serverAddr.sin_family = AF_INET;                 /* Internet addr family */
    serverAddr.sin_addr.s_addr = inet_addr(DNSServIP);  /* Server IP address */
    serverAddr.sin_port   = htons(PORT);     /* Server port */

    /* If user gave a dotted decimal address, we need to resolve it  */
    if (serverAddr.sin_addr.s_addr == -1) {
        the_host = gethostbyname(DNSServIP);
        serverAddr.sin_addr.s_addr = *((unsigned long *) the_host->h_addr_list[0]);
    }

    /* Seed the random number */
    srand(time(NULL));
    /* Create Query ID */
    query_id = getQueryID();

    /* Build query to send to the DNS server */
    buildQueryHeader(&DNS_message, query_id, query_header_flags, query_header_qdcount,
              query_header_ancount, query_header_nscount, query_header_arcount);

    buildQueryBody(&DNS_message, argv[argc-1]);
    QNAME_LEN = strlen(argv[argc-1]) + 1;


    // printf("\nDNS_message ID: %x  %x\n", query_id, htons(query_id));
    // printf("\nDNS_message qdcount: %d %d\n", htons(query_header_qdcount), *(DNS_message.data() + 2));

    // printf("\nsize of DNS_message.data() %d\n", DNS_message.size()*sizeof(uint16_t));

    // printf("\n%s, %d\n", DNSServIP, PORT);


    fromSize = sizeof(fromAddr);
    while (RETRIES != 0) {
        --RETRIES; // remove 1 retry attempt
        /* Send DNS query to the server */
        if (sendto(sock, DNS_message.data(), DNS_message.size()*sizeof(uint16_t), 0, 
                (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != DNS_message.size()*sizeof(uint16_t))
            dieWithError((char *)"sendto() sent a different number of bytes than expected");

        // printf("\nDNS_message size: %lu\n", DNS_message.size()*sizeof(uint16_t));
        
        alarm(TIMEOUT_SECONDS); // set timeout for TIMEOUT_SECONDS seconds
        respLen = recvfrom(sock, DNS_response, MAX_BUFF, 0, (struct sockaddr *) &fromAddr, &fromSize);

        // printf("\nrespLen: %d\n", respLen);
        /* response error handler */
        if (respLen == -1) {
            if (errno == EINTR) { /* timeout occured */
                if (RETRIES == 0)
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

    processResponse(DNS_response, respLen);



	return 0;
}


void processResponse(uint8_t DNS_response[], int respLen) {
    printf("\n");
    int i;
    // for (i = 0; i < respLen; ++i) {
    //     printf("%x ", *(DNS_response + i));
    // }
    // printf("\n");

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
    // printf("\n%s\n", auth_string);
    numResponses = (DNS_response[6] << 8) + DNS_response[7];
    // printf("\n%d\n", numResponses);
    if (numResponses == 0) {
        fprintf(stdout, "NOTFOUND");
        exit(0);
    }


    response_offset = skipName(DNS_response, response_offset) + 5;

    for (i = 0; i < numResponses; ++i) {
        response_offset = skipName(DNS_response, response_offset);
        type = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 4; // skip to time to live
        response_offset = processAnswer(DNS_response, response_offset, type, auth_string);
    }

    // response_index = 12 + QNAME_LEN + 5; /* set response index to the beggining of the answer section */


    // for (i = 0; i < numResponses; ++i) {
    //     label_length = DNS_response[response_index];
    //     addLabelsToResponseString(&DNS_response, &response_string, label_length, response_index);
    // }
}


int processAnswer(uint8_t DNS_response[], unsigned int response_offset, uint16_t type, char *auth_string) {
    string response_string = "";
    int data_length = 0;
    uint32_t time_to_live = 0;

    // printf("\nresponse_offset: %d -> %x\n", response_offset, DNS_response[response_offset]);
    if (type == 0x0001) {
        response_string.append("IP\t");
        time_to_live = (DNS_response[response_offset] << 24) + (DNS_response[response_offset + 1] << 16)
                     + (DNS_response[response_offset + 2] << 8) + (DNS_response[response_offset + 3]);
        response_offset += 4;
        data_length = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 2;
        addIPAddressToResponseString(DNS_response, &response_string, response_offset);
    }
    else if (type == 0x0005) {
        response_string.append("CNAME\t");
        time_to_live = (DNS_response[response_offset] << 24) + (DNS_response[response_offset + 1] << 16)
                     + (DNS_response[response_offset + 2] << 8) + (DNS_response[response_offset + 3]);
        response_offset += 4;
        data_length = (DNS_response[response_offset] << 8) + DNS_response[response_offset + 1];
        response_offset += 2;
        // printf("\nAlias begin: %d -> %x\n", response_offset, DNS_response[response_offset + 1]);

        addAliasToResponseString(DNS_response, &response_string, response_offset);
        response_string.push_back('\t');
    }
    else
        dieWithError((char *)"processResponse() failed - invalid response type");

    // printf("\nresponse_offset: %d -> %x\n", response_offset, DNS_response[response_offset]);

    response_string.append(to_string(time_to_live));
    response_string.append("\t");
    response_string.append(auth_string);
    printf("\n%s\n", response_string.c_str());

    return response_offset + data_length;
}


void addIPAddressToResponseString(uint8_t DNS_response[], string *response_string, unsigned int response_offset) {
    int i;
    uint8_t IP_octet;

    for (i = 0; i < 3; ++i) {
        IP_octet = DNS_response[response_offset++];
        response_string->append(to_string((int)IP_octet) + ".");
    }

    IP_octet = DNS_response[response_offset++];
    response_string->append(to_string((int)IP_octet));

    response_string->push_back('\t');
}


unsigned int skipName(uint8_t DNS_response[], unsigned int response_offset) {
    while (DNS_response[response_offset] != 0x00) {
        // printf("\nresponse_offset: %d -> %x\n", response_offset, DNS_response[response_offset]);
        ++response_offset;
    }
    return response_offset;  // skip type and class
}


void addAliasToResponseString(uint8_t DNS_response[], string *response_string, unsigned int response_offset) {
    int i, size;
    // printf("\nGot here %d\n", response_offset);
    if (DNS_response[response_offset] == 0xC0) { /* pointer */
        // printf("\n---1---\n");
        addAliasToResponseString(DNS_response, response_string, DNS_response[response_offset + 1]);
    }
    else if (DNS_response[response_offset] != 0x00) { /* end of name */
        //printf("\n%d -> %x\n", response_offset, DNS_response[response_offset]);
        size = DNS_response[response_offset++];
        // printf("\n%d\n", size);
        for (i = 0; i < size; ++i) {
            // printf("%d -> %x\n",response_offset, DNS_response[response_offset]);
            response_string->push_back(DNS_response[response_offset++]);
            //usleep(250000);
        }
        response_string->push_back('.');
        // printf("\n---2---\n");
        // printf("\nDNS_response[response_offset]: %x\n", DNS_response[response_offset]);
        addAliasToResponseString(DNS_response, response_string, response_offset);
    }
    else return; /* end of name is reached - base case */
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

    for (i = 0; i < label_sizes.size(); ++i) {
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

    for (i = 0; i < DNS_body.size(); ++i) {
        printf("%x ", DNS_body[i] & 0xff);
    }
    cout << endl;

    addBodyToDNSMessage(&DNS_body, DNS_message);

    for (i = 0; i < DNS_message->size(); ++i) {
        printf("%x ", *(DNS_message->data() + i) & 0xffff);
    }
    cout << endl;

    // printf("\n%d\n", label_sizes.size());

    // for (i = 0; i < label_sizes.size(); ++i)
    //     printf("%d ", label_sizes[i]);
    // printf("\n");



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

    for(i = 0; i < DNS_body->size(); i++) {
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


void getLabelSizes(vector<uint8_t> *label_sizes, char *host_name) {
    uint8_t count = 0;
    int i = 0;

    for(i = 0; i < strlen(host_name); ++i) { /* process string until the end */
        if (*(host_name + i) == '.') {
            label_sizes->push_back(count); /* push the size onto the size vector */
            count = 0; /* reset count */
        }
        else
            count++;
    }
    label_sizes->push_back(count);
}


uint16_t getQueryID() {
    return rand() % MAX_RAND_NUMBER + 1;
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

