#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

#define RESOURCE_SERVER_PORT 106
#define BUF_SIZE 256

/*
Features:
-You should ensure that operations with the cache are thread safe, meaning, there could exist a race
condition if one thread deletes a file from memory while another is being read. Also, the data structure
that you use to manage the cache could be modified by two threads at once. You should handle these
cases appropriately.

-Your memory cache server should implement a hash map to quickly lookup if a file exists. Your hash
function is up to you, but you should use the variable name as input and return an integer where you can
then take the modulus to determine the entry in the cache. 

-The cache can hold up to 8 entries. The entry in a cache should only hold the most recently stored file. If there is a collision in your hash map,
replace the current entry with the new one that is being stored.

-In the map, you should also store the filename and size of the file.

-Be sure to ensure there are no memory leaks in your implementation. 

Commands:
Load from Cache syntax: load filename (Returns the length of the file followed by the contents.)
Store to Cache syntax: store filename n:[contents]  (Saves the filename in the cache with n being the size and contents being the contents)
Delete Cache syntax: rm filename (Deletes the file from the cache.)
*/

int serverSocket;

// We need to make sure we close the connection on signal received, otherwise we have to wait for server to timeout.
void closeConnection() {
    printf("\nClosing Connection with file descriptor: %d \n", serverSocket);
    close(serverSocket);
    exit(1);
}

// Handle request
void * processClientRequest(void * request) {
    int connectionToClient = *(int *)request;
    char receiveLine[BUF_SIZE];
    char sendLine[BUF_SIZE];
    
    int bytesReadFromClient = 0;
    // Read the request that the client has
    while ( (bytesReadFromClient = read(connectionToClient, receiveLine, BUF_SIZE)) > 0) {
        // Need to put a NULL string terminator at end
        receiveLine[bytesReadFromClient] = 0;
        
        // Show what client sent
        printf("Received: %s\n", receiveLine);
      
        // Print text out to buffer, and then write it to client (connfd)
        snprintf(sendLine, sizeof(sendLine), "true");
      
        printf("%Sending s\n", sendLine);
        write(connectionToClient, sendLine, strlen(sendLine));
        
        // Zero out the receive line so we do not get artifacts from before
        bzero(&receiveLine, sizeof(receiveLine));
        close(connectionToClient);
    }
}

int main(int argc, char *argv[]) {
    int connectionToClient, bytesReadFromClient;
  
    // Create a server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    
    // INADDR_ANY means we will listen to any address
    // htonl and htons converts address/ports to network formats
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(RESOURCE_SERVER_PORT);
    
    // Bind to port
    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
        printf("Unable to bind to port just yet, perhaps the connection has to be timed out\n");
        exit(-1);
    }
    
    // Before we listen, register for Ctrl+C being sent so we can close our connection
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = closeConnection;
    sigIntHandler.sa_flags = 0;
    
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    // Listen and queue up to 10 connections
    listen(serverSocket, 10);
    
    while (1) {
        /*
         Accept connection (this is blocking)
         2nd parameter you can specify connection
         3rd parameter is for socket length
         */
        connectionToClient = accept(serverSocket, (struct sockaddr *) NULL, NULL);
        
        // Kick off a thread to process request
        pthread_t someThread;
        pthread_create(&someThread, NULL, processClientRequest, (void *)&connectionToClient);
        
    }
}