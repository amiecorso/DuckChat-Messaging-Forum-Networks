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
unode *create_user(char *uname);		  // creates (if needed) a new user and installs in list
cnode *create_channel(char *cname);	  // creates (if needed) a new channel and installs in list
unode *find_user(char *uname, unode *head);	  // searches list for user with given name and returns pointer to user
cnode *find_channel(char *cname, cnode *head);  // searches list for channel of given name and returns pointer to channel

// consider writing an install_user and install_channel function that takes a user * and a list HEAD...
void delete_user(char *uname);    	  i       // removes user from all channels, frees user data, deletes list node
void delete_channel(char *cname);	          // frees channel data, deletes list node
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
    unode *prev;
};

struct clist_node {
    channel *c;
    cnode *next;
    unode *prev;
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
unode *create_user(char *uname)		  // creates (if needed) a new user and installs in list
{
    user *newuser = (user *)malloc(sizeof(user));
    if (newuser == NULL)
	fprintf(stderr, "Error malloc'ing new user: %s\n", uname);
    strcpy(newuser->username, uname);         // add the username
    gettimeofday(newuser->last_active, NULL); // user's initial activity time
    newuser->mychannels = NULL;		      // initially not on any channels
    add_chtou(newuser, "Common");             // becomes active on Common
    // install in list of users
    unode *newnode = (unode *)malloc(sizeof(unode)); // new user list node
    newnode->u = newuser;  // data is pointer to our new user struct
    newnode->next = uhead; // new node points at whatever head WAS pointing at
    newnode->prev = NULL; // pointing backward at nothing
    uhead->prev = newnode; //old head points back at new node
    uhead = newnode;       // finally, update head to be our new node
    return newnode;			      // finally, return pointer to newly created user
}

cnode *create_channel(char *cname)	  // creates (if needed) a new channel and installs in list
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
    newnode->prev = NULL;
    chead->prev = newnode;
    chead = newnode;
    return newnode; // finally, return pointer to new channel
}

unode *find_user(char *uname, unode *head) // searches list for user with given name and returns pointer to LIST NODE
{
    unode *nextnode = head; // start at the head
    while (nextnode != NULL) { // while we've still got list to search...
        if (strcmp((nextnode->u)->username, uname) == 0) { // if we've got a match
	    return nextnode; // return pointer to this ulist node!
        }
        nextnode = nextnode->next; // move down the list
    }
    return NULL;
}

cnode *find_channel(char *cname, cnode *head)  // searches list for channel of given name and returns pointer to CHANNEL NODE
{
    cnode *nextnode = head; // start at the head
    while (nextnode != NULL) { // while we've still got list to search...
        if (strcmp((nextnode->c)->channelname, cname) == 0) { // if we've got a match
	    return nextnode; // return pointer to this channel node
        }
        nextnode = nextnode->next; // move down the list
    }
    return NULL;
}

// consider writing an install_user and install_channel function that takes a user * and a list HEAD...
void delete_user(char *uname)    	  i       // removes user from all channels, frees user data, deletes list node
{
    // go through user's channel's and remove user from each channel (decrement channel counts)
    unode *listnode;
    if ((listnode = find_user(uname, uhead)) == NULL) { // get a handle on the list node
	fprintf(stdout, "User %s doesn't exist.\n", uname);
    }
    user *user_p = listnode->u;
    channel *ch_p;
    cnode *next_channel = user_p->mychannels; // start at the head
    while (next_channel != NULL) {
        ch_p = next_channel->c; // get a handle on the channel
        rm_ufromch(ch_p, user_p->username); // remove user from channel
        ch_p->count--;                 // decrement count
        if (ch_p->count == 0)          // check if last user
	    delete_channel(ch_p);      // if so, delete the channel
        free(next_channel); 		// FREE this linked list node
        next_channel = next_channel->next; // move to the next one (if there is one)
    }
    // now that all channels have been left (and nodes freed), free the user's struct
    free(user_p);
    // update the linkages in the list
    (listnode->prev)->next = listnode->next;
    (listnode->next)->prev = listnode->prev;
    // finally, free the listnode
    free(listnode);
    
}

void delete_channel(char *cname)	          // frees channel data, deletes list node
{
    // channel SHOULD have no more user's in list
    cnode *listnode = find_channel(cname);
    if (listnode == NULL)
	fprintf(stdout, "Channel %s doesn't exist.\n", cname);
    // free the channel struct
    free(listnode->c);
    // update linkages in the list:
    (listnode->prev)->next = listnode->next;
    (listnode->next)->prev = listnode->prev;
    // finally, free the node in the clist   
    free(listnode);
}

int add_utoch(char *uname, char *cname)  // adds user to specified channel's list of users
{
    unode *u_node = find_user(uname);
    if (u_node == NULL) {
        fprintf(stderr, "Can't add user \"%s\" to channel \"%s\", they aren't logged in.\n", uname, cname);
	return -1;
    }
    user *user_p = u_node->u;
    cnode *c_node = find_channel(cname);
    if (c_node == NULL) {
        fprintf(stderr, "Can't add user \"%s\" to channel \"%s\", channel doesn't exist.\n", uname, cname);
	return -1;
    }
    channel *ch_p = c_node->c;
    if (find_user(uname, ch_p->myusers) != NULL) {
	fprintf(stderr, "User \"%s\" already on channel \"%s\".\n", uname, cname);
	return -1;
    }
    // if we've made it here, we can go ahead and install user
    unode *newlistnode = (unode *)malloc(sizeof(unode));
    newlistnode->u = user_p;           // populate data load
    newlistnode->next = ch_p->myusers; // new node points at whatever head was pointing at
    newlistnode->prev = NULL; // a new insertion always points back at nothing
    if (ch_p->myusers != NULL) // only if the head wasn't NULL do we have a prev to update
	(ch_p->myusers)->prev = newlistnode; 
    ch_p->myusers = newlistnode; // finally, update the pos of head
    return 0; // success! added user to a channel
}

int add_chtou(char *cname, char *uname)  // adds channel to specified user's list of channels
{
    // check if the channel exists
    cnode *c_node = find_channel(cname);
    if (c_node == NULL) {
	fprintf(stderr, "Can't add channel \"%s\" to user \"%s\", channel doesn't exist.\n", cname, uname);
	return -1;
    }
    channel *ch_p = c_node->c;
    // check if the user exists
    unode *u_node = find_user(uname);
    if (u_node == NULL) {
	fprintf(stderr, "Can't add channel \"%s\" to user \"%s\", user doesn't exist.\n", cname, uname);
	return -1;
    }
    // get a handle on the user
    user *user_p = u_node->u;
    // check if the user is ALREADY subscribed to the channel
    if (find_channel(cname, user_p->mychannels) != NULL) {
	fprintf(stderr, "Channel \"%s\" already in \"%s\"'s list.\n", cname, uname);
	return -1;
    }
    // if not, go ahead and add channel to user's list of channels
    cnode *newlistnode = (cnode *)malloc(sizeof(cnode));
    newlistnode->c = ch_p;           // populate data load
    newlistnode->next = user_p->mychannels; // new node points at whatever head was pointing at
    newlistnode->prev = NULL; // a new insertion always points back at nothing
    if (user_p->mychannels != NULL) // only if the head wasn't NULL do we have a prev to update
	(user_p->mychannels)->prev = newlistnode; 
    user_p->mychannels = newlistnode; // finally, update the pos of head
    return 0; // success! added user to a channel
    
}

int rm_ufromch(char *uname, char *cname) // removes user from specified channel's list of users
{
    // attempt to find user in channel's list
    // if found, remove user node from channel's list
    // decrement count of users on channel
 	// SHOUDL THIS BE THE ONLY PLACE THIS HAPPENS??  i.e. should delete user call this function?
}

int rm_chfromu(char *cname, char *uname) // removes channel from specified user's list of channels
{
	// attempt to find channel in user's list
	// if found, remove channel from user

}

