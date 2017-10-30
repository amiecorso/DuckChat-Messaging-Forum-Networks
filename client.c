/* Duckchat client program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017
*/

/* 
TODO:
- does gethostbyname() handle already-IP addresses??
*/

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
#include "duckchat.h"
#include "raw.h"

#define UNUSED __attribute__((unused))
// forward declarations
int help_getword(char buf[], int i, char word[]);
int help_strchr(char buf[], char c);
int classify_input(char *in);
int pack_request(request_t code, char *input, void **next_request);
int extract_ch(char *input, char *buf);


// GLOBAL
char active_ch[CHANNEL_MAX]; // currently active channel name
char SERVER_HOST_NAME[64]; // is this buffer size ok??
int SERVER_PORT; 
char USERNAME[USERNAME_MAX];


int
main(int argc, char **argv) {
    // MODES
    raw_mode();
    atexit(cooked_mode);

    // Parse command-line arguments
    if (argc < 4) {
        perror("Client: missing arguments.");
        return 0;
    }
    // TODO: ADD ARG ERROR CHECKING
	// - username can't be longer than 32 chars
	// error checking for bad returns from gethostbyname()
    strcpy(SERVER_HOST_NAME, argv[1]);
    SERVER_PORT = atoi(argv[2]);
    strcpy(USERNAME, argv[3]);
    strcpy(active_ch, "Common"); // default to common

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
    struct hostent *hostent_p; // for getting IP address from host name
    hostent_p = gethostbyname(SERVER_HOST_NAME);//DNS lookup for host name -> IP
    if (!hostent_p) {
        fprintf(stderr, "Could not obtain address for %s\n", SERVER_HOST_NAME);
        return 0;
    } // and store it in the .sin_addr...
    memcpy((void *)&serv_addr.sin_addr, hostent_p->h_addr_list[0], hostent_p->h_length);

    /* CONNECTION ESTABLISHED */

    // send initial LOGIN request
    struct request_login login_req = {0}; // zero-out struct
    login_req.req_type = 0;
    strcpy(login_req.req_username, USERNAME);
    if (sendto(sockfd, (void *)&login_req, 36, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	perror("Client: Login failed");
	return 0;
    }  
    // initial JOIN [Common] request
    struct request_join join_req = {0};
    join_req.req_type = 2;
    strcpy(join_req.req_channel, active_ch); // should be common by default
    if (sendto(sockfd, (void *)&join_req, 36, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	perror("Client: Failed to join common.");
	return 0;
    }  
    
    /* MAIN WHILE LOOP */

    char input_buf[1024] = {0}; // store gradual typed input
    int nextin;     // for storing chars 
    int buf_in = 0; // for storing chars
    request_t code; 
    void *next_request;
    int bytes_to_send;

    // set up fields for SELECT()
    fd_set rfds;      // set of file descriptors for select() to watch
    struct timeval tv; // timeout interval
    int retval;        // stores return val of select call
    int fdmax = sockfd; // largest file desc

    FD_ZERO(&rfds);    // clears out rfds
    FD_SET(0, &rfds); // make stdin part of the rfds
    FD_SET(sockfd, &rfds); // make our socket part of the rfds

    tv.tv_sec = 0;
    tv.tv_usec = 500000; // check every half second
    while (1) {
	retval = select(fdmax + 1, &rfds, NULL, NULL, &tv);
	if (retval > 0 && FD_ISSET(sockfd, &rfds)) { // was it the server?
	    //response = recv(sockfd, buff...
	// get reponse and use
	// reset select fields?? are we going to go back into while loop??
	printf("It was the server!");
	}
	else if (retval > 0 && FD_ISSET(0, &rfds)) { // was it the user typing?
		while ((nextin = fgetc(stdin)) != '\n') { // waiting for enter key
		    printf("%c", nextin); // display for user to see
		    input_buf[buf_in++] = (char) nextin; // store in buffer	
		}
		input_buf[buf_in] = '\0';
		code = (request_t) classify_input(input_buf);
		printf("\nrequest CODE = %d\n", code);
		bytes_to_send = pack_request(code, input_buf, &next_request);
		// SEND CASES
		if (bytes_to_send == -1) {
		    //problem
		    printf("Invalid command.\n");
		}
		else if (bytes_to_send == 0) {
			//switch case!
		}
		else {
		    if (sendto(sockfd, next_request, bytes_to_send, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			perror("sendto failed");
			return 0; // DO WE WANT TO ACTUALLY RETURN?
		    }  
		    free(next_request);
		} // end WHILE WAITING FOR ENTER KEY
		buf_in = 0; // reset our buffer's index.
	} // end if-user

	FD_ZERO(&rfds); // reset select fields
	FD_SET(0, &rfds);
	FD_SET(sockfd, &rfds); 
	tv.tv_sec = 0;
	tv.tv_usec = 500000; 
    } // end while(1)


    close(sockfd); // close our socket
    return 0; // MAIN return
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
	if (strncmp(in, "/exit", 5) == 0) // EXIT
	    return 1;
	if (strncmp(in, "/join", 5) == 0) // JOIN
	    return 2;
	if (strncmp(in, "/leave", 6) == 0) // LEAVE
	    return 3;
	if (strncmp(in, "/list", 5) == 0 ) // LIST
	    return 5;
	if (strncmp(in, "/who", 4) == 0) // WHO
	    return 6;
	if (strncmp(in, "/switch", 7) == 0) // SWITCH --> create own return code, 8
	    return 0;
	printf("Unknown request.\n");
	return -1;
	}
} // end classify_input

/* pack_request uses the code type given to correctly prepare a structure with the right contents
to be sent to the server.  Returns a void * */
int
pack_request(request_t code, char *input, void **next_request)
{
    char channel_buf [32]; // for the cases that require us to get a channel
    switch (code) {
	case 1: {// EXIT
	    struct request_logout *logout_req = (struct request_logout *)malloc(sizeof(struct request_logout));
	    logout_req->req_type = REQ_LOGOUT;
	    *next_request = logout_req;
	    return 4;
	}
	case 2: {// JOIN (req_channel[CHANNEL_MAX])
	    if (extract_ch(input, channel_buf) == -1)
		return -1;
	    struct request_join *join_req  = (struct request_join *)malloc(sizeof(struct request_join));
	    join_req->req_type = REQ_JOIN;
	    strcpy(join_req->req_channel, channel_buf); // put the channel in the struct
	    *next_request = join_req;
	    return 36;
	}
	case 3: {// LEAVE (req_channel)
	    if (extract_ch(input, channel_buf) == -1)
		return -1;
	    struct request_leave * leave_req = (struct request_leave *)malloc(sizeof(struct request_leave));
	    leave_req->req_type = REQ_LEAVE;
	    strcpy(leave_req->req_channel, channel_buf); // put the channel in the struct
	    *next_request = leave_req;
	    return 36;
	}
	case 4: {// SAY
	    struct request_say *say_req = (struct request_say *)malloc(sizeof(struct request_say));
	    say_req->req_type = REQ_SAY;
	    strcpy(say_req->req_channel, active_ch); // store active channel
	    strcpy(say_req->req_text, input); // store msg 
	    *next_request = say_req;
	    return 100;
	}
	case 5: {// LIST 
	    struct request_list *list_req = (struct request_list *)malloc(sizeof(struct request_list));
	    list_req->req_type = REQ_LIST;
	    *next_request = list_req;
	    return 4;
	}
	case 6: {// WHO (req_channel)
	    if (extract_ch(input, channel_buf) == -1)
		return -1;
	    struct request_who *who_req = (struct request_who *)malloc(sizeof(struct request_who));
	    who_req->req_type = REQ_WHO;
	    strcpy(who_req->req_channel, channel_buf); // put the channel in the struct
	    *next_request = who_req;
	    return 36;
	}
	case 8: { // SWITCH (req_channel) ?? weird case
	    if (extract_ch(input, channel_buf) == -1)
		return 8;
	}
    }
    return -1;  // if no good code
}

/* extracts second word from input and stores in buf.
Returns 0 if successful, -1 if unsuccessful */
int
extract_ch(char *input, char *buf)
{
    int w = 0;
    char word[32]; // word buffer
    w = help_getword(input, w, word); // this should pass the command ("join")
    if (w == -1) {
	printf("Improper format: missing channel.\n");
	return -1;
    }
    w = help_getword(input, w, word); // now we SHOULD have a channel in here. (Server will let us know if it's legit)
    // should we check for EXTRA args here?? (like if there are more words to grab?)
    strcpy(buf, word); // put the word in our buffer
    return 0;
}



/*
CREDIT: Joe Sventek wrote this function available for use in our 415 projects.
fetch next blank-separated word from buffer into word
return value is index into buffer for next search or -1 if at end
assumes that word[] is large enough to hold the next word
 */
static char *singlequote = "'";
static char *doublequote = "\"";
static char *whitespace = " \t";

int help_getword(char buf[], int i, char word[]) {
    char *tc, *p;

    /* skip leading white space */
    while(help_strchr(whitespace, buf[i]) != -1)
        i++;
    /* buf[i] is now '\0' or a non-blank character */
    if (buf[i] == '\0')
        return -1;
    p = word;
    switch(buf[i]) {
    case '\'': tc = singlequote; i++; break;
    case '"': tc = doublequote; i++; break;
    default: tc = whitespace; break;
    }
    while (buf[i] != '\0') {
        if (help_strchr(tc, buf[i]) != -1)
            break;
        *p++ = buf[i];
        i++;
    }
    /* either at end of string or have found one of the terminators */
    if (buf[i] != '\0') {
        if (tc != whitespace) {
            i++;	/* skip over terminator */
        }
    }
    *p = '\0';
    return i;
}

/* 
CREDIT: Joe Sventek wrote this function for a 415 project.
help_strchr - return the array index of leftmost occurrence of 'c' in 'buf'
return -1 if not found
 */
int help_strchr(char buf[], char c) {
    int i;
    for (i = 0; buf[i] != '\0'; i++)
        if (buf[i] == c)
            return i;
    return -1;
}
