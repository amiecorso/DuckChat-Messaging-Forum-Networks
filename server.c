/* DuckChat Server program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017

The server does not need to directly interact with the user in any way. However, it is strongly recommended that the server outputs debugging messages to the console. For example, each time the server receives a message from a client, it can output a line describing the contents of the message and who it is from using the following format: [channel][user][message] where message denotes a command and its parameters (if any).

Channel creation and deletion at server are handled implicitly. Whenever a channel has no users, it is deleted. Whenever a user joins a channel that did not exist, it is created.
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

#define UNUSED __attribute__((unused))
#define BUFSIZE 512 

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
unode *create_user(char *uname, struct sockaddr_in *addr);		  // creates (if needed) a new user and installs in list
cnode *create_channel(char *cname);	  // creates (if needed) a new channel and installs in list
unode *find_user(char *uname, unode *head);	  // searches list for user with given name and returns pointer to user
cnode *find_channel(char *cname, cnode *head); // searches list for channel of given name and returns pointer to channel
void delete_user(char *uname);    	         // removes user from all channels, frees user data, deletes list node
void delete_channel(char *cname);	          // frees channel data, deletes list node
int add_chtou(char *cname, char *uname);  // adds channel to specified user's list of channels
int add_utoch(char *uname, char *cname);  // adds user to specified channel's list of users
int rm_chfromu(char *cname, char *uname); // removes channel from specified user's list of channels
int rm_ufromch(char *uname, char *cname); // removes user from specified channel's list of users
unode *getuserfromaddr(struct sockaddr_in *addr); // returns NULL if no such user by address, unode * otherwise

struct user_data {
    char username[USERNAME_MAX];
    struct sockaddr_in u_addr;  // keeps track of user's network address
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
int channelcount = 0; //running count of how many channels exist.
char HOST_NAME[64]; // is this buffer size ok??
int PORT; 

/*========= MAIN ===============*/
int
main(int argc, char **argv) {
    struct sockaddr_in serv_addr;
    int sockfd;
    // Parse command-line arguments
    if (argc < 3) {
        perror("Server: missing arguments.");
        return 0;
    }
    // TODO: ADD ARG ERROR CHECKING
	// error checking for bad returns from gethostbyname()
	// do we need to use gethostbyname??
    strcpy(HOST_NAME, argv[1]);
    PORT = atoi(argv[2]);
    if (( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("server: can't open socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
    serv_addr.sin_port = htons(PORT); //host to network short

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("Server: can't bind local address");
    create_channel("Common");    // default channel
    // for receiving messages
    unsigned char buffer[BUFSIZE]; // for incoming datagrams
    int bytecount;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct request *raw_req = (struct request *)malloc(BUFSIZE);

    while(1) {
        /* now loop, receiving data and printing what we received */
	printf("waiting on port %d\n", PORT);
	bytecount = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
	printf("received %d bytes\n", bytecount);
	if (bytecount > 0) {
		buffer[bytecount] = '\0';
		printf("received message: \"%s\"\n", buffer);
	}
	// Acquire and examine the data from the request
	memcpy(raw_req, buffer, BUFSIZE);
	switch (raw_req->req_type) {
	    case 0: {
		printf("It's a login msg!\n");
		struct request_login *login = (struct request_login *)raw_req;
		printf("username = %s\n", login->req_username);
		if (getuserfromaddr(&client_addr) == NULL) {
		    printf("Logging in user %s\n", login->req_username);
		    create_user(login->req_username, &client_addr);
                    add_utoch(login->req_username, "Common");             // becomes active on Common
		}
		break;
	    }
	    case 1: {
		printf("It's a logout msg\n");
		struct request_logout *logout = (struct request_logout *)raw_req;
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) {
		    printf("Can't log out user because they weren't logged in.\n");
		    break;
		}
		delete_user((unode_p->u)->username);
		break;
	    }
	    case 2: {
		printf("It's a join msg\n");
		struct request_join *join = (struct request_join *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		// see if the channel already exists
		cnode *ch_p = find_channel(join->req_channel, chead);
		if (ch_p == NULL) {     // if the channel doesn't exist yet
		    create_channel(join->req_channel); // create it first
		}
		// add the user to the channel and the channel to the user
		user *user_p = unode_p->u;
		add_utoch(user_p->username, join->req_channel);
		add_chtou(join->req_channel, user_p->username);
		printf("User %s joined channel %s\n", user_p->username, join->req_channel);	
		break;
	    }
	    case 3: {
		printf("It's a leave msg\n");
		struct request_leave *leave = (struct request_leave *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		// if so, try to remove the user from the channel
		user *user_p = unode_p->u;
		// try to remove user from channel
		rm_ufromch(user_p->username, leave->req_channel);
		rm_chfromu(leave->req_channel, user_p->username);
		printf("%s left channel %s\n", user_p->username, leave->req_channel);
		break;
	    }
	    case 4: {
		printf("It's a say msg\n");
		struct request_say *say = (struct request_say *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		user *user_p = unode_p->u;
		struct text_say *saymsg = (struct text_say *)malloc(sizeof(struct text_say));
		strcpy(saymsg->txt_channel, say->req_channel);
		saymsg->txt_type = 0;
		strcpy(saymsg->txt_username, user_p->username);
		strcpy(saymsg->txt_text, say->req_text);
		cnode *cnode_p = find_channel(say->req_channel, chead); // handle on channel node
		channel *ch_p = cnode_p->c; // handle on actual channel		
		unode *nextnode = ch_p->myusers; // start at the head of the channel's list
		user *user_on_channel;
                while (nextnode != NULL) {
		    user_on_channel = nextnode->u; // get a handle on this user
		    printf("sending msg to user: %s\n", user_on_channel->username);
		    // send the say msg to their address
	            sendto(sockfd, saymsg, sizeof(struct text_say), 0, (const struct sockaddr *)&(user_on_channel->u_addr), addr_len);
		    nextnode = nextnode->next;		    
		}
		free(saymsg);
		break;
	    }
	    case 5: {
		printf("It's a list msg\n");
		struct request_list *list = (struct request_list *)raw_req;
		// see if the user exists (if not, ignore)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		user *user_p = unode_p->u;
		struct text_list *listmsg;
		memset(&listmsg, 0, sizeof(struct text_list));
		listmsg.txt_type = 1;
		listmsg.txt_nchannels = 1; // NEED TO FILL THIS OUT
		// send back to user
		break;
	    }
	    case 6: {
		printf("It's a who msg\n");
		struct request_who *who = (struct request_who *)raw_req;
		// see if the user exists (if not, ignore)
		// find the channel the user specified (if it exists)
		// package up a list of all the users on the channel
		// send back to user
		break;
	    }
	    case 7: {
		printf("It's a keepalive msg\n");
		struct request_keep_alive *keepalive = (struct request_keep_alive *)raw_req;
		// see if user exists (if not, ignore)
		// get a new timeofday
		// find and update the user's timeofday stamp
		break;
	    }
	} // end switch

	sendto(sockfd, buffer, strlen((const char *)buffer), 0, (struct sockaddr *)&client_addr, addr_len);
// do we ever need to close the socket??
// close(sockfd); ??
    } // end while
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
unode *create_user(char *uname, struct sockaddr_in *addr)		  // creates (if needed) a new user and installs in list
{
    user *newuser = (user *)malloc(sizeof(user));
    if (newuser == NULL)
	fprintf(stderr, "Error malloc'ing user\n");
    strcpy(newuser->username, uname);         // add the username
    memcpy(&(newuser->u_addr), addr, sizeof(struct sockaddr_in));
    gettimeofday(&(newuser->last_active), NULL); // user's initial activity time
    newuser->mychannels = NULL;		      // initially not on any channels
    // install in list of users
    unode *newnode = (unode *)malloc(sizeof(unode)); // new user list node
    newnode->u = newuser;  // data is pointer to our new user struct
    newnode->next = uhead; // new node points at whatever head WAS pointing at
    newnode->prev = NULL; // pointing backward at nothing
    if (uhead != NULL)
        uhead->prev = newnode; //old head points back at new node
    uhead = newnode;       // finally, update head to be our new node

    //add_utoch(uname, "Common");             // becomes active on Common
    return newnode;			      // finally, return pointer to newly created user
}

cnode *create_channel(char *cname)	  // creates (if needed) a new channel and installs in list
{
    cnode *existingchannel = find_channel(cname, chead);
    if (existingchannel != NULL){
	printf("Channel %s already exists\n", cname);
	return Null;
    }
    // otherwise, we can create it
    channelcount++;
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
    printf("Creating channel %s\n", cname);
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
	fprintf(stdout, "Delete_user: User %s doesn't exist.\n", uname);
    }
    user *user_p = u_node->u;
    printf("Logging out user %s\n", user_p->username);
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
    channelcount--; // and decrement channel count
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

// returns NULL if no such user associated with address.  Returns unode * to user list node otherwise.
unode *getuserfromaddr(struct sockaddr_in *addr)
{
    unode *nextnode = uhead; // start at the head
    struct sockaddr_in u_addr;
    while (nextnode != NULL) { // while we've still got list to search...
        u_addr = (nextnode->u)->u_addr; // get a handle on the sockaddr_in for convenience
        if (((addr->sin_addr).s_addr == u_addr.sin_addr.s_addr) && 
		((addr->sin_port) == u_addr.sin_port)) { // if we've got a match
	    return nextnode; // return pointer to this ulist node!
        }
        nextnode = nextnode->next; // move down the list
    }
    return NULL;
}
