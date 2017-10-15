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
#include <sys/socket.h>
#include <netinet/in.h>
#include "duckchat.h"
#define SERVER_HOST_IP_ADDRESS 'localhost'
#define SERVER_TCP_PORT ?
int
main(int argc, char **argv) {
    // Parse command-line arguments
    // add error checking?? is it ok that these are upper case??
    // host name where server is running
    char SERVER_HOST_IP_ADDRESS[108] = argv[1]; // is this buffer size ok??
    // port number on which server is listening
    int SERVER_PORT = atoi(argv[2]);
    // user's username
    char USERNAME[USERNAME_MAX] = argv[3];


    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(ser_addr));// set fields to NULL
    // specify server info
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(SERVER_HOST_IP_ADDRESS);
    serv_addr.sin_port = htons(SERVER_TCP_PORT);

    if (( sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("client: cannot connect to server");

    // send request to server, receive reply

    close(sockfd);


}
