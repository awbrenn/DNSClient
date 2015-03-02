CC = gcc
PROG = DNSClient

DEPS = DNSClient.h

DNSCLIENTSRC = DNSClient.c DieWithError.c
DNSCLIENTOBJS = $(CLIENTSRC:.c=.o)

all: $(PROG)

DNSClient: $(DNSCLIENTOBJS)
	${CC} $@.c $(DNSCLIENTOBJS) -o client

backup:
	rm -f DNSClient.tar.gz
	tar -cf DNSClient.tar *.c *.h readme.txt Makefile
	gzip -f DNSClient.tar

clean:
	rm -f ${PROG} *.o *.out client
