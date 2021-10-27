#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAXDATASIZE 256 


int main(int argc , char *argv[]) {
	int socketDesc, responseCode;
    struct addrinfo hints, *serverInfo, *itr;

	// We need Computer A's hostname and port number
	if (argc != 3) {
		fprintf(stderr, "requires: <hostname> <port>\n");
		exit(1);
	}

	// CLIENT SETUP    
	// Set up hints addrinfo
    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE; 

	// Generate all possible addrinfo structs from hostname and port number provided
    if ((responseCode = getaddrinfo(argv[1], argv[2], &hints, &serverInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(responseCode));
		return 1;
	}

	// Loop through all addrinfo structs and connect to the first we can
	for(itr = serverInfo; itr != NULL; itr = itr->ai_next) {
		if ((socketDesc = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(socketDesc, itr->ai_addr, itr->ai_addrlen) == -1) {
			perror("client: connect");
			close(socketDesc);
			continue;
		}

		break;
	}

	if (itr == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 1;
	}

	// After connecting (or failing to connect), we no longer neeed the linked list of addrinfo structs
	freeaddrinfo(serverInfo);


	// SEND AND RECIEVE DATA
	// Send message to server
	char msg[MAXDATASIZE];
	printf("Enter message: ");
	scanf("%[^\n]", msg);
	int len = strlen(msg);	
	if (send(socketDesc, msg, len, 0) == -1) {
		perror("send");
		exit(1);
	}

	// Wait for reply from server then close connection
	int numBytesRead;
	char recvBuffer[MAXDATASIZE];
	if ((numBytesRead = recv(socketDesc, recvBuffer, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	recvBuffer[numBytesRead] = '\0';
	printf("Computer B's reply: '%s'\n", recvBuffer);
	
	close(socketDesc);

		
	return 0;
}