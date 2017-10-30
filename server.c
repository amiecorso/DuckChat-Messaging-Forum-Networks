/* DuckChat Server program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017
*/

/*

The server takes two arguments: the host address to bind to and the port number. The host address can be either 'localhost', the IP host name of the machine that the server is running on, or the IP address of an interface connected to the machine. Once the server is running, clients may use the host name and port to connect to the server. Note that if you use 'localhost', you will not be able to connect to the server from another machine, but you also do not have to worry about dropped packets.
The server does not need to directly interact with the user in any way. However, it is strongly recommended that the server outputs debugging messages to the console. For example, each time the server receives a message from a client, it can output a line describing the contents of the message and who it is from using the following format: [channel][user][message] where message denotes a command and its parameters (if any).

Channel creation and deletion at server are handled implicitly. Whenever a channel has no users, it is deleted. Whenever a user joins a channel that did not exist, it is created.

If the server receives a message from a user who is not logged in, the server should silently ignore the message.

1) establish server data structures
userlist and a chanlist: structs with just HEADs to linked list of unodes and chnodes.
unode and chnode? MAKE TWO THINGS
users = linked list of pointers to users (should users itself be the head pointer??)
channels = head of linked list of pointers to channels
user = struct with:
- username
- active channel
- socket fd
- last-active (gettimeofday that user was last active)
- HEAD of a linked list of channel pointers

channel = struct with:
- channel name
- USERCOUNT
- HEAD of linked list of user pointers

Server will create a ulist and a chanlist upon its establishment, and free them upon exit??
2) figure out how to multiplex client sockets (and associate with user names)
    ???

3) write functions to handle each of the 8 possible requests
   --> some of these requests will package up packets of info and send to correct client
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "duckchat.h"

#define PORT 4000 // this will have to be replaced by an argument
#define BUFSIZE 2048

/* Forward Declarations */
typedef struct user_data user;
typedef struct channel_data channel;
typedef struct ulist_node unode;
typedef struct chlist_node cnode;

void login(char *uname);
void logout(char *uname);
void join(char *uname, char *cname);
void leave(char *uname, char *cname);
void say(char *uname, char *cname);
void list(char *uname, char *cname);
void who(uname, cname);
void keep_alive(char *uname); //update time stamp
	// helpers:
user *create_user(char *uname);		  // creates (if needed) a new user and installs in list
channel *create_channel(char *cname);	  // creates (if needed) a new channel and installs in list
user *find_user(char *uname, unode *head);	  // searches list for user with given name and returns pointer to user
channel *find_channel(char *cname, cnode *head);  // searches list for channel of given name and returns pointer to channel

// consider writing an install_user and install_channel function that takes a user * and a list HEAD...
int delete_user(user *u);    	  i       // removes user from all channels, frees user data, deletes list node
int delete_channel(channel *c);	          // frees channel data, deletes list node
int add_utoch(channel *c, char *uname);  // adds user to specified channel's list of users
int add_chtou(user *u, char *cname);  // adds channel to specified user's list of channels
int rm_ufromch(channel *c, char *uname); // removes user from specified channel's list of users
int rm_chfromu(user *u, char *cname); // removes channel from specified user's list of channels

struct user_data {
    char username[USERNAME_MAX];
    int sock_fd;
    struct timeval last_active; // to populate with gettimeofday() and check activity    
    cnode *mychannels; // pointer to linked list of channel pointers
};

struct channel_data {
    char channelname[CHANNEL_MAX];
    int user_count; // channel gets deleted when this becomes 0;  rm_ufromch decrements
    unode *myusers; // pointer to linked list of user pointers
};

struct ulist_node {
    user *u;
    unode *next;
};

struct clist_node {
    channel *c;
    cnode *next;
};


/* GLOBALS */
unode *uhead; // head of my linked list of user pointers
ccnode *chead; // head of my linked list of channel pointers


/*========= MAIN ===============*/
int
//main(int argc, char **argv) {
main() {
    struct sockaddr_in serv_addr, remote_addr;

    socklen_t addr_len = sizeof(remote_addr);
    int rec_len, sockfd;
    unsigned char buf[BUFSIZE]; // receive buffer
    // TODO parse command line arguments (host address, port num)

    if (( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("server: can't open socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
    serv_addr.sin_port = htons(PORT); //host to network short

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("Server: can't bind local address");


    while(1) {
        /* now loop, receiving data and printing what we received */
        for (;;) {
                printf("waiting on port %d\n", PORT);
                rec_len = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&remote_addr, &addr_len);
                printf("received %d bytes\n", rec_len);
                if (rec_len > 0) {

                        buf[rec_len] = '\0';
                        printf("received message: \"%s\"\n", buf);
                }
                sendto(sockfd, buf, strlen((const char *)buf), 0, (struct sockaddr *)&remote_addr, addr_len);
        }
        // do we ever need to close the socket??
        // close(sockfd); ??
    }
} // END MAIN

/* ================ HELPER FUNCTIONS ========================================= */


void login(char *uname)
{
}

void logout(char *uname)
{
}

void join(char *uname, char *cname)
{
}

void leave(char *uname, char *cname)
{
}

void say(char *uname, char *cname)
{
}

void list(char *uname, char *cname)
{
}

void who(uname, cname)
{
}

void keep_alive(char *uname) //update time stamp
{
}

/* DATA MANIPULATORS */
user *create_user(char *uname)		  // creates (if needed) a new user and installs in list
{
    user *newuser = (user *)malloc(sizeof(user));
    if (newuser == NULL)
	fprintf(stderr, "Error malloc'ing new user: %s\n", uname);
    strcpy(newuser->username, uname);         // add the username
    gettimeofday(newuser->last_active, NULL); // user's initial activity time
    add_chtou(newuser, "Common");             // Initially active on Common
    // install in list of users
    unode *newnode = (unode *)malloc(sizeof(unode)); // new user list node
    newnode->u = newuser;  // data is pointer to our new user struct
    newnode->next = uhead; // new node points at whatever head WAS pointing at
    uhead = newnode;       // head is now our new node
    return newuser;			      // finally, return pointer to newly created user
}

channel *create_channel(char *cname)	  // creates (if needed) a new channel and installs in list
{
    channel *newchannel = (channel *)malloc(sizeof(channel));
    if (newchannel == NULL)
	fprintf(stderr, "Error malloc'ing new channel; %s\n", cname);
    strcpy(newchannel->channelname, cname);    
    newchannel->count = 0;
    newchannel->myusers = NULL; // initially no users on channel
    // install in list of channels
    cnode *newnode = (cnode *)malloc(sizeof(cnode)); // new channel list node
    newnode->c = newchannel;
    newnode->next = chead;
    chead = newnode;
    return newchannel; // finally, return pointer to new channel
}

user *find_user(char *uname, unode *head)	  // searches list for user with given name and returns pointer to user
{
    unode *nextnode = head; // start at the head
    while (nextnode != NULL) { // while we've still got list to search...
        if (strcmp((nextnode->u)->username, uname) == 0) { // if we've got a match
	    return nextnode->u; // return pointer to this user
        }
        nextnode = nextnode->next; // move down the list
    }
    return NULL;
}

channel *find_channel(char *cname, cnode *head)  // searches list for channel of given name and returns pointer to channel
{
    cnode *nextnode = head; // start at the head
    while (nextnode != NULL) { // while we've still got list to search...
        if (strcmp((nextnode->c)->channelname, cname) == 0) { // if we've got a match
	    return nextnode->c; // return pointer to this user
        }
        nextnode = nextnode->next; // move down the list
    }
    return NULL;
}

// consider writing an install_user and install_channel function that takes a user * and a list HEAD...
int delete_user(user *u)    	  i       // removes user from all channels, frees user data, deletes list node
{
    // go through user's channel's and remove user from each channel (decrement channel counts)

    // free the user's linked list (channel) nodes

    // free the user's struct

    // delete the node the ulist
}

int delete_channel(channel *c)	          // frees channel data, deletes list node
{
    // channel SHOULD have no more user's in list

    // free the struct

    // delete the node in the clist   
}

int add_utoch(channel *c, char *uname)  // adds user to specified channel's list of users
{

}

int add_chtou(user *u, char *cname)  // adds channel to specified user's list of channels
{

}

int rm_ufromch(channel *c, char *uname) // removes user from specified channel's list of users
{

}

int rm_chfromu(user *u, char *cname) // removes channel from specified user's list of channels
{

}

