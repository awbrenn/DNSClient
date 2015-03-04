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

/* Global Variables */
unsigned int TIMEOUT_SECONDS = 5;
unsigned int RETRIES = 3;
unsigned short PORT = 53;

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

    // struct sockaddr_in fromAddr;     /* Source address */
    // struct hostent *thehost;         /* Hostent from gethostbyname() */
    // unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *DNSServIP;                    /* IP address of server */
    struct sigaction timeoutAction;  /* signal action for timeouts */
    vector<uint16_t> DNS_message;
    uint8_t *DNS_response


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



    // printf("\nDNS_message ID: %x  %x\n", query_id, htons(query_id));
    // printf("\nDNS_message qdcount: %d %d\n", htons(query_header_qdcount), *(DNS_message.data() + 2));

    // printf("\nsize of DNS_message.data() %d\n", DNS_message.size()*sizeof(uint16_t));

    /* Send DNS query to the server */
    if (sendto(sock, DNS_message.data(), DNS_message.size()*sizeof(uint16_t), 0, 
            (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != DNS_message.size()*sizeof(uint16_t))
        DieWithError("sendto() sent a different number of bytes than expected");

    recvfrom(sock, respStringBuffer, MAX_BUFF, 0, 
                                (struct sockaddr *) &fromAddr, &fromSize);


	return 0;
}


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

