#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXDATASIZE 256
#define PORT "12000"
#define BACKLOG 8 


// Handle SIGCHLD signal - cleans up the child's entry in the process table
void sigchld_handler(int s) {
	(void)s; // For unused variable warning
	int saved_errno = errno; // waitpid() might overwrite errno
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


int main() {
    struct sigaction sa;

    int welcomeSocketDesc, clientSocketDesc, responseCode;
    struct addrinfo hints, *serverInfo, *itr;
    
    // SERVER SETUP
    // Setup hints addrinfo
    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 

    // Generate all possible addrinfo structs for local IP and port number
    if ((responseCode = getaddrinfo(NULL, PORT, &hints, &serverInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(responseCode));
		return 1;
	}

    // Loop through all addrinfo structs and bind our "welcome" port to the first we can
	for(itr = serverInfo; itr != NULL; itr = itr->ai_next) {
		if ((welcomeSocketDesc = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (bind(welcomeSocketDesc, itr->ai_addr, itr->ai_addrlen) == -1) {
			close(welcomeSocketDesc);
            perror("server: bind");
			continue;
		}

		break;
	}

    if (itr == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 1;
	}

	// After binding (or failing to bind), we no longer neeed the linked list of addrinfo structs
	freeaddrinfo(serverInfo);

    // Setup "welcome" socket to listen for incoming connections
    if (listen(welcomeSocketDesc, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

    // Set up SIGCHLD signal handler
    // Remove child process entry from process table
    sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("Computer A: Waiting for messages!\n");


    // SEND AND RECEIVE DATA
    // The following is used for setting up the client socket file descriptor
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;

    // Accept client connection request, fork process, then send and recieve messages
    while(1) {
        sin_size = sizeof(clientAddr);
		clientSocketDesc = accept(welcomeSocketDesc, (struct sockaddr *)&clientAddr, &sin_size);
		if (clientSocketDesc == -1) {
			perror("accept");
			continue;
		}

		if (!fork()) {
            // Child doesn't need the "welcome" port because this port is still listening on the parent process
			close(welcomeSocketDesc); 
			
            // Receive and print client's message
            int numBytesRead;
            char recvBuffer[MAXDATASIZE];
            if ((numBytesRead = recv(clientSocketDesc, recvBuffer, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
			recvBuffer[numBytesRead] = '\0';
			printf("Computer B's message: %s\n", recvBuffer);
			
            // Send reply to client
            char msg[MAXDATASIZE];
            printf("Enter message: ");
            scanf("%[^\n]", msg);
            int len = strlen(msg);	
            if (send(clientSocketDesc, msg, len, 0) == -1) {
                perror("send");
                exit(1);
            }
			
            close(clientSocketDesc);
			exit(0);
		}
        
		close(clientSocketDesc);
	}


    return 0;
}