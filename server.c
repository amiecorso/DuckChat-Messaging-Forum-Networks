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
login(uname)
logout(uname)
join(uname, chname))
leave(uname, chname)
say(uname, chname)
list(uname, chname)
who(uname, chname)
keep_alive(uname) (update time)
 --> these may call subfunctions such as 
create_user(uname)
create_channel(chname)
delete_user(uname)
delete_channel(chname)
add_utoch(uname, chname)
add_chtou(uname, chname)
rm_ufromch(uname, chname)
rm_chfromu(chname, chname)
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
}
