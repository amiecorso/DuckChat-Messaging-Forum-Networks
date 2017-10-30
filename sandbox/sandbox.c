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
//#include "duckchat.h"

#define UNUSED __attribute__((unused))
int
main(UNUSED int argc, UNUSED char **argv) {
    char input_buf[1024]; // store gradual input
    int nextin; 
    int buf_in = 0;
    
    while (1){
	nextin = fgetc(stdin);
	printf("nextin = %c\n", nextin);
	    if (nextin == '\r') {
	        input_buf[buf_in] = '\0';
		printf("input_buf = %s\n", input_buf);
		buf_in = 0;
	    }
	input_buf[buf_in++] = (char) nextin;	
    } // end while(1)
}
