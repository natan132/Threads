#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
 
#include<pthread.h> //for threading , link with lpthread
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
void logThreadHandler();

int portNumber;
int DEFAULT_PORT = 8888;
int WORKER_COUNT = 3;
int MAX = 10;


//dictionary 
char *dict;
char* DEFAULT_DICTIONARY = "dictionary.txt";

//saves the socket descriptor of the clientes that connect to it 
int *allDocketDesc[1000000];
int socketDescStart = 0;
int socketDescEnd = 0;

// buffer where the producer which is the main thread adding socket descriptors () to the queue
int *workQueue[MAX];
int workQueStart = 0;
int workQueEnd = 0;

// A log queue where logs are written to it in different places and then the server log main thread will remove it from the queue and write it to a file 
char *logQueue[1000000];
int logQueStart = 0;
int logQueEnd = 0;

protect lock1;
protect lock2;

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
	init(&lock1, 0, 1);
	init(&lock2, 0, 1);
	init(&empty1, 0, MAX);
	init(&empty2, 0, MAX);
	init(&full1, 0, 0);
	init(&full2, 0, 0);
	
	//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	//Pthread_mutex_lock(&lock);
	//Pthread_mutex_unlock(&lock);
	
	//pthread_cond_t c = PTHREAD_COND_INITIALIZER;
	//pthread_cond_init(&name, NULL);
	//pthread_cond_wait(&name, &mutexName);
	//pthread_cond_signal(&name);

	
	 //get arguments form the command line 
	 if(argc > 1){
		int i = 1;
		while (i < argc){
			if(isNumber(argv[i])){			
				portNumber = atoi(argv[i]);
				printf("%d\n",portNumber);
			}
			else{
				dict = argv[i];
				printf("%s\n",dict);
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
        pthread_t sniffer_thread;
        //new_sock = malloc(1);
		//workQueue[workQueEnd] = malloc(1);
        //*new_sock = new_socket;
		//*workQueue[workQueEnd++] = new_socket;
		addWork(new_socket);
		
		//put(new_socket);
		for(i = 0; i < WORKER_COUNT; i++){
			threadIDs[i] = i;
			//Start running the threads.
			if( pthread_create(&threadPool[i], NULL, connection_handler, NULL) < 0){
				perror("could not create thread");
				return 1;
			}
		}
		pthread_t logThread;
		if( pthread_create(&logThread, NULL, logThreadHandler, NULL) < 0){
				perror("could not create thread");
				return 1;
			}
        //consumeWork(); 
		//consume work 
        /*if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0){
            perror("could not create thread");
            return 1;
        }*/
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
     
    if (new_socket<0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(){
    //Get the socket descriptor
    
	/*int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];*/
	int sock = consumeWork();
     
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
        //Send the message back to client
        //write(sock , client_message , strlen(client_message));
		
		//log the word sent to the log file over here 
		addLog(client_message);
		
		//this is where the code goes to search for the word in the string goes 
		char* result = strstr(dict, client_message);
		if(result != NULL){
			strcat(client_message, "Ok! \n\n");
			addLog(client_message);
			write(sock, client_message, strlen(client_message));
			//write(sock, strlen(client_message), strlen(client_message));
		}
		else{
			strcat(client_message, "Misspelled! \n\n");
			addLog(client_message);
			write(sock, client_message ,strlen(client_message));
		}
		//empty the string 
		memset(client_message,0,strlen(client_message));
    }
     
    if(read_size == 0){
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1){
        perror("recv failed");
    }
         
    //Free the socket pointer
    free(socket_desc);
     
    return 0;
}

//checks if the parameter given is a number or not 
int isNumber(char* number){

    int i = 0;

    for (; number[i] != strlen(number); i++){
        //if (number[i] > '9' || number[i] < '0')
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
	Cond_init(&s->cond);
	Mutex_init(&s->lock);
}

//waits for a condition to become true before executing 
void protectWait(protect *s) {
	Mutex_lock(&s->lock);
	while (s->value <= 0)
		Cond_wait(&s->cond, &s->lock);
	s->value--;
	Mutex_unlock(&s->lock);
}

//singlas a thread that something has finished executing 
void protectPost(protect *s) {
	Mutex_lock(&s->lock);
	s->value++;
	Cond_signal(&s->cond);
	Mutex_unlock(&s->lock);
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
	protectWait(&ful2);
	protectWait(&mute2);
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
	//workQueEnd++;
}

//adds a message to the log Queue
void putLog(char* clientMessage) {
	logQueue[logQueEnd] = (char*)malloc(sizeof(char*)*200);
	*logQueue[logQueEnd] = clientMessage;
	logQueEnd = (logQueEnd + 1) % MAX;
	//workQueEnd++;
}

//gets a socket descriptor from the work queue 
int getWork() {
	int tmp = workQueue[workQueStart];
	workQueStart = (workQueStart + 1) % MAX;
	//workQueStart++;
	return tmp;
}

//gets a message from the log queue 
char* getLog() {
	char* tmp = logQueue[logQueStart];
	logQueStart = (logQueStart + 1) % MAX;
	//workQueStart++;
	return tmp;
}

//handles the process of getting a message from log queue and writing it to a log file 
void logThreadHandler(){
	while(1){
		while (logQueStart != logQueEnd){
			char *temp = consumeLog();
			writeToFile(temp, "logFile.txt");
		}
	}
}

