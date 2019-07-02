
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> 

#include <ctype.h> 



void *connection_handler(); 
char* readFromfile(char* nameOfFile);
int isNumber(char number[]);
char* readFromfile(char* nameOfFile);
void writeToFile(char* write, char *fileName);
void *addWork(int socketDesc);
int consumeWork();
void *addLog(char* clientMessage);
char *consumeLog();
void putWork(int value);
void putLog(char* clientMessage);
int getWork();
char* getLog();
void *logThreadHandler();


#define DEFAULT_PORT 8888
#define WORKER_COUNT 3
#define MAXWORK 3
#define MAXLOG 3
int portNumber;

//dictionary 
char *dict;
char* DEFAULT_DICTIONARY = "dictionary.txt";

// buffer where the producer which is the main thread adding socket descriptors () to the queue
int workQueue[MAXWORK];
int workQueStart = 0;
int workQueEnd = 0;

// A log queue where logs are written to it in different places and then the server log main thread will remove it from the queue and write it to a file 
char *logQueue[MAXLOG];
int logQueStart = 0;
int logQueEnd = 0;

int countWork = 0;
int countLog = 0;

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

pthread_cond_t empty1;
pthread_cond_t empty2;
pthread_cond_t full1;
pthread_cond_t full2;

int main(int argc , char *argv[]){
    int socket_desc , new_socket , c , *new_sock;
    struct sockaddr_in server , client;
    char *message;
	char *portNum;
    pthread_t threadPool[WORKER_COUNT]; 
	int threadIDs[WORKER_COUNT];
	
	pthread_cond_init(&empty1, NULL);
	pthread_cond_init(&empty2, NULL);
	pthread_cond_init(&full1, NULL);
	pthread_cond_init(&full2, NULL);
	
	int rc = pthread_mutex_init(&mutex1, NULL);
	int rc2 = pthread_mutex_init(&mutex2, NULL);
	
	 //get arguments form the command line to set the file name for the dictionary and port number 
	 if(argc > 1){
		if(isdigit(argv[1][0])){		
			portNumber = atoi(argv[1]);
			if(argc == 2)
				dict = readFromfile(DEFAULT_DICTIONARY);
			else{
				dict = readFromfile(argv[2]);
				printf("Dictionary name has been set.\n");
			}
			printf("Port number has been set to: %d\n",portNumber);
		}
		else{
			dict = readFromfile(argv[1]);
			if(argc == 2)
				portNumber = DEFAULT_PORT;
			else
				portNumber = atoi(argv[2]);
			printf("Dictionary name has been set.\n");
		}
	
	}else{
		dict = readFromfile(DEFAULT_DICTIONARY);
		portNumber = DEFAULT_PORT; 
	}
	  
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)    {
        printf("Could not create socket");
    }
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( portNumber );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)    {
        puts("bind failed");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc, 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
	
    c = sizeof(struct sockaddr_in);
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts("Connection accepted");
         
        //Reply to the client
        message = "Hello Client: This is a spell checking program. Type word!";
        write(new_socket , message , strlen(message));

		//socket descriptor stored in queue   
		pthread_mutex_lock(&mutex1);
		while(countWork == MAXWORK)
			pthread_cond_wait(&empty1, &mutex1);
		workQueue[workQueEnd] = new_socket;
		workQueEnd = (workQueEnd + 1) % MAXWORK;
		countWork++;
		pthread_mutex_unlock(&mutex1);
		pthread_cond_signal(&full1);
		
		//create worker threads to handle socket descriptors in work Queue
		int i;
		for(i = 0; i < WORKER_COUNT; i++){
			threadIDs[i] = i;
			//Start running the threads.
			if( pthread_create(&threadPool[i], NULL, connection_handler, NULL) < 0){
				perror("could not create thread");
				return 1;
			}
		}
		
		//create log thread to write to a log file 
		pthread_t logThread;
		if( pthread_create(&logThread, NULL, logThreadHandler, NULL) < 0){
			perror("could not create thread");
			return 1;
		}
        
        puts("Work Queue and Log Queue handlers have been assigned ");
		
    }
     
	 //error check
    if (new_socket<0)    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 

 //This will handle connection for each client
void *connection_handler(){
    //Get the socket descriptor
    int read_size;
    char client_message[2000];
	//char *client_message;
	int sock;
	puts("Handler is working... \n");
	
	//mutual exclusion to remove from the queue
	pthread_mutex_lock(&mutex1);
	while(countWork == 0)
		pthread_cond_wait(&full1, &mutex1);
	sock = workQueue[workQueStart];
	workQueStart = (workQueStart + 1) % MAXWORK;
	countWork--;
	pthread_mutex_unlock(&mutex1);
	pthread_cond_signal(&empty1);


 
    //Receive a message from client
	memset(client_message,0,strlen(client_message));
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
		
		//this is where the code goes to search for the word in the string goes 
		char* result = strstr(dict, client_message);
		
		if(result != NULL){
			strcat(client_message, "Ok! \n\n");
			write(sock, client_message, strlen(client_message));
		}
		else{
			strcat(client_message, "Misspelled! \n\n");
			write(sock, client_message ,strlen(client_message));
		}
		
		//This is where code adds to log Queue with mutual exclsuion
		logQueue[logQueEnd] = (char*)calloc(strlen(client_message),1);
		pthread_mutex_lock(&mutex2);
		while(countLog == MAXLOG)
			pthread_cond_wait(&empty2, &mutex2);
		
		strcpy(logQueue[logQueEnd], client_message); 
		printf("The Log: %s\n", logQueue[0]);
		logQueEnd = (logQueEnd + 1) % MAXLOG;
		countLog++;
		pthread_mutex_unlock(&mutex2);
		pthread_cond_signal(&full2);
		
		//empty the string 
		memset(client_message,0,strlen(client_message));
	}
	close(sock);
	 //error handling 
	if(read_size == 0){
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1){
		perror("recv failed");
	}
  
    return 0;
}

//checks if the parameter given is a number or not 
int isNumber(char number[]){
	int i = 0;
    for (; number[i] != 0; i++){
		//isdigit retruns true if the given string is a number 
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}

//This function is gonna read a file into a string a return it.
char* readFromfile(char* nameOfFile){
	char *string = (char*) malloc(1000000*sizeof(char*));    
    FILE *pointer;
    pointer = fopen(nameOfFile, "r");
    if(pointer == NULL){
        printf("ERROR: File not opened.\n");
        exit(3);
    }
    int i = 0;
    while((string[i++] = getc(pointer))!=EOF);
    string[i] = '\0';
	fclose(pointer);
	//printf("Sring: %s\n", string);
	return string;
}

//opens <fileName> and writes the cotenct within <write> to the given file
void writeToFile(char* write, char *fileName){
    FILE *pointer;
	pointer = fopen(fileName, "a");
	if(pointer == NULL){
		perror("Could not open input file");
	}
	else{
		puts("Has been wrote");
		fprintf(pointer, "\n%s", write);
		fprintf(pointer, "\r \n");
		fclose(pointer);
	}
}


//handles the process of getting a message from log queue and writing it to a log file 
void *logThreadHandler(){
	char *tmp;
	while(1){
		while (countLog != 0){
			pthread_mutex_lock(&mutex2);
			while(countLog == 0)
				pthread_cond_wait(&full2, &mutex2);
			tmp = logQueue[logQueStart];
			logQueStart = (logQueStart + 1) % MAXLOG;
			countLog--;
			pthread_mutex_unlock(&mutex2);
			pthread_cond_signal(&empty2);
			//writes to dictLog file
			writeToFile(tmp, "dictLog.txt");
		}
	}
	return 0;
}

