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
#include <stdbool.h>

#define RESOURCE_SERVER_PORT 1060
#define BUF_SIZE 256
#define CACHE_SIZE 8

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

struct cachedFile{
    int key;
    char fileName[BUF_SIZE];
    char contents[BUF_SIZE];
} cachedFile;

struct hashObject {
    //key 1-CACHE_SIZE
    int key;
    char value[BUF_SIZE];
    //Pointer to next hashObject
    void* next;
} hashObject;

int serverSocket;
struct hashObject* hashArray[CACHE_SIZE];
//struct cachedFile* cacheArray[CACHE_SIZE]; Necessary?

int hashFileIndex(char * name){
    int hash = 0;
    for(int i = 0; i < strlen(name); i++){
        hash += ((int)name[i]) * 53;
    }
    hash %= CACHE_SIZE;
    return hash;
}

bool searchHashMap(char * value){
    bool found = false;
    int index = hashFileIndex(value);
    struct hashObject* object = hashArray[index];
    if(object!=NULL){
        while(object->next!= NULL){
            if(strcmp(object->value,value) == 0){
                found = true;
            }
            object= object->next;
        }
    }
    return found;
}

//Might want to return something with implementation
void deleteHashMap(){}
void deleteCache(){}
//void printHashmap(){}
//void printCache(){}

struct entry
{  
    char name[32];
	int order;
    int length;
};
struct entry entryarr[8];


void * store(void *inputReceived) 
{
	 char * receiveLine = (char*)inputReceived;
	 char fileName[32];
	 char fileLength[32];
	 int fileStart;
	 int fileEnd;
	 int lengthEnd;
	 char lengthOfFile[32];
	 
	//Find Where FileName Starts And Ends in receiveLine
	for(int i=0; i<8; i++)
	{
		if(receiveLine[i] == ' ')
		{
			fileStart = i+1;
			for(int j=(fileStart); j<(fileStart+32); j++)
			{
				if(receiveLine[j] == ' ')
				{
					fileEnd = j;
					break;
				}
			}
			break;
		}
	}
	
	//Find Where Declared Length of File Ends
	for(int i=0; i<BUF_SIZE; i++)
	{
		if(receiveLine[i] == ':')
		{
			lengthEnd = i;
			break;
		}
	}
	
	// Parse File Name to fileName[]
	for(int i=fileStart; i<fileEnd+1; i++)
			{
				if(receiveLine[i] == ' ')
				{
					fileName[i] = '\0';
					break;
				}
				else
					fileName[i] = receiveLine[i];
			}
			
	//Parse Length of File
	int lengthIndex = 0;
	for(int i=(fileEnd+1); i<lengthEnd; i++)
	{
		lengthOfFile[lengthIndex] = receiveLine[i];
		lengthIndex++;
		if(i == lengthIndex-1) // Last Time through the loop add null terminator and reset index. atoi Function needs null terminator;
		{
			lengthOfFile[lengthIndex] = '\0';
			lengthIndex = 0;
		}
	}
	
	//Create First Open Slot Using Data Parsed
	for(int i=0; i<8; i++)
	{
		if(entryarr[i].length > 0) {}
		
		else
		{
				strncpy(entryarr[i].name, fileName, 32);
				entryarr[i].length = atoi(lengthOfFile);
				break;
		}
	}
}

//void remove() {}

//void load() {}

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
      
	 //OUR CODE HERE	
		char *inputLine = receiveLine;
		
	if(receiveLine[0] == 's' && receiveLine[1] == 't' && receiveLine[2] == 'o' && receiveLine[3] == 'r' && receiveLine[4] == 'e')
	{
		store(inputLine);
	}
	else if(receiveLine[0] == 'r' && receiveLine[1] == 'm')
	{
		//remove();
	}
	else if(receiveLine[0] == 'l' && receiveLine[1] == 'o' && receiveLine[2] == 'a' && receiveLine[3] == 'd')
	{
		//load();
	}
	//END OUR CODE
	  
        // Print text out to buffer, and then write it to client (connfd)
        snprintf(sendLine, sizeof(sendLine), "true");
      
        printf("Sending %s\n", sendLine);
        write(connectionToClient, sendLine, strlen(sendLine));
        
        // Zero out the receive line so we do not get artifacts from before
        bzero(&receiveLine, sizeof(receiveLine));
        close(connectionToClient);
    }
}

int main(int argc, char *argv[]) {
    int connectionToClient, bytesReadFromClient;
	//Initialize Cache Array
	for(int i=0; i<8; i++)
	{
			entryarr[i].order = i;
	}
  
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