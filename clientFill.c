#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#define SERVER_PORT 1060 /// CHANGE!
#define BUF_SIZE 256

char store[BUF_SIZE];
char * content= " 4096:[content]";
char sendLine[BUF_SIZE];
char receiveLine[BUF_SIZE];

void * fill(void * character){
    printf("\nFilling\n");
    strcpy(store, "store ");
    int serverSocket, bytesRead;
    
    // These are the buffers to talk back and forth with the server
    char * name = (char *) character;
    // Create socket to server
    if ( (serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Unable to create socket\n");
    	exit(-1);
	//    return -1;
    }
    
    // Setup server connection
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress)); // Ensure address is blank
    
    // Setup the type of connection and where the server is to connect to
    serverAddress.sin_family = AF_INET; // AF_INET - talk over a network, could be a local socket
    serverAddress.sin_port   = htons(SERVER_PORT); // Conver to network byte order
    
    // Try to convert character representation of IP to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("Unable to convert IP for server address\n");
    	exit(-1);
	//    return -1;
    }
    
    // Connect to server, if we cannot connect, then exit out
    if (connect(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        printf("Unable to connect to server");
    	exit(-1);
    }
    
    strcat(store, name);
    strcat(store, content);
    strcpy(sendLine, store);
    printf("\nSending: %s\n", sendLine);
    // Write will actually write to a file (in this case a socket) which will transmit it to the server
    write(serverSocket, sendLine, strlen(sendLine));
    
    // Now start reading from the server
    // Read will read from socket into receiveLine up to BUF_SIZE
    while ( (bytesRead = read(serverSocket, receiveLine, BUF_SIZE)) > 0) {
        receiveLine[bytesRead] = 0; // Make sure we put the null terminator at the end of the buffer
        printf("Received %d bytes from server with message: %s\n", bytesRead, receiveLine);
        
        // Got response, get out of here
        break;
    }
    
    // Close the server socket
    close(serverSocket);
    //return 1;
}

int main(int argc, char *argv[]) {;
	char * a = "a";
	char * b = "b";
	char * c = "c";
	char * d = "d";
	char * e = "e";
	char * f = "f";
	char * g = "g";
	char * h = "h";
	fill(a);
	fill(b);
	fill(c);
	fill(d);
	fill(e);
	fill(f);
	fill(g);
	fill(h);
}
