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
#define FILE_SIZE 32
#define CACHE_SIZE 8

/*
Commands:
	load filename (Returns the length of the file followed by the contents.)
	store filename n:[contents]  (Saves the filename in the cache with n being the size and contents being the contents)
	rm filename (Deletes the file from the cache.)

Note:
	So we have one array for the cache called cacheArray[]. The array is made of pointers for structs named cachedFile. 
	The cachedFile contains all information on the file. The key is the unique name. The index is 
	determined by the hash function, which takes in a name string and returns the index as an int. Use the hash function to 
	determine the location.
*/

//Cached file object
struct cachedFile{
	//Size
	int length;
	//This is the unique key!
	char fileName[FILE_SIZE];
	//Contents of the file
    	char contents[BUF_SIZE];
	//Mutex to make the array thread safe
	pthread_mutex_t fileLock;
	//No pointer is needed as the file will be replaced when collisions occur
} cachedFile;

//Global Variables
int serverSocket;
struct cachedFile* cacheArray[CACHE_SIZE];

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

//Takes the input received and parses the file name to fileName while returning an array of ints for the endpoints
int * parseFileName(char * inputReceived, char * fileName){
	static int fileEndpoints[2];
	char * receiveLine = inputReceived;
	//Find Where FileName Starts And Ends in receiveLine
	for(int i=0; i<CACHE_SIZE; i++)
	{
		if(receiveLine[i] == ' ')
		{
			fileEndpoints[0] = i+1;
			for(int j=(fileEndpoints[0]); j<(fileEndpoints[0]+FILE_SIZE); j++)
			{
				if(receiveLine[j] == ' ' || receiveLine[j] == '\0' || receiveLine[j]=='\n')
				{
					fileEndpoints[1] = j;
					break;
				}
			}
			break;
		}
	}
	
	// Parse File Name to fileName[]
	int tempCount = 0;
	for(int i=fileEndpoints[0]; i<fileEndpoints[1]+1; i++)
	{
		if(receiveLine[i] == ' ' || receiveLine[i]== '\n')
		{
			fileName[tempCount] = '\0';
			break;
		}
		else
		{
			fileName[tempCount] = receiveLine[i];
			tempCount++;
		}
	}
	return fileEndpoints;
}

//Prints out information on files in cache
void printCache(){
	printf("\nPrinting Cache:\n\n");
	for (int i=0; i<CACHE_SIZE; i++){
		if(cacheArray[i]!=NULL && *(cacheArray[i]->fileName)!= '\0'){
			printf("Index: %d\n", i);
			printf("File:%s\n", cacheArray[i]->fileName);
			printf("Size:%d\n", cacheArray[i]->length);
			printf("Contents:%s\n", cacheArray[i]->contents);
			printf("\n");
		}
	}
}

//Saves the filename in the cache with n being the size and contents being the contents
void * store(void *inputReceived) {
	printf("\nStoring File\n");
	printf("\n******************************************************************\n");
	
	int lengthEnd;
	int lengthParsed;
	char * receiveLine = (char*)inputReceived;
	char contents[BUF_SIZE];
	char fileLength[FILE_SIZE];
	char fileName[FILE_SIZE];
	char lengthOfFile[FILE_SIZE];
	
	//Parse filename
	int * fileEndpoints = parseFileName(receiveLine, fileName);
	
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
	for(int i=lengthEnd+1; i<(BUF_SIZE); i++)
	{
		if (receiveLine[i] == '[' || receiveLine[i] == '\n'){
			i++;
		}

		if (receiveLine[i] == ']')
		{
			contents[tempCount] = '\0';
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
	for(int i=(*(fileEndpoints+1)+1); i<lengthEnd+FILE_SIZE; i++)
	{
		if(i == lengthEnd) // Last Time through the loop add null terminator and reset index. atoi Function needs null terminator;
		{
			lengthOfFile[lengthIndex] = '\0';
			lengthIndex = 0;
			lengthParsed = atoi(lengthOfFile);
		}
		else
		{
			lengthOfFile[lengthIndex] = receiveLine[i];
			lengthIndex++;
		}
	}

	//Find index
	int index = hashFileIndex(fileName);
	
	//Allocating memory
	if (cacheArray[index]==NULL){
		cacheArray[index] = malloc(sizeof(cachedFile));
	}

	//Lock here
	pthread_mutex_lock(&(cacheArray[index]->fileLock));
		
	//Set the file
	strcpy(cacheArray[index]->fileName,fileName);
	strcpy(cacheArray[index]->contents,contents);
	cacheArray[index]->length=lengthParsed;
	
	//Unlock here
	pthread_mutex_unlock(&(cacheArray[index]->fileLock));
	
	//Print out the cache
	printCache();
}

//Deletes the file from the cache.
void removeFile(void * inputReceived) {
	printf("\nRemoving File\n");
	printf("\n******************************************************************\n");
	
	char * receiveLine = (char *)inputReceived;
	char fileName[FILE_SIZE];
	parseFileName(receiveLine, fileName);
	int index = hashFileIndex(fileName);
	if(cacheArray[index]!=NULL && strcmp(cacheArray[index]->fileName, fileName)==0){
		//Lock here
		pthread_mutex_lock(&(cacheArray[index]->fileLock));
		
		//check if it was deleted
		if (*(cacheArray[index]->fileName)!= '\0'){
			//Null values then free and null pointer
			*(cacheArray[index]->fileName)='\0';
			*(cacheArray[index]->contents)='\0';
			cacheArray[index]->length=0;
			
			//Do not want to do this with current implementation
			//pthread_mutex_destroy(&(cacheArray[index]->fileLock));
			//free(cacheArray[index]);
			//cacheArray[index]=NULL;
		}
		//Unlock here
		pthread_mutex_unlock(&(cacheArray[index]->fileLock));

	}
	printCache();
}

//Returns the length of the file followed by the contents.
void load(void * inputReceived, int request) {
	printf("\nLoading File\n");
	printf("\n******************************************************************\n");
	
	int connectionToClient = request;
	char * receiveLine = (char *)inputReceived;
	char sendLine[BUF_SIZE];
	char fileName[FILE_SIZE];
	parseFileName(receiveLine, fileName);
	int index = hashFileIndex(fileName);
	if(cacheArray[index]!=NULL && strcmp(cacheArray[index]->fileName, fileName)==0){
		//Lock here
		pthread_mutex_lock(&(cacheArray[index]->fileLock));

		if (*(cacheArray[index]->fileName)!= '\0'){
			//Load Contents
			snprintf(sendLine, sizeof(sendLine), "%d:%s", cacheArray[index]->length, cacheArray[index]->contents);
        		write(connectionToClient, sendLine, strlen(sendLine));
		
        		// Zero out the receive line so we do not get artifacts from before
        		bzero(&receiveLine, sizeof(receiveLine));
        		close(connectionToClient);

			//printf("\n%d:[%s] \n", cacheArray[index]->length, cacheArray[index]->contents);
		}
		else{
			//Send 0:
			snprintf(sendLine, sizeof(sendLine), "0:");
       			write(connectionToClient, sendLine, strlen(sendLine));
       			
    			// Zero out the receive line so we do not get artifacts from before
        		bzero(&receiveLine, sizeof(receiveLine));
        		close(connectionToClient);
		}
		printf("Sending %s\n", sendLine);
		//Unlock here
		pthread_mutex_unlock(&(cacheArray[index]->fileLock));
	}
	else {
		//Send 0:
		snprintf(sendLine, sizeof(sendLine), "0:");
		printf("Sending %s\n", sendLine);
       		write(connectionToClient, sendLine, strlen(sendLine));
       		
    		// Zero out the receive line so we do not get artifacts from before
        	bzero(&receiveLine, sizeof(receiveLine));
        	close(connectionToClient);
	}
}

// We need to make sure we close the connection on signal received, otherwise we have to wait for server to timeout.
void closeConnection() {
    printf("\nClosing Connection with file descriptor: %d \n", serverSocket);
    close(serverSocket);
    exit(1);
}

// Handle request
void * processClientRequest(void * request) {
    int connectionToClient = *(int *)request;
    int bytesReadFromClient = 0;
    char receiveLine[BUF_SIZE];
    char sendLine[BUF_SIZE];
    snprintf(sendLine, sizeof(sendLine), "true");

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
			//snprintf(sendLine, sizeof(sendLine), "Received Store command");
		}
		else if(strncmp(receiveLine, "rm", 2)==0)
		{
			removeFile(inputLine);
			//snprintf(sendLine, sizeof(sendLine), "Received Remove command");
		}
		else if(strncmp(receiveLine, "load", 4)==0)
		{
			load(inputLine, connectionToClient);
			//snprintf(sendLine, sizeof(sendLine), "Received Load command");
		}
		else{
			snprintf(sendLine, sizeof(sendLine), "Please use correct syntax");
		}
		//END OUR CODE
	  
        printf("Sending: %s\n", sendLine);
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

    //Able to start
    printf("\nStarted Memory Cache Server\n");

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
