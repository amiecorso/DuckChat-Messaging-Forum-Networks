/* DuckChat Server program
Author: Amie Corso
532 Intro to Networks - Program 1
Fall 2017
*/

#include <stdio.h>
#include <stdlib.h>

/*

To make the project seem more real, but not without the added complexity of TCP, DuckChat uses UDP to communicate. While UDP is not reliable, we can run the client and server on the same machine and not worry about dropping packets or out of order packets.

The server takes two arguments: the host address to bind to and the port number. The host address can be either 'localhost', the IP host name of the machine that the server is running on, or the IP address of an interface connected to the machine. Once the server is running, clients may use the host name and port to connect to the server. Note that if you use 'localhost', you will not be able to connect to the server from another machine, but you also do not have to worry about dropped packets.

The server does not need to directly interact with the user in any way. However, it is strongly recommended that the server outputs debugging messages to the console. For example, each time the server receives a message from a client, it can output a line describing the contents of the message and who it is from using the following format: [channel][user][message] where message denotes a command and its parameters (if any).

The server's primary job is to deliver messages from a user X to all users on X's active channel. To accomplish this, the server must keep track of individual users and the channels they have subscribed to. On the flip side, the server must also track each channel and the subscribed users on it.

Channel creation and deletion at server are handled implicitly. Whenever a channel has no users, it is deleted. Whenever a user joins a channel that did not exist, it is created.

If the server receives a message from a user who is not logged in, the server should silently ignore the message.

SKETCH OF A SERVER IN C
if (( sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror("server: can't open stream socket");

bzero((char *)&serv_addr, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
serv_addr.sin_port = htons(SERV_TCP_PORT); //host to network short

if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    perror("Server: can't bind local address");

listen(sockfd, 5);

while(1) {
    // accept a connect req from a client and return a new socket for that connection
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    // receive request from client and send reply

    close(newsockfd);

}
*/
int
main(int argc, char **argv) {

    if (( sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("server: can't open stream socket");



}
