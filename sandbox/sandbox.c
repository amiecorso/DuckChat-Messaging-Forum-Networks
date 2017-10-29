// SANDBOX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "duckchat.h"

int
main(int argc, char **argv) {
    // Parse command-line arguments
    if (argc < 4) {
        perror("Client: missing arguments.");
        return 0;
    }
    // add error checking?? is it ok that these are upper case??
    // host name where server is running
    char SERVER_HOST_NAME[64]; // is this buffer size ok??
    //TODO: check for "localhost" and convert to 127.00..??
    strcpy(SERVER_HOST_NAME, argv[1]);
    // port number on which server is listening
    int SERVER_PORT = atoi(argv[2]);
    // user's username
    char USERNAME[USERNAME_MAX];
    strcpy(USERNAME, argv[3]);
    printf("Port # %d\n", SERVER_PORT);
    
    // specify SERVER INFO
    struct hostent *hostent_p;
    hostent_p = gethostbyname(SERVER_HOST_NAME);//DNS lookup for host name -> IP
    if (!hostent_p) {
        fprintf(stderr, "Could not obtain address for %s\n", SERVER_HOST_NAME);
        return 0;
    }
    int i;
    for (i = 0; hostent_p->h_addr_list[i] != 0; i++) {
        paddr((unsigned char*) hostent_p->h_addr_list[i]);
    }

}
