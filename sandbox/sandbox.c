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
#include "raw.h"
#include <sys/time.h> // select
#include <sys/types.h> // select
//#include "duckchat.h"

#define UNUSED __attribute__((unused))
int
main(UNUSED int argc, UNUSED char **argv) {
    raw_mode();
    atexit(cooked_mode);
    char input_buf[1024]; // store gradual input
    int nextin; 
    int buf_in = 0;
    
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds); // make stdin part of the rfds

    tv.tv_sec = 1; // wait up to one second
    tv.tv_usec = 0;
    while (1) {
	    while (!(retval = select(1, &rfds, NULL, NULL, &tv))) {
	        //printf("WAITING\n");
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
            }
	    while ((nextin = fgetc(stdin)) != '\n') {
	        printf("%c\n", nextin);
		input_buf[buf_in++] = (char)nextin;
	    }
	    input_buf[buf_in] = '\0';
            printf("input_buf = %s\n", input_buf);
            buf_in = 0;
    } // end while(1)
    return 0; // main
}
