// SANDBOX


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> // select
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include "duckchat.h"

char HOST_NAME[64]; // is this buffer size ok??
int PORT; 


void print_debug_msg(struct sockaddr_in recv_addr, struct request *raw_request);


int
main(int argc, char **argv) {
    struct sockaddr_in serv_addr;
    strcpy(HOST_NAME, argv[1]);
      
      
}    


// prints debug diagnostic to console
void print_debug_msg(struct sockaddr_in recv_addr, struct request *raw_request)
{
    int msgtype; // 1, 2, 3, ...
    char *my_ip; // printable version of ip address (make global?)
    int my_port = PORT; // same
    char *their_ip; // extract from recv_addr
    int their_port; // extract from recv_addr
    char *send_or_recv; // send or recv
    char *type; // i.e. say, join, etc.
    char *username; // if necessary 
    char *channel;  // if necessary
    
}
