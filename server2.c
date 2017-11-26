/* DuckChat Server program
Author: Amie Corso
532 Intro to Networks - Program 2 
Fall 2017

TODO: 
clean the line numbers out of printdebug
clean up extra prints
test test test 
- many args
- bad args

creation of common
	- does a JOIN msg need to be sent to neighbors?  or can we all just sign each other up?
	- I guess it's my server so it's my call.
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
#include <signal.h>
#include "duckchat.h"
#define UNUSED __attribute__((unused))
#define BUFSIZE 512 
#define MAXNEIGHBORS 20
#define ID_BUFSIZE 20
/* Forward Declarations */
typedef struct user_data user;
typedef struct channel_data channel;
typedef struct server_data server;
typedef struct ulist_node unode;
typedef struct chlist_node cnode;
typedef struct slist_node snode;
unode *create_user(char *uname, struct sockaddr_in *addr);  // creates (if needed) a new user and installs in list
cnode *create_channel(char *cname);	  // creates (if needed) a new channel and installs in list
unode *find_user(char *uname, unode *head);	  // searches list for user with given name and returns pointer to user
cnode *find_channel(char *cname, cnode *head); // searches list for channel of given name and returns pointer to channel
void delete_user(char *uname);    	         // removes user from all channels, frees user data, deletes list node
void delete_channel(char *cname);	          // frees channel data, deletes list node
int add_chtou(char *cname, char *uname);  // adds channel to specified user's list of channels
int add_utoch(char *uname, char *cname);  // adds user to specified channel's list of users
int rm_chfromu(char *cname, char *uname); // removes channel from specified user's list of channels
int rm_ufromch(char *uname, char *cname); // removes user from specified channel's list of users
void add_stoch(server *s, cnode *ch);	
void rm_sfromch(server *s, cnode *ch);
server *find_server(struct sockaddr_in *addr);
unode *getuserfromaddr(struct sockaddr_in *addr); // returns NULL if no such user by address, unode * otherwise
void update_timestamp(struct sockaddr_in *addr); // updates timestamp for user with given address (if they exist)
void force_logout();    // forcibly logs out users who haven't been active for at least two minutes
void set_timer(int interval);
void print_debug_msg(struct sockaddr_in *recv_addr, int msg_type, char *send_or_recv, char *username, char *channel, char *message, int line);
int checkID(long long ID); // returns 1 if ID is found, 0 if not
void generateID(long long *ID); // generates random 64-bit ID for tagging say messages
// STRUCTS
struct user_data {
    char username[USERNAME_MAX];
    struct sockaddr_in u_addr;  // keeps track of user's network address
    struct timeval last_active; // to populate with gettimeofday() and check activity    
    cnode *mychannels; // pointer to linked list of channel pointers
};
struct channel_data {
    char channelname[CHANNEL_MAX];
    int count; //  rm_ufromch decrements
    int servercount; // gets deleted when this AND count (usercount) become 0
    unode *myusers; // pointer to linked list of user pointers
    snode *sub_servers; // linked list of subscribed servers to this channel.
};
struct server_data {
    struct sockaddr_in remote_addr;
    struct timeval last_join; // to populate with gettimeofday() and check activity    
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
struct slist_node {
    server *s;
    snode *next;
    snode *prev;
};
/* GLOBALS */
server servers[MAXNEIGHBORS]; // static array of server structs
int neighborcount = 0; // keep track of how many servers are attached to us
long long recentIDs[ID_BUFSIZE]; // keeps track of recently seen unique-IDs for say msgs
int nextin = 0; 	// index into recentIDs, bounded buffer with % arithmetic
unode *uhead = NULL; // head of my linked list of user pointers
cnode *chead = NULL; // head of my linked list of channel pointers
int channelcount = 0; //running count of how many channels exist.
char HOST_NAME[64]; // store host name from argv
struct sockaddr_in serv_addr;
socklen_t addr_len;
char MY_IP[INET_ADDRSTRLEN]; // printable IP address
int PORT; 
int sockfd;
struct itimerval timer;
int toggle = 1; // 1 indicates only one minute has gone by, 2 indicates two minutes

/*========= MAIN ===============*/
int
main(int argc, char **argv) {
    // zero recent IDs  =====================================================
    int i, j;
    for (i = 0; i < ID_BUFSIZE; i++) {
	recentIDs[i] = 0;
    }
    // Parse command-line arguments ====================================================
    if ((argc < 3) || ((argc % 2) != 1)) { // if we don't have enough args or an odd number of args
        perror("Useage: ./server <hostname> <port> [<hostname> <port> ...]");
        return 0;
    }
    if (argc > (MAXNEIGHBORS*2 + 3)) {
	perror("Too many connections supplied\n");
	return 0;
    }
    strcpy(HOST_NAME, argv[1]);
    PORT = atoi(argv[2]);
    inet_ntop(AF_INET, (void *)&serv_addr.sin_addr, MY_IP, INET_ADDRSTRLEN); // populate printable IP
    j = 0; // index into servers array
    struct hostent *host_p;
    in_port_t remote_port;
    for (i = 3; i < argc; i += 2) {
	host_p = gethostbyname(argv[i]); // DNS lookup for IP from hostname
        if (!host_p) {
            fprintf(stderr, "Could not obtain address for %s\n", argv[i]);
            return 0;
        } // and store it in the .sin_addr...
	servers[j].remote_addr.sin_family = AF_INET;
	memcpy((void *)&(servers[j].remote_addr.sin_addr), host_p->h_addr_list[0], host_p->h_length);
	remote_port = atoi(argv[i + 1]);
	remote_port = htons(remote_port);
	if (remote_port) {
	    servers[j].remote_addr.sin_port = remote_port;
	}
	else {
	    fprintf(stderr, "Invalid port number: %s\n", argv[i + 1]);
	    return 0;
	}
	memset(&(servers[i].last_join), 0, sizeof(struct timeval));
	j++; 
	neighborcount += 1; // keep track of how many connected servers
    } // END PARSING ======================================================================
    // DEBUG
    printf("neighborcount = %d\n", neighborcount);	
    char print_address[INET_ADDRSTRLEN];
    for (i = 0; i < neighborcount; i++) {
	inet_ntop(AF_INET, (void *)&servers[i].remote_addr.sin_addr, print_address, INET_ADDRSTRLEN);
	printf("neighbor %d: address: %s  port: %d\n", i, print_address, ntohs(servers[i].remote_addr.sin_port));
    }
    // end debug

    if (( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        perror("server: can't open socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
    serv_addr.sin_port = htons(PORT); //host to network short

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("Server: can't bind local address");
    
    // create default channel and send join messages to neighbors
    create_channel("Common");
    cnode *common_p = find_channel("Common", chead); 
    for (i = 0; i < neighborcount; i++) { // send each neighbor a join and subscribe them to channel
	// not going to send join messages, just going to add the neighbors
	//sendto(sockfd, s2sjoin, sizeof(struct s2s_join), 0, (const struct sockaddr *)&servers[i].remote_addr, addr_len);
	//print_debug_msg(&servers[i].remote_addr, 8, "sent", NULL, s2sjoin->req_channel, NULL);
	add_stoch(&servers[i], common_p);
    }
    // for receiving messages
    unsigned char buffer[BUFSIZE]; // for incoming datagrams
    int bytecount;
    struct sockaddr_in client_addr;
    addr_len = sizeof(client_addr);
    struct request *raw_req = (struct request *)malloc(BUFSIZE);

    signal(SIGALRM, force_logout); // register signal handler
    set_timer(60);  // goes off every 60 seconds
    while(1) {
        /* now loop, receiving data and printing what we received */
	bytecount = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
	if (bytecount > 0) {
		buffer[bytecount] = '\0';
	}
	// Acquire and examine the data from the request
	memcpy(raw_req, buffer, BUFSIZE);
	switch (raw_req->req_type) {
	    case 0: {
		struct request_login *login = (struct request_login *)raw_req;
		if ((getuserfromaddr(&client_addr) == NULL) && (find_user(login->req_username, uhead) == NULL)) {
		    print_debug_msg(&client_addr, 0, "recv", login->req_username, NULL, NULL, 195);
		    create_user(login->req_username, &client_addr);
		}
		else {
		    printf("User %s already logged in.\n", login->req_username);
		}
		break;
	    }
	    case 1: {
		unode *unode_p = getuserfromaddr(&client_addr);
		print_debug_msg(&client_addr, 1, "recv", (unode_p->u)->username, NULL, NULL, 205);
		if (unode_p == NULL) {
		    printf("Can't log out user because they weren't logged in.\n");
		    break;
		}
		
		delete_user((unode_p->u)->username);
		break;
	    }
	    case 2: {
		struct request_join *join = (struct request_join *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		print_debug_msg(&client_addr, 2, "recv", (unode_p->u)->username, join->req_channel, NULL, 222);
		// see if the channel already exists
		cnode *ch_p = find_channel(join->req_channel, chead);
		if (ch_p == NULL) {     // if the channel doesn't exist yet
		    cnode *cnode_p = create_channel(join->req_channel); // create it first
		    printf("created channel %s\n", join->req_channel);
		    printf("cnode_p = %p\n", cnode_p);
		    // and then send a join request to all server neighbors
		    struct s2s_join *s2sjoin = (struct s2s_join *)malloc(sizeof(struct s2s_join));
		    s2sjoin->req_type = 8;
		    strcpy(s2sjoin->req_channel, join->req_channel);
		    printf("created s2sjoin msg\n");
		    for (i = 0; i < neighborcount; i++) { // send each neighbor a join and subscribe them to channel
	                sendto(sockfd, s2sjoin, sizeof(struct s2s_join), 0, (const struct sockaddr *)&(servers[i].remote_addr), addr_len);
		        print_debug_msg(&(servers[i].remote_addr), 8, "sent", NULL, s2sjoin->req_channel, NULL, 234);
			add_stoch(&servers[i], cnode_p);
		    }
		}
		// add the user to the channel and the channel to the user
		user *user_p = unode_p->u;
		add_utoch(user_p->username, join->req_channel);
		add_chtou(join->req_channel, user_p->username);
		printf("User %s joined channel %s\n", user_p->username, join->req_channel);	
		break;
	    }
	    case 3: {
		struct request_leave *leave = (struct request_leave *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		// if so, try to remove the user from the channel
		user *user_p = unode_p->u;
		print_debug_msg(&client_addr, 3, "recv", (unode_p->u)->username, leave->req_channel, NULL, 253);
		// try to remove user from channel
		rm_ufromch(user_p->username, leave->req_channel);
		rm_chfromu(leave->req_channel, user_p->username);
		printf("%s left channel %s\n", user_p->username, leave->req_channel);
		break;
	    }
	    case 4: {
		struct request_say *say = (struct request_say *)raw_req;
		// see if the user is already logged in (if not, forget the message)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		cnode *cnode_p = find_channel(say->req_channel, chead); // handle on channel node
		if (cnode_p == NULL)
			break;
		user *user_p = unode_p->u;
		print_debug_msg(&client_addr, 4, "recv", (unode_p->u)->username, say->req_channel, say->req_text, 270);
		struct text_say *saymsg = (struct text_say *)malloc(sizeof(struct text_say));
		strcpy(saymsg->txt_channel, say->req_channel);
		saymsg->txt_type = 0;
		strcpy(saymsg->txt_username, user_p->username);
		strcpy(saymsg->txt_text, say->req_text);
		channel *ch_p = cnode_p->c; // handle on actual channel		
		unode *nextnode = ch_p->myusers; // start at the head of the channel's list
		user *user_on_channel;
                while (nextnode != NULL) {
		    user_on_channel = nextnode->u; // get a handle on this user
		    printf("sending msg to user: %s\n", user_on_channel->username);
		    // send the say msg to their address
	            sendto(sockfd, saymsg, sizeof(struct text_say), 0, (const struct sockaddr *)&(user_on_channel->u_addr), addr_len);
		    print_debug_msg(&(user_on_channel->u_addr), 11, "send", user_on_channel->username, ch_p->channelname, saymsg->txt_text, 284);
		    nextnode = nextnode->next;		    
		}
		// SEND TO SERVERS
		struct s2s_say *s2ssay = (struct s2s_say *)malloc(sizeof(struct s2s_say));
		s2ssay->req_type = 10;
		strcpy(s2ssay->req_username, user_p->username);
		strcpy(s2ssay->req_channel, say->req_channel);
		strcpy(s2ssay->req_text, say->req_text);
		generateID(&(s2ssay->unique_id)); // puts the ID in this field
		recentIDs[nextin] = s2ssay->unique_id; // store the ID
		nextin = (nextin + 1) % ID_BUFSIZE; // update the index
		snode *nextserv = ch_p->sub_servers;
		server *serv;
		while (nextserv != NULL) {
		    serv = nextserv->s;
	            sendto(sockfd, s2ssay, sizeof(struct s2s_say), 0, (const struct sockaddr *)&(serv->remote_addr), addr_len);
		    print_debug_msg(&(serv->remote_addr), 10, "send", s2ssay->req_username, s2ssay->req_channel, s2ssay->req_text, 301);
		    nextserv = nextserv->next;
		}
		free(saymsg);
		free(s2ssay);
		break;
	    }
	    case 5: {
		// see if the user exists (if not, ignore)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		print_debug_msg(&client_addr, 5, "recv", NULL, NULL, NULL, 313);
		struct text_list *listmsg = (struct text_list *)malloc(sizeof(struct text_list) + channelcount * sizeof(struct channel_info));
		memset(listmsg, 0, sizeof(struct text_list) + (channelcount * sizeof(struct channel_info)));
		listmsg->txt_type = 1;
		listmsg->txt_nchannels = channelcount; // NEED TO FILL THIS OUT
		int i = 0;
		cnode *nextnode = chead;
		channel *ch_p;
		while (nextnode != NULL) {
		    ch_p = nextnode->c;   // handle on the actual channel
		    strcpy(listmsg->txt_channels[i++].ch_channel, ch_p->channelname);
		    nextnode = nextnode->next;
		}
		int packetsize = sizeof(struct text_list) + channelcount * sizeof(struct channel_info);
		// send back to client
	        sendto(sockfd, listmsg, packetsize, 0, (const struct sockaddr *)&client_addr, addr_len);
		print_debug_msg(&client_addr, 12, "send", NULL, NULL, NULL, 329);
	        free(listmsg);	
		break;
	    }
	    case 6: {
		struct request_who *who = (struct request_who *)raw_req;
		// see if the user exists (if not, ignore)
		unode *unode_p = getuserfromaddr(&client_addr);
		if (unode_p == NULL) // if user doesn't exist, break
			break;
		print_debug_msg(&client_addr, 6, "recv", NULL, who->req_channel, NULL, 339);
		cnode *cnode_p = find_channel(who->req_channel, chead);
		if (cnode_p == NULL) {
		    printf("Bad who request: channel %s doesn't exist\n", who->req_channel);
		    break;
		}
		channel *ch_p = cnode_p->c;
		struct text_who *whomsg = (struct text_who *)malloc(sizeof(struct text_who) + ch_p->count * sizeof(struct user_info));
		memset(whomsg, 0, sizeof(struct text_who) + (ch_p->count * sizeof(struct user_info)));
		whomsg->txt_type = 2;
		whomsg->txt_nusernames = ch_p->count; // NEED TO FILL THIS OUT
		int i = 0;
		unode *nextnode = ch_p->myusers; // going through CHANNEL's users
		user *user_p;
		while (nextnode != NULL) {
		    user_p = nextnode->u;   // handle on the actual channel
		    strcpy(whomsg->txt_users[i++].us_username, user_p->username);
		    nextnode = nextnode->next;
		}
		int packetsize = sizeof(struct text_who) + ch_p->count * sizeof(struct user_info);
		// send back to client
	        sendto(sockfd, whomsg, packetsize, 0, (const struct sockaddr *)&client_addr, addr_len);
		print_debug_msg(&client_addr, 13, "send", NULL, who->req_channel, NULL, 361);
	        free(whomsg);	
		break;
	    }
	    case 8: { // S2S join
		struct s2s_join *s2sjoin = (struct s2s_join *)raw_req;
		// do we need to first check if this server is legit??
		print_debug_msg(&client_addr, 8, "recv", NULL, s2sjoin->req_channel, NULL, 368);
		server *sender = find_server(&client_addr);

		if (sender == NULL) {
		    printf("Unrecognized server sent join message\n");
		    break;
		}
	        // update server's timestamp! (got join)	
		gettimeofday(&(sender->last_join), NULL);
		cnode *cnode_p = find_channel(s2sjoin->req_channel, chead); // search for the channel
		if (cnode_p != NULL) { // then we are already subscribed to this channel
		    add_stoch(sender, cnode_p); // function automatically prevents duplicates
		    break;		// do nothing
		}
		else {
		    cnode_p = create_channel(s2sjoin->req_channel); // create the channel
                    add_stoch(sender, cnode_p); // subscribe the sender to this channel
		    for (i = 0; i < neighborcount; i++) { // send each neighbor a join and subscribe them to channel
			int equal_addr = client_addr.sin_addr.s_addr == servers[i].remote_addr.sin_addr.s_addr;
		        int equal_ports = client_addr.sin_port == servers[i].remote_addr.sin_port;	
			if (!equal_addr || !equal_ports) {
	                    sendto(sockfd, s2sjoin, sizeof(struct s2s_join), 0, (const struct sockaddr *)&servers[i].remote_addr, addr_len);
		            print_debug_msg(&servers[i].remote_addr, 8, "sent", NULL, s2sjoin->req_channel, NULL, 388);
			    add_stoch(&servers[i], cnode_p);
			}
		    }
		}
		break;
	    }
	    case 9: { // S2S leave
		struct s2s_leave *s2sleave = (struct s2s_leave *)raw_req;
		server *sender = find_server(&client_addr);
		print_debug_msg(&client_addr, 9, "recv", NULL, s2sleave->req_channel, NULL, 404);
		if (sender == NULL) {
		    printf("Unrecognized server sent leave message\n");
		    break;
		}
		cnode *cnode_p = find_channel(s2sleave->req_channel, chead); // search for the channel
		if (cnode_p == NULL) { // then we do not actually have this channel
		    printf("case 9: channel was null\n");
		    break;		// do nothing
		}
		rm_sfromch(sender, cnode_p); // remove sender from list
		break;
	    }
	    case 10: { // S2S say
		struct s2s_say *s2ssay = (struct s2s_say *)raw_req;
		server *sender = find_server(&client_addr);
		print_debug_msg(&client_addr, 10, "recv", s2ssay->req_username, s2ssay->req_channel, s2ssay->req_text, 413);
		if (sender == NULL) { // do we recognize this server??
		    printf("Unrecognized server sent say message\n");
		    break;
		}
		cnode *cnode_p = find_channel(s2ssay->req_channel, chead); // search for the channel
		if (cnode_p == NULL) { // then we do not actually have this channel
		    printf("Unrecognized channel: %s\n", s2ssay->req_channel);
		    break;		// do nothing
		}
	       	

		if (checkID(s2ssay->unique_id)) { // then it's a DUPLICATE!
		    printf("duplicate message detected\n");
		    struct s2s_leave *s2sleave = (struct s2s_leave *)malloc(sizeof(struct s2s_leave));
		    s2sleave->req_type = 9;
		    strcpy(s2sleave->req_channel, s2ssay->req_channel);
	            sendto(sockfd, s2sleave, sizeof(struct s2s_leave), 0, (const struct sockaddr *)&client_addr, addr_len);
		    print_debug_msg(&client_addr, 9, "send", NULL, s2sleave->req_channel, NULL, 430);
		    //delete_channel(s2sleave->req_channel); // get rid of channel!
		    free(s2sleave);
		    break; // and we're done
		}
		else {// it is unique
		    recentIDs[nextin] = s2ssay->unique_id; // store the ID
		    nextin = (nextin + 1) % ID_BUFSIZE; // update the index
		    int forwarded = 0;
		    channel *ch_p = cnode_p->c; // handle on the channel
		    // SEND TO CLIENTS
		    struct text_say *saymsg = (struct text_say *)malloc(sizeof(struct text_say));
		    strcpy(saymsg->txt_channel, s2ssay->req_channel);
		    saymsg->txt_type = 0;
		    strcpy(saymsg->txt_username, s2ssay->req_username);
		    strcpy(saymsg->txt_text, s2ssay->req_text);
		    // iterate through channel's users
		    unode *nextnode = ch_p->myusers; // start at the head of the channel's list
		    user *user_on_channel;
                    while (nextnode != NULL) {
		        user_on_channel = nextnode->u; // get a handle on this user
		        printf("sending msg to user: %s\n", user_on_channel->username);
		        // send the say msg to their address
	                sendto(sockfd, saymsg, sizeof(struct text_say), 0, (const struct sockaddr *)&(user_on_channel->u_addr), addr_len);
		        print_debug_msg(&(user_on_channel->u_addr), 11, "send", user_on_channel->username, ch_p->channelname, saymsg->txt_text, 452);
		        nextnode = nextnode->next;		    
			forwarded += 1; // keep track of whether this was forwarded at all
		    }
		    free(saymsg);
		    // SEND TO SERVERS
		    snode *nextserv = ch_p->sub_servers;
		    server *serv;
		    while (nextserv != NULL) {
			serv = nextserv->s;
			int equal_addr = client_addr.sin_addr.s_addr == serv->remote_addr.sin_addr.s_addr;
		        int equal_ports = client_addr.sin_port == serv->remote_addr.sin_port;	
			if (!equal_addr || !equal_ports) {
	                    sendto(sockfd, s2ssay, sizeof(struct s2s_say), 0, (const struct sockaddr *)&(serv->remote_addr), addr_len);
		            print_debug_msg(&(serv->remote_addr), 10, "send", s2ssay->req_username, s2ssay->req_channel, s2ssay->req_text, 466);
			    forwarded += 1;
			}
			nextserv = nextserv->next;
		    }  // end while
		    // if there are NO clients on channel and no OTHER servers on channel.. reply with a leave msg
		    if (forwarded == 0) {
		        struct s2s_leave *s2sleave = (struct s2s_leave *)malloc(sizeof(struct s2s_leave));
			s2sleave->req_type = 9;
		        strcpy(s2sleave->req_channel, s2ssay->req_channel);
	                sendto(sockfd, s2sleave, sizeof(struct s2s_leave), 0, (const struct sockaddr *)&client_addr, addr_len);
		        print_debug_msg(&client_addr, 9, "send", NULL, s2sleave->req_channel, NULL, 487);
			delete_channel(s2sleave->req_channel); // we're a leaf anyway
		        free(s2sleave);
		    }
		} // end else
		break;
	    } // end case 10
	    //default:
	    //	printf("Received SOMETHING\n");
	} // end switch
	update_timestamp(&client_addr); // update timestamp for sender
    } // end while
} // END MAIN
/* ================ HELPER FUNCTIONS ========================================= */
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
    return newnode;			      // finally, return pointer to newly created user
}

cnode *create_channel(char *cname)	  // creates (if needed) a new channel and installs in list
{
    cnode *existingchannel = find_channel(cname, chead);
    if (existingchannel != NULL){
	printf("Channel %s already exists\n", cname);
	return NULL;
    }
    // otherwise, we can create it
    channelcount++;
    channel *newchannel = (channel *)malloc(sizeof(channel));
    if (newchannel == NULL)
	fprintf(stderr, "Error malloc'ing new channel; %s\n", cname);
    strcpy(newchannel->channelname, cname);    
    newchannel->count = 0;
    newchannel->servercount = 0;
    newchannel->myusers = NULL; // initially no users on channel
    newchannel->sub_servers = NULL; //initially no other servers subscribed
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
	return;
    }
    user *user_p = u_node->u;
    printf("Logging out user %s\n", user_p->username);
    channel *ch_p;
    cnode *current_channel = user_p->mychannels; // start at the head
    cnode *next_channel;
    while (current_channel != NULL) {
        ch_p = current_channel->c; // get a handle on the channel
        rm_ufromch(user_p->username, ch_p->channelname); // remove user from channel (this will call delete_channel if it's empty)
        next_channel = current_channel->next; // (store the next one to look at)
        free(current_channel); 		// FREE this linked list node
	current_channel = next_channel; // update with next one to look at
    }
    // now that all channels have been left (and nodes freed), free the user's struct
    free(user_p);
    // update the linkages in the list: depends on case
    if ((u_node->prev == NULL) && (u_node->next == NULL)) { // case, ONLY NODE
	uhead = NULL; // just set the head to null
    }
    else if ((u_node->prev == NULL) && (u_node->next != NULL)) { // case: head, multiple nodes
	//(u_node->next)->prev == NULL; // new head points back to nothing
	uhead = u_node->next; // head points to next down
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
    if (c_node == NULL) {
	fprintf(stdout, "Channel %s doesn't exist.\n", cname);
	return;
    }
    channel *ch_p = c_node->c;
    // free the channel struct
    free(ch_p);
    // update linkages in the list: depends on case 
    if ((c_node->prev == NULL) && (c_node->next == NULL)) { // case, ONLY NODE
	chead = NULL; // just set the head to null
    }
    else if ((c_node->prev == NULL) && (c_node->next != NULL)) { // case: head, multiple nodes
	//(c_node->next)->prev == NULL; // new head points back to nothing
	chead = c_node->next; // head points to next down
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
    ch_p->count++;
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
	//(u_node->next)->prev == NULL; // new head points back to nothing
	ch_p->myusers = u_node->next; // head points to next down
    } else if ((u_node->prev != NULL) && (u_node->next != NULL)) { //case: middle of list
	(u_node->next)->prev = u_node->prev; // bridge the gap
	(u_node->prev)->next = u_node->next;
    } else if ((u_node->prev != NULL) && (u_node->next == NULL)) { // case: tail, multiple nodes
	(u_node->prev)->next = NULL; // tail points out to nothing
    }
    // in call cases, free the list node
    free(u_node);
    ch_p->count -= 1;
    if (((ch_p->count == 0) && (strcmp(ch_p->channelname, "Common") != 0)) && (ch_p->servercount == 0))
	delete_channel(cname); // only if empty of users AND servers to forward
    return 0;
}

int rm_chfromu(char *cname, char *uname) // removes channel from specified user's list of channels
{
    // attempt to find user 
    unode *u_node = find_user(uname, uhead);
    if (u_node == NULL) {
	return -1;
    }
    user *user_p = u_node->u; // handle on user
    // attempt to find channel IN USER'S LIST
    cnode *c_node = find_channel(cname, user_p->mychannels);
    if (c_node == NULL) {
	return -1;
    }
    // if found, remove channel node from user's list
    if ((c_node->prev == NULL) && (c_node->next == NULL)) { // case, ONLY NODE
	user_p->mychannels = NULL; // just set the head to null
    }
    else if ((c_node->prev == NULL) && (c_node->next != NULL)) { // case: head, multiple nodes
	//(c_node->next)->prev == NULL; // new head points back to nothing
	user_p->mychannels = c_node->next; // head points to next down
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

void update_timestamp(struct sockaddr_in *addr)
{
    unode *unode_p = getuserfromaddr(addr);
    if (unode_p == NULL) {
	return;
    }
    user *user_p = unode_p->u;
    gettimeofday(&(user_p->last_active), NULL);
}

void force_logout(UNUSED int sig)
{
    //send join messages! (in all cases)
    cnode *nextnode = chead; // start at the head of our channels
    int i;
    while (nextnode != NULL) { // go through all channels
	channel *ch_p = nextnode->c;
	struct s2s_join *s2sjoin = (struct s2s_join *)malloc(sizeof(struct s2s_join));
	s2sjoin->req_type = 8;
	strcpy(s2sjoin->req_channel, ch_p->channelname);
	for (i = 0; i < neighborcount; i++) { // for each neighbor server
	    sendto(sockfd, s2sjoin, sizeof(struct s2s_join), 0, (const struct sockaddr *)&servers[i].remote_addr, addr_len);
	    print_debug_msg(&servers[i].remote_addr, 8, "(auto) sent", NULL, s2sjoin->req_channel, NULL, 823);
	    add_stoch(&servers[i], nextnode);
	}
	nextnode = nextnode->next;
    }
    printf("timer went off: toggle = %d\n", toggle);
    if (toggle == 1) {
	toggle = 2;// increment toggle
    }	
    else { // toggle == 2
	// check to see who we need to kick off
	// set toggle back to 1
	toggle = 1;
        struct timeval current;
        gettimeofday(&current, NULL); 
        long unsigned elapsed_usec;
	// deal with users
        unode *nextuser = uhead; 
        while (nextuser != NULL) {
            user *user_p = nextuser->u;
            elapsed_usec = (current.tv_sec * 1000000 +  current.tv_usec) - 
                                (user_p->last_active.tv_sec * 1000000 + user_p->last_active.tv_usec);
	    if (elapsed_usec >= 120000000) {
	        delete_user(user_p->username);
	        printf("Forcibly logging out user %s\n", user_p->username);
	    }
	    nextuser = nextuser->next;
        }// end while
	// deal with servers:
	cnode *nextchannel = chead; // we're going to go through every channel
	while (nextchannel != NULL) {
	    channel *ch_p = nextchannel->c;
	    snode *nextserver = ch_p->sub_servers;
	    while (nextserver != NULL) { // and every server on every channel
		server *serv = nextserver->s; // get a handle on the server
                elapsed_usec = (current.tv_sec * 1000000 +  current.tv_usec) - 
                                (serv->last_join.tv_sec * 1000000 + serv->last_join.tv_usec);
	        if (elapsed_usec >= 120000000) {
		    rm_sfromch(serv, nextchannel);
		    print_debug_msg(&(serv->remote_addr), 9, "(auto) recv", NULL, ch_p->channelname, NULL, 857);
		}
		nextserver = nextserver->next;
	    }
	    nextchannel = nextchannel->next;
	} // end while
    }// end else
}

void 
set_timer(int interval)
{ // takes an interval (in seconds) and sets and starts a timer
    timer.it_value.tv_sec = interval;
    timer.it_value.tv_usec = 0; 
    timer.it_interval.tv_sec = interval;
    timer.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
	fprintf(stderr, "error calling setitimer()\n");
    }
} 

// prints debug diagnostic to console
void print_debug_msg(struct sockaddr_in *recv_addr, int msg_type, char *send_or_recv, char *username, char *channel, char *message, int line)
{
    char remote_ip[INET_ADDRSTRLEN]; // extract from recv_addr
    // populate the printable IP addresses
    inet_ntop(AF_INET, (void *)&recv_addr->sin_addr, remote_ip, INET_ADDRSTRLEN);
    //in_port_t remote_port = ntohs(recv_addr->sin_port); // extract from recv_addr (make sure correct byte order)
    in_port_t remote_port = ntohs(recv_addr->sin_port);
    char *type; // i.e. say, join, etc.
    char format_username[32];
    char format_channel[32];
    char format_msg[64];
    if (username == NULL)
	strcpy(format_username, "");
    else {
	strcpy(format_username, " ");
	strcat(format_username, username);
    }
    if (channel == NULL)
	strcpy(format_channel, "");
    else {
        strcpy(format_channel, " ");
	strcat(format_channel, channel);
    }
    if (message == NULL)
	strcpy(format_msg, "");
    else {
	strcpy(format_msg, " \"");
	strcat(format_msg, message);
	strcat(format_msg, "\"");
    }

    switch (msg_type) {
        case 0:
	    type = "Request Login";
	    break;
	case 1:
	    type = "Request Logout";
	    break;
	case 2:
	    type = "Request Join";
	    break;
	case 3:
	    type = "Request Leave";
	    break;
	case 4:
	    type = "Request Say";
	    break;
	case 5:
	    type = "Request List";
	    break;
	case 6:
	    type = "Request Who";
	    break;
	case 7:
	    type = "Request Keepalive";
	    break;
	case 8:
	    type = "S2S Join";
	    break;
	case 9:
	    type = "S2S Leave";
	    break;
	case 10:
	    type = "S2S Say";
	    break;
	case 11:   // for server-to-client
	    type = "Text Say";
	    break;
	case 12:
	    type = "Text List";
	    break;
	case 13:
	    type = "Text Who";
	    break;
	case 14:
	    type = "Text Error";
	    break;
    } //end switch
    printf("%d   %s:%d  %s:%d %s %s%s%s%s\n", line, MY_IP, PORT, remote_ip, remote_port, send_or_recv, type, format_username, format_channel, format_msg);
}

void add_stoch(server *s, cnode *ch)
{
    channel *ch_p = ch->c;
    // need to prevent duplicate adds!
    snode *nextserv = ch_p->sub_servers;
    server *serv;
    while (nextserv != NULL) { //going through the whole list of servers
	serv = nextserv->s;
	if (serv == s) {
	    return; // server is already subscribed to this channel
	}
	nextserv = nextserv->next;
    } // end while
    snode *snode_p = (snode *)malloc(sizeof(snode));
    snode_p->s = s;
    snode_p->prev = NULL; // it's going in at the head
    snode_p->next = ch_p->sub_servers; // new node points at whatever this head was pointing at
    if (ch_p->sub_servers != NULL) { // only if head wasn't NULL do we have an item that needs to update its prev
	(ch_p->sub_servers)->prev = snode_p;
    }
    ch_p->sub_servers = snode_p; // update the head of this list
    ch_p->servercount += 1;
}


void rm_sfromch(server *s, cnode *ch)
{   // first find the node 
    channel *ch_p = ch->c;   // handle on the actual channel
    snode *nextnode = ch_p->sub_servers; // start at the head
    snode *victim = NULL;
    while (nextnode != NULL) {
	if (nextnode->s == s) {
	    victim = nextnode;
	    break;
	}
	nextnode = nextnode->next;
    }
    if (victim == NULL) {
	printf("Server wasn't on channel %s... nothing to remove\n", ch->c->channelname);
	return; // server wasn't on that channel anyway
    }
    // if found, remove victim server  node from channel's list
    if ((victim->prev == NULL) && (victim->next == NULL)) { // case, ONLY NODE
	ch_p->sub_servers = NULL; // just set the head to null
    }
    else if ((victim->prev == NULL) && (victim->next != NULL)) { // case: head, multiple nodes
	//(c_node->next)->prev == NULL; // new head points back to nothing
	ch_p->sub_servers = victim->next; // head points to next down
    } else if ((victim->prev != NULL) && (victim->next != NULL)) { //case: middle of list
	(victim->next)->prev = victim->prev; // bridge the gap
	(victim->prev)->next = victim->next;
    } else if ((victim->prev != NULL) && (victim->next == NULL)) { // case: tail, multiple nodes
	(victim->prev)->next = NULL; // tail points out to nothing
    }
    // in call cases, free the list node
    free(victim);
    ch_p->servercount -= 1; // decrement servercount
    if (((ch_p->count == 0) && (strcmp(ch_p->channelname, "Common") != 0)) && (ch_p->servercount == 0))
	delete_channel(ch_p->channelname); // only if empty of users AND servers to forward
    return;
}

server *find_server(struct sockaddr_in *addr)
{
    int i = 0;
    for (i = 0; i < neighborcount; i++) {
        if ((addr->sin_addr.s_addr == servers[i].remote_addr.sin_addr.s_addr) 
			&& (addr->sin_port == servers[i].remote_addr.sin_port)) {
	    return &servers[i];
	}
    }
    return NULL;
}

 
int checkID(long long ID)
{
    int i;
    for (i = 0; i < ID_BUFSIZE; i++) {
	if (recentIDs[i] == ID) {
	    return 1; // ID found
	}
    }
    return 0; // ID not found
}


void generateID(long long *ID)
{
    FILE *fp;
    fp = fopen("/dev/urandom", "r");
    if (fp == NULL) {
	printf("Your file pointer is null\n");
	return;
    }
    fread(ID, 8, 1, fp);
}

