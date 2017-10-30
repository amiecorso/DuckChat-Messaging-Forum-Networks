/* Duckchat client program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017
*/

/* 
The client program must take exactly three command-line arguments. The first is the host name where the server is running. The second argument is the port number on which the server is listening. The third argument is the user's username.

When the client starts, it automatically connects to the chat server, joins a channel called “Common”, and provides the user a prompt (i.e. the client must send the join message to join “Common”). When the user types/enters text at the prompt and hits 'Enter', the text is sent to the server (using the "say request" message), and the server relays the text to all users on the channel (including the speaker).

The exception is when the text begins with a forward slash ('/') character. In that case, the text is interpreted as a special command. These special commands are supported by the DuckChat client:

/exit: Logout the user and exit the client software.
/join channel: Join (subscribe in) the named channel, creating the channel if it does not exist.
/leave channel: Leave the named channel.
/list: List the names of all channels.
/who channel: List the users who are on the named channel.
/switch channel: Switch to an existing named channel that user has already joined.host name where the server is running. 
The second argument is the port number on which the server is listening. 
The third argument is the user's username.
TODO:
- does gethostbyname() handle already-IP addresses??
*/

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
#include "raw.h"

// GLOBAL
char active_ch[CHANNEL_MAX]; // currently active channel name


int
main(int argc, char **argv) {
    // MODES
    raw_mode();
    atexit(cooked_mode);

    strcpy(active_ch, "Common"); // default to common

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

    // set up CLIENT INFO and SOCKET
    struct sockaddr_in client_addr; // our local port information
    int sockfd; // socket file descriptor
    if (( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // attempt to open socket
        perror("Client: cannot open socket.");
        return 0;
    } // otherwise, socket is open and good to go.

    //specify CLIENT INFO 
    bzero((char *)&client_addr, sizeof(client_addr)); //set fields to NULL
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY); // let the OS specify the interface and choose the IP address 
    client_addr.sin_port = htons(0); // let the OS choose a free port
    
    //BIND CLIENT socket to port
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Client failed to bind.");
        return 0;
    }

    // specify SERVER INFO
    struct sockaddr_in serv_addr; // remote server information
    bzero((char *)&serv_addr, sizeof(serv_addr));// set fields to NULL
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    // USE NPE
    //inet_pton(AF_INET, SERVER_HOST_IP_ADDRESS, &(serv_addr.sin_addr)); // convert from char[] to network address
    //serv_addr.sin_addr.s_addr = htonl(SERVER_HOST_IP_ADDRESS); do we need to get at sin_addr.s_addr???
    // get IP address from host name...
    struct hostent *hostent_p;
    hostent_p = gethostbyname(SERVER_HOST_NAME);//DNS lookup for host name -> IP
    if (!hostent_p) {
        fprintf(stderr, "Could not obtain address for %s\n", SERVER_HOST_NAME);
        return 0;
    } // and store it in the .sin_addr...
    memcpy((void *)&serv_addr.sin_addr, hostent_p->h_addr_list[0], hostent_p->h_length);

    /*   
    unsigned int alen = sizeof(client_addr);
    if (getsockname(sockfd, (struct sockaddr *)&client_addr, &alen) < 0) {
        perror("getsockname failed");
        return 0;
    }
    printf("bind complete.  Port number = %d\n", ntohs(client_addr.sin_port));
    */    
    // send request to server, receive reply

    // WHILE... until we hit exit.

    char input_buf[1024]; // store gradual input
    int nextin; 
    int buf_in = 0;
    
    while (1){ // main CLIENT WHILE
	while ((nextin = fgetc(stdin)) != '\n') {
	    printf("%c", nextin); // display for user to see
	    input_buf[buf_in++] = (char) nextin; // store in buffer	
	}
	input_buf[buf_in] = '\0';
	printf("\ninput_buf = %s\n", input_buf);
	buf_in = 0;
    } // end while(1)

    // provide prompt to user
	// read input as it is typed? (fgetc)
	// when "enter" is detected, parse input
	// if input did not start with a /, send as text to server
	// if input DID start with a /, read the next word in the input.
	// if the word matches one of our 7 cases, take the corresponding action
		// package up an appropriate message to the server
		// send such message to the server.


    char *test_message = "this is a test message\n";
    if (sendto(sockfd, test_message, strlen(test_message), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("sendto failed");
        return 0;
    }  
    close(sockfd);
    return 0; // ??
}

/* ========== HELPER FUNCTIONS ==============================================*/
/* classify_input takes a message typed by the user (after hitting enter)
and returns either the request_t CODE (int) of the message, or -1 if bad format.
If message is too long, returns -1 and prints error. */
int
classify_input(char *in)
{
    if (in[0] != '/') { // SAY request 
	if (strlen(in) > 64) { // test for length
	    printf("Message is too long. 64 characters max.\n");
	    return -1;
	}
	return 4; // otherwise, we're good, return code 4
    }
    else { // we have a leading '/'
	if (strncmp(in, "/exit ", 6) == 0) { // EXIT

	}
	if (strncmp(in, "/join ", 6) == 0) { // JOIN

	}
	if (strncmp(in, "/leave ", 7) == 0) { // LEAVE

	}
	if (strncmp(in, "/list ", 6) == 0 ) { // LIST

	} 
	if (strncmp(in, "/who ", 5) == 0) { // WHO

	}
	if (strncmp(in, "/switch ", 8) == 0) { // SWITCH --> create own return code, 8

	}
	else { // either matches nothing or is missing space after request
	    printf("Unknown request.\n");
	    return -1;
	}
    }
} // end classify_input

/* pack_request uses the code type given to correctly prepare a structure with the right contents
to be sent to the server.  Returns a void * */
void *
pack_request(request_t code, char *msg) // should this be a char * or a request container?
{
    // switch case on code...
}
