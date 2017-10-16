/* Duckchat client program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017
*/

/* 
client takes 3 command-line arguments:
The first is theient starts, it automatically connects to the chat server, joins a channel called “Common”, and provides the user a prompt (i.e. the client must send the join message to join “Common”). When the user types/enters text at the prompt and hits 'Enter', the text is sent to the server (using the "say request" message), and the server relays the text to all users on the channel (including the speaker).

The exception is when the text begins with a forward slash ('/') character. In that case, the text is interpreted as a special command. These special commands are supported by the DuckChat client:

/exit: Logout the user and exit the client software.
/join channel: Join (subscribe in) the named channel, creating the channel if it does not exist.
/leave channel: Leave the named channel.
/list: List the names of all channels.
/who channel: List the users who are on the named channel.
/switch channel: Switch to an existing named channel that user has already joined.host name where the server is running. 
The second argument is the port number on which the server is listening. 
The third argument is the user's username.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "duckchat.h"
int
main(int argc, char **argv) {
    // Parse command-line arguments
    if (argc < 4) {
        perror("Client: missing arguments.");
        return 0;
    }
    // add error checking?? is it ok that these are upper case??
    // host name where server is running
    char SERVER_HOST_IP_ADDRESS[64]; // is this buffer size ok??
    //TODO: check for "localhost" and convert to 127.00..??
    strcpy(SERVER_HOST_IP_ADDRESS, argv[1]);
    // port number on which server is listening
    int SERVER_PORT = atoi(argv[2]);
    // user's username
    char USERNAME[USERNAME_MAX];
    strcpy(USERNAME, argv[3]);


    struct sockaddr_in client_addr; // our local port information
    struct sockaddr_in serv_addr; // remote server information
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
    bzero((char *)&serv_addr, sizeof(serv_addr));// set fields to NULL
    serv_addr.sin_family = AF_INET;
    // USE NPE
    inet_pton(AF_INET, SERVER_HOST_IP_ADDRESS, &(serv_addr.sin_addr)); // convert from char[] to network address
    //serv_addr.sin_addr.s_addr = htonl(SERVER_HOST_IP_ADDRESS); do we need to get at sin_addr.s_addr???
    serv_addr.sin_port = htons(SERVER_PORT);

    unsigned int alen = sizeof(client_addr);
    if (getsockname(sockfd, (struct sockaddr *)&client_addr, &alen) < 0) {
        perror("getsockname failed");
        return 0;
    }
    printf("bind complete.  Port number = %d\n", ntohs(client_addr.sin_port));
    // send request to server, receive reply

    close(sockfd);
}
