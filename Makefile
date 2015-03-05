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
	./dnsq -p 6767 -t 2 -r 3 @192.168.1.254 gmail.google.com

backup:
	rm -f DNSClient.tar.gz
	tar -cf DNSClient.tar *.cpp *.h readme.txt Makefile
	gzip -f DNSClient.tar

clean:
	rm -f dnsq *.o *.out client
