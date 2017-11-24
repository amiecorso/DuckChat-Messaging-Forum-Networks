cc=gcc

CFLAGS=-Wall -W -g -Werror 


all: client server

client: client.c raw.c
	$(cc) client.c raw.c $(CFLAGS) -o client

server: server2.c 
	$(cc) server2.c $(CFLAGS) -o server

clean:
	rm -f client server *.o

