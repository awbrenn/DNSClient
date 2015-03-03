CC = g++
PROG = DNSClient

DEPS = DNSClient.h
CPLUSFLAGS = -c -ggdb -O2 -Wall

DNSCLIENTSRC = DNSClient.cpp ErrorFunctions.cpp
DNSCLIENTOBJS = $(CLIENTSRC:.cpp=.o)


all: $(PROG)

DNSClient: $(DNSCLIENTOBJS)
	${CC} $(CPLUSFLAGS) -c -o $@.o $@.cpp
	${CC} $(CPLUSFLAGS) -c -o ErrorFunctions.o ErrorFunctions.cpp
	${CC} -o dnsq $@.o ErrorFunctions.o

test:
	./dnsq -p 57 -t 14 -r 12 @192.168.1.254 www.clemson.edu

backup:
	rm -f DNSClient.tar.gz
	tar -cf DNSClient.tar *.cpp *.h readme.txt Makefile
	gzip -f DNSClient.tar

clean:
	rm -f dnsq *.o *.out client
