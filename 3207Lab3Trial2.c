
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <dirent.h>
#include <unistd.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <pthread.h>
#include <unistd.h>
#include <ctype.h>


char* readFromfile(char* nameOfFile);
int isNumber(char *number);

//dictionary 
char *dict;
char* DEFAULT_DICTIONARY = "dictionary.txt";

//saves the socket descriptor of the clientes that connect to it 
char *allDocketDesc[1000000];
int socketDescStart = 0;
int socketDescEnd = 0;

// buffer where the producer which is the main thread adding socket descriptors () to the queue
char *workQueue[1000000];
int workQueStart = 0;
int workQueEnd = 0;

// A log queue where logs are written to it in different places and then the server log main thread will remove it from the queue and write it to a file 
char *logQueue[1000000];
int logQueStart = 0;
int logQueEnd = 0;

int portNumber;
int DEFAULT_PORT = 8888;
int main(int argc, char *argv[]){ 
	
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
	
	
	
	//create worker threads 
	
	//main thread (Accept and dirtribute connection requests)
	
	//while (true) { 
		//connected_socket = accept(listening_socket); 
		//add connected_socket to the work queue; 
		//signal any sleeping workers that there's a new socket in the queue; 
	//}
	
	//second worker: monitor log queue (write and remove enteties into log file)
	
	/*while (true) {
		while (the log queue is NOT empty) {
			remove an entry from the log
			write the entry to the log file
		}
	}*/
	
	
	//worker thread////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//while (true) { 
	//	while (the work queue is NOT empty) { 
		//	remove a socket from the queue 
		//	notify that there's an empty spot in the queue 
		//	service client 
		//close socket 
		//} 
	//}
	
	//client servicing logic
	
	//while (there's a word left to read) { 
		//read word from the socket 
		//if (the word is in the dictionary) { 
			//echo the word back on the socket concatenated with "OK"; 
		//} 
		//else { 
			//echo the word back on the socket concatenated with "MISSPELLED"; 
		//}
		//write the word and the socket response value (“OK” or “MISSPELLED”) to the log queue; 
	//}
	
	return 0;
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

int isNumber(char* number){

    int i = 0;

    for (; number[i] != strlen(number); i++){
        //if (number[i] > '9' || number[i] < '0')
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}