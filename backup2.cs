#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include <pthread.h> 
//#include <pthread.h> //for threading , link with lpthread
#include <ctype.h> 

typedef struct protect {
	int value;
	pthread_cond_t cond;
	pthread_mutex_t lock;
} protect;


void *connection_handler(); 
char* readFromfile(char* nameOfFile);
int isNumber(char* number);
char* readFromfile(char* nameOfFile);
void writeToFile(char* write, char *fileName);
void init(protect *s, int value);
void protectWait(protect *s);
void protectPost(protect *s);
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
#define MAX 10
int portNumber;

//dictionary 
char *dict;
char* DEFAULT_DICTIONARY = "dictionary.txt";

// buffer where the producer which is the main thread adding socket descriptors () to the queue
int *workQueue[MAX];
int workQueStart = 0;
int workQueEnd = 0;

// A log queue where logs are written to it in different places and then the server log main thread will remove it from the queue and write it to a file 
char *logQueue[1000000];
int logQueStart = 0;
int logQueEnd = 0;

protect mutex1;
protect mutex2;

protect empty1;
protect empty2;
protect full1;
protect full2;

int main(int argc , char *argv[]){
    int socket_desc , new_socket , c , *new_sock;
    struct sockaddr_in server , client;
    char *message;
    pthread_t threadPool[WORKER_COUNT]; 
	int threadIDs[WORKER_COUNT];
	init(&mutex1, 1);
	init(&mutex2, 1);
	init(&empty1, MAX);
	init(&empty2, MAX);
	init(&full1, 0);
	init(&full2, 0);
	

	 //get arguments form the command line to set the file name for the dictionary and port number 
	 if(argc > 1){
		int i = 1;
		while (i < argc){
			if(isNumber(argv[i])){			
				portNumber = atoi(argv[i]);
				printf("Port number has been set to: %d\n",portNumber);
			}
			else{
				dict = argv[i];
				printf("Dictionary name has been set to: %s\n",dict);
			}
			i++;
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
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)    {
        puts("bind failed");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
	
    c = sizeof(struct sockaddr_in);
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        puts("Connection accepted");
         
        //Reply to the client
        message = "Hello Client: This is a spell checking program. Type word!";
        write(new_socket , message , strlen(message));
         
		//socket descriptor stored in queue   
		addWork(new_socket);
		
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
        
		
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
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
	int sock = consumeWork();
     
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
		
		//log the word sent to the log file over here 
		addLog(client_message);
		
		//this is where the code goes to search for the word in the string goes 
		char* result = strstr(dict, client_message);
		if(result != NULL){
			strcat(client_message, "Ok! \n\n");
			addLog(client_message);
			write(sock, client_message, strlen(client_message));
		}
		else{
			strcat(client_message, "Misspelled! \n\n");
			addLog(client_message);
			write(sock, client_message ,strlen(client_message));
		}
		//empty the string 
		memset(client_message,0,strlen(client_message));
    }
     
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
int isNumber(char* number){
	int i = 0;
    for (; number[i] != strlen(number); i++){
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
		fprintf(pointer, "%s", write);
		fprintf(pointer, "\n");
		fclose(pointer);
	}
}

//intialize the conditional variable and mutex lock
void init(protect *s, int value) {
	s->value = value;
	pthread_cond_init(&s->cond, NULL);
	int rc = pthread_mutex_init(&s->lock, NULL);
}

//waits for a condition to become true before executing 
void protectWait(protect *s) {
	pthread_mutex_lock(&s->lock);
	while (s->value <= 0)
		pthread_cond_wait(&s->cond, &s->lock);
	s->value--;
	pthread_mutex_unlock(&s->lock);
}

//singlas a thread that something has finished executing 
void protectPost(protect *s) {
	pthread_mutex_lock(&s->lock);
	s->value++;
	pthread_cond_signal(&s->cond);
	pthread_mutex_unlock(&s->lock);
}

//adds a socket descriptor to the work Queue with mutual exclusion and mutex
void *addWork(int socketDesc){
	protectWait(&empty1);
	protectWait(&mutex1);
	putWork(socketDesc);
	protectPost(&mutex1);
	protectPost(&full1);
}

//removies a socket descriptor from work queue and returns it
int consumeWork() {
	protectWait(&full1);
	protectWait(&mutex1);
	int tmp = getWork();
	protectPost(&mutex1);
	protectPost(&empty1);
	return tmp;
}

//adds a message to the loq Queue with mutual exclusion and mutex
void *addLog(char* clientMessage){
	protectWait(&empty2);
	protectWait(&mutex2);
	putLog(clientMessage);
	protectPost(&mutex2);
	protectPost(&full2);
}

//removies a message from work queue and returns it
char *consumeLog() {
	protectWait(&full2);
	protectWait(&mutex2);
	char* tmp = getLog();
	protectPost(&mutex2);
	protectPost(&empty2);
	return tmp;
}

//adds a socket descriptor to the work Queue
void putWork(int value) {
	workQueue[workQueEnd] = malloc(1);
	*workQueue[workQueEnd] = value;
	workQueEnd = (workQueEnd + 1) % MAX;
}

//adds a message to the log Queue
void putLog(char* clientMessage) {
	logQueue[logQueEnd] = (char*)malloc(sizeof(char*)*200);
	logQueue[logQueEnd] = clientMessage;
	logQueEnd = (logQueEnd + 1) % MAX;
}

//gets a socket descriptor from the work queue 
int getWork() {
	int tmp = *workQueue[workQueStart];
	workQueStart = (workQueStart + 1) % MAX;
	return tmp;
}

//gets a message from the log queue 
char* getLog() {
	char* tmp = logQueue[logQueStart];
	logQueStart = (logQueStart + 1) % MAX;
	return tmp;
}

//handles the process of getting a message from log queue and writing it to a log file 
void *logThreadHandler(){
	while(1){
		while (logQueStart != logQueEnd){
			char *temp = consumeLog();
			writeToFile(temp, "logFile.txt");
		}
	}
	return 0;
}

