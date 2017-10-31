/* DuckChat Server program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017

The server takes two arguments: the host address to bind to and the port number. The host address can be either 'localhost', the IP host name of the machine that the server is running on, or the IP address of an interface connected to the machine. Once the server is running, clients may use the host name and port to connect to the server. Note that if you use 'localhost', you will not be able to connect to the server from another machine, but you also do not have to worry about dropped packets.
The server does not need to directly interact with the user in any way. However, it is strongly recommended that the server outputs debugging messages to the console. For example, each time the server receives a message from a client, it can output a line describing the contents of the message and who it is from using the following format: [channel][user][message] where message denotes a command and its parameters (if any).

Channel creation and deletion at server are handled implicitly. Whenever a channel has no users, it is deleted. Whenever a user joins a channel that did not exist, it is created.

If the server receives a message from a user who is not logged in, the server should silently ignore the message.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "duckchat.h"
#define UNUSED __attribute__((unused))
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
void who(char *uname, char *cname);
void keep_alive(char *uname); //update time stamp	
// helpers:
unode *create_user(char *uname);		  // creates (if needed) a new user and installs in list
cnode *create_channel(char *cname);	  // creates (if needed) a new channel and installs in list
unode *find_user(char *uname, unode *head);	  // searches list for user with given name and returns pointer to user
cnode *find_channel(char *cname, cnode *head); // searches list for channel of given name and returns pointer to channel
void delete_user(char *uname);    	         // removes user from all channels, frees user data, deletes list node
void delete_channel(char *cname);	          // frees channel data, deletes list node
int add_chtou(char *cname, char *uname);  // adds channel to specified user's list of channels
int add_utoch(char *uname, char *cname);  // adds user to specified channel's list of users
int rm_chfromu(char *cname, char *uname); // removes channel from specified user's list of channels
int rm_ufromch(char *uname, char *cname); // removes user from specified channel's list of users

struct user_data {
    char username[USERNAME_MAX];
    int sock_fd;
    struct timeval last_active; // to populate with gettimeofday() and check activity    
    cnode *mychannels; // pointer to linked list of channel pointers
};

struct channel_data {
    char channelname[CHANNEL_MAX];
    int count; // channel gets deleted when this becomes 0;  rm_ufromch decrements
    unode *myusers; // pointer to linked list of user pointers
};

struct ulist_node {
    user *u;
    unode *next;
    unode *prev;
};

struct chlist_node {
    channel *c;
    cnode *next;
    cnode *prev;
};


/* GLOBALS */
unode *uhead = NULL; // head of my linked list of user pointers
cnode *chead = NULL; // head of my linked list of channel pointers


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


void login(UNUSED char *uname)
{
}

void logout(UNUSED char *uname)
{
}

void join(UNUSED char *uname, UNUSED char *cname)
{
}

void leave(UNUSED char *uname, UNUSED char *cname)
{
}

void say(UNUSED char *uname, UNUSED char *cname)
{
}

void list(UNUSED char *uname, UNUSED char *cname)
{
}

void who(UNUSED char *uname, UNUSED char *cname)
{
}

void keep_alive(UNUSED char *uname) //update time stamp
{
}

/* DATA MANIPULATORS */
unode *create_user(char *uname)		  // creates (if needed) a new user and installs in list
{
    user *newuser = (user *)malloc(sizeof(user));
    if (newuser == NULL)
	fprintf(stderr, "Error malloc'ing new user: %s\n", uname);
    strcpy(newuser->username, uname);         // add the username
    gettimeofday(&(newuser->last_active), NULL); // user's initial activity time
    newuser->mychannels = NULL;		      // initially not on any channels
    add_chtou("Common", uname);             // becomes active on Common
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
    if (chead != NULL) 
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
void delete_user(char *uname)    	         // removes user from all channels, frees user data, deletes list node
{
    // go through user's channel's and remove user from each channel (decrement channel counts)
    unode *u_node;
    if ((u_node = find_user(uname, uhead)) == NULL) { // get a handle on the list node
	fprintf(stdout, "User %s doesn't exist.\n", uname);
    }
    user *user_p = u_node->u;
    channel *ch_p;
    cnode *next_channel = user_p->mychannels; // start at the head
    while (next_channel != NULL) {
        ch_p = next_channel->c; // get a handle on the channel
        rm_ufromch(user_p->username, ch_p->channelname); // remove user from channel (this will call delete_channel if it's empty)
        free(next_channel); 		// FREE this linked list node
        next_channel = next_channel->next; // move to the next one (if there is one)
    }
    // now that all channels have been left (and nodes freed), free the user's struct
    free(user_p);
    // update the linkages in the list: depends on case
    if ((u_node->prev == NULL) && (u_node->next == NULL)) { // case, ONLY NODE
	uhead = NULL; // just set the head to null
    }
    else if ((u_node->prev == NULL) && (u_node->next != NULL)) { // case: head, multiple nodes
	uhead = u_node->next; // head points to next down
	//(u_node->next)->prev == NULL; // new head points back to nothing
    } else if ((u_node->prev != NULL) && (u_node->next != NULL)) { //case: middle of list
	(u_node->next)->prev = u_node->prev; // bridge the gap
	(u_node->prev)->next = u_node->next;
    } else if ((u_node->prev != NULL) && (u_node->next == NULL)) { // case: tail, multiple nodes
	(u_node->prev)->next = NULL; // tail points out to nothing
    }
    // in call cases, free the list node
    free(u_node);
}

void delete_channel(char *cname)	          // frees channel data, deletes list node
{
    // channel SHOULD have no more user's in list
    cnode *c_node = find_channel(cname, chead);
    if (c_node == NULL)
	fprintf(stdout, "Channel %s doesn't exist.\n", cname);
    channel *ch_p = c_node->c;
    // free the channel struct
    free(ch_p);
    // update linkages in the list: depends on case 
    if ((c_node->prev == NULL) && (c_node->next == NULL)) { // case, ONLY NODE
	chead = NULL; // just set the head to null
    }
    else if ((c_node->prev == NULL) && (c_node->next != NULL)) { // case: head, multiple nodes
	chead = c_node->next; // head points to next down
	//(c_node->next)->prev == NULL; // new head points back to nothing
    } else if ((c_node->prev != NULL) && (c_node->next != NULL)) { //case: middle of list
	(c_node->next)->prev = c_node->prev; // bridge the gap
	(c_node->prev)->next = c_node->next;
    } else if ((c_node->prev != NULL) && (c_node->next == NULL)) { // case: tail, multiple nodes
	(c_node->prev)->next = NULL; // tail points out to nothing
    }
    // finally, free the node in the clist   
    free(c_node);
}

int add_utoch(char *uname, char *cname)  // adds user to specified channel's list of users
{
    unode *u_node = find_user(uname, uhead);
    if (u_node == NULL) {
        fprintf(stderr, "Can't add user \"%s\" to channel \"%s\", they aren't logged in.\n", uname, cname);
	return -1;
    }
    user *user_p = u_node->u;
    cnode *c_node = find_channel(cname, chead);
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
    cnode *c_node = find_channel(cname, chead);
    if (c_node == NULL) {
	fprintf(stderr, "Can't add channel \"%s\" to user \"%s\", channel doesn't exist.\n", cname, uname);
	return -1;
    }
    channel *ch_p = c_node->c;
    // check if the user exists
    unode *u_node = find_user(uname, uhead);
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
    cnode *c_node = find_channel(cname, chead);
    if (c_node == NULL) {
	fprintf(stderr, "Can't find channel \"%s\" to remove user \"%s\".\n", cname, uname);
	return -1;
    }
    channel *ch_p = c_node->c;
    // check if the user exists in that channel's list
    unode *u_node = find_user(uname, ch_p->myusers);
    if (u_node == NULL) {
	fprintf(stderr, "User \"%s\" not in channel \"%s\"'s list.\n", uname, cname);
	return -1;
    }
    // if found, remove user node from channel's list
    if ((u_node->prev == NULL) && (u_node->next == NULL)) { // case, ONLY NODE
	ch_p->myusers = NULL; // just set the head to null
    }
    else if ((u_node->prev == NULL) && (u_node->next != NULL)) { // case: head, multiple nodes
	ch_p->myusers = u_node->next; // head points to next down
	//(u_node->next)->prev == NULL; // new head points back to nothing
    } else if ((u_node->prev != NULL) && (u_node->next != NULL)) { //case: middle of list
	(u_node->next)->prev = u_node->prev; // bridge the gap
	(u_node->prev)->next = u_node->next;
    } else if ((u_node->prev != NULL) && (u_node->next == NULL)) { // case: tail, multiple nodes
	(u_node->prev)->next = NULL; // tail points out to nothing
    }
    // in call cases, free the list node
    free(u_node);
    ch_p->count -= 1;
    if (ch_p->count == 0)
	delete_channel(cname);
    return 0;
}

int rm_chfromu(char *cname, char *uname) // removes channel from specified user's list of channels
{
    // attempt to find user 
    unode *u_node = find_user(uname, uhead);
    if (u_node == NULL) {
	fprintf(stderr, "rm_chfromu: User \"%s\" doesn't seem to exist.\n", uname);
	return -1;
    }
    user *user_p = u_node->u; // handle on user
    // attempt to find channel IN USER'S LIST
    cnode *c_node = find_channel(cname, user_p->mychannels);
    if (c_node == NULL) {
	fprintf(stderr, "rm_chfromu: can't find channel \"%s\" to remove FROM user \"%s\".\n", cname, uname);
	return -1;
    }
    // if found, remove channel node from user's list
    if ((c_node->prev == NULL) && (c_node->next == NULL)) { // case, ONLY NODE
	user_p->mychannels = NULL; // just set the head to null
    }
    else if ((c_node->prev == NULL) && (c_node->next != NULL)) { // case: head, multiple nodes
	user_p->mychannels = c_node->next; // head points to next down
	//(c_node->next)->prev == NULL; // new head points back to nothing
    } else if ((c_node->prev != NULL) && (c_node->next != NULL)) { //case: middle of list
	(c_node->next)->prev = c_node->prev; // bridge the gap
	(c_node->prev)->next = c_node->next;
    } else if ((c_node->prev != NULL) && (c_node->next == NULL)) { // case: tail, multiple nodes
	(c_node->prev)->next = NULL; // tail points out to nothing
    }
    // in call cases, free the list node
    free(c_node);
    return 0;
}

