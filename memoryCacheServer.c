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

#define RESOURCE_SERVER_PORT 1061
#define BUF_SIZE 256
#define FILE_SIZE 32
#define CACHE_SIZE 8

/*
Features:
-Thread safe. Also, the data structure that you use to manage the cache could be modified by two threads at once.
 You should handle these cases appropriately.

-Your memory cache server should implement a hash map to quickly lookup if a file exists. Use the variable name as input 
and return an integer where you can then take the modulus to determine the entry in the cache. 

-The entry in a cache should only hold the most recently stored file. If there is a collision in your hash map,
replace the current entry with the new one that is being stored.

-In the map, you should also store the filename and size of the file.

-Be sure to ensure there are no memory leaks in your implementation. 

Commands:
load filename (Returns the length of the file followed by the contents.)
store filename n:[contents]  (Saves the filename in the cache with n being the size and contents being the contents)
rm filename (Deletes the file from the cache.)
*/

//Cached file object
struct cachedFile{
    //Key: 1-CACHE_SIZE
	int key;
    int length;
    char fileName[FILE_SIZE];
    char contents[BUF_SIZE];
	//Pointer to next cachedFile with same array index
	void* next;
} cachedFile;

//Global Variables
int serverSocket;
struct cachedFile* cacheArray[CACHE_SIZE];
char fileName[FILE_SIZE];
int fileStart;
int fileEnd;

//Takes in a name string and returns the resulting index for the hashmap 
int hashFileIndex(char * name){
    int hash = 0;
	//Iterates through the char * name and takes the numerical values of the characters and multiplies by the prime number 53
    for(int i = 0; i < strlen(name); i++){
        hash += ((int)name[i]) * 53;
    }
	//Sets the hash as the modulus of the resulting hash and the size of the cache
    hash %= CACHE_SIZE;
    return hash;
}

void parseFileName(char * inputReceived){
	//Can't return an array, so we must modify existing strings
	char * receiveLine = inputReceived;
	//Find Where FileName Starts And Ends in receiveLine
	for(int i=0; i<CACHE_SIZE; i++)
	{
		if(receiveLine[i] == ' ')
		{
			fileStart = i+1;
			for(int j=(fileStart); j<(fileStart+FILE_SIZE); j++)
			{
				if(receiveLine[j] == ' ' || receiveLine[j] == '\0')
				{
					fileEnd = j;
					break;
				}
			}
			break;
		}
	}
	
	// Parse File Name to fileName[]
	int tempCount = 0;
	for(int i=fileStart; i<fileEnd+1; i++)
			{
				if(receiveLine[i] == ' ')
				{
					fileName[tempCount] = '\0';
					printf("File Name: %s\n", fileName);
					break;
				}
				else
				{
					fileName[tempCount] = receiveLine[i];
					tempCount++;
				}
			}
}

//Might want to return something with implementation
void printCache(){}

//Saves the filename in the cache with n being the size and contents being the contents
void * store(void *inputReceived) 
{
	 char * receiveLine = (char*)inputReceived;
	 char contents[BUF_SIZE];
	 char fileLength[FILE_SIZE];
	 int lengthEnd;
	 char lengthOfFile[FILE_SIZE];
	 // Run Parse File Name
	 parseFileName(receiveLine);
	
	//Find Where Declared Length of File Ends
	for(int i=0; i<BUF_SIZE; i++)
	{
		if(receiveLine[i] == ':')
		{
			lengthEnd = i;
			break;
		}
	}
	
	// Parse Contents to contents[]
	int tempCount = 0;
	for(int i=lengthEnd+2; i<(BUF_SIZE); i++)
			{
				if(receiveLine[i] == ']')
				{
					contents[tempCount] = '\0';
					printf("Contents: %s\n", contents);
					break;
				}
				else
				{
					contents[tempCount] = receiveLine[i];
					tempCount++;
				}
			}
			
	//Parse Length of File
	int lengthIndex = 0;
	for(int i=(fileEnd+1); i<lengthEnd+FILE_SIZE; i++)
	{
		if(i == lengthEnd) // Last Time through the loop add null terminator and reset index. atoi Function needs null terminator;
		{
			lengthOfFile[lengthIndex] = '\0';
			lengthIndex = 0;
			printf("Length: %s\n", lengthOfFile);
		}
		else
		{
		lengthOfFile[lengthIndex] = receiveLine[i];
		lengthIndex++;
		}
	}

	/*
	So we have one array for the cache called cacheArray[]. The array is made of pointers for structs named cachedFile. 
	The cachedFile contains all information on the file as well as a key and a pointer. The key is used to determine how many 
	objects are in the array. The pointer should be for another cachedFile* for a cachedFile with the same index. The index is 
	determine by the hash function, which takes in a name string and returns the index as an int. Use the hash function to 
	determine the location in cachedArray[]. Store it there if it is empty. Overwrite it if it is the same file name. 
	Set it as the pointer of the last object if it is new. If it is new, be sure to kick out the oldest cachedFile and change
	the keys of all files.
	*/

	int index = hashFileIndex(fileName);
	struct cachedFile* file = cacheArray[index];
	bool found=false;
	if(file!=NULL){
		while(file->next!=NULL){
			if(strcmp(file->fileName,fileName)==0){
				found=true;
				//overwrite
			}
			file= file->next;
		}
		if (found==true){
			//save new
		}

	}

	//Create First Open Slot Using Data Parsed
	/*
	for(int i=0; i<CACHE_SIZE; i++)
	{
		if(cacheArray[i]->key > 0) {}
		
		else
		{
				strncpy(cacheArray[i]->fileName, fileName, FILE_SIZE);
				cacheArray[i]->length = atoi(lengthOfFile);
				break;
		}
	}
	*/
}

//Deletes the file from the cache.
/*void * remove(void * inputReceived) {
	char * receiveLine= (char *)inputReceived;
	char command[FILE_SIZE];
	char fileName[FILE_SIZE];
	parseCommandAndFileName(receiveLine, command, fileName);
	int index = hashFileIndex(fileName);
	struct cachedFile* file = cacheArray[index];
	if(file!=NULL){
		while(file->next!=NULL){
			if(file->fileName==fileName){
				//delete and set previous pointer as 'file->next
			}
			file= file->next;
		}
	}
	return NULL;
}*/

//Returns the length of the file followed by the contents.
/*void * load(void * inputReceived) {
	return NULL;
}*/

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
		
		if(strncmp(receiveLine, "store", 4)==0)
		{
			store(inputLine);
		}
		else if(strncmp(receiveLine, "rm", 2)==0)
		{
			//remove(inputLine);
		}
		else if(strncmp(receiveLine, "load", 4)==0)
		{
			//load(inputLine);
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
	//for(int i=0; i<CACHE_SIZE; i++)
	//{
	//		entryarr[i].order = i;
	//}
  
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