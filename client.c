#define _GNU_SOURCE
#define GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>

typedef struct {
	int sockfd;
} threadstruct;

//this function is used by the secondary thread to receive messages from server
void * recvf(void * ts) {
	char buff[250];
	int sd = ((threadstruct *)ts)->sockfd;
	while(1) {
		memset(buff, 0, 250);
		int valread = read( sd , buff, 250);
		if(valread ==0) {
			close(sd);
    		printf("Server closed the connection!\n");
			exit(0);
		}
		buff[valread]=0;
		printf("\33[2K\r %s\n", buff);
	}
}

int main(int argc, char *argv[]) {
	//get arguments from command line and parse them
	if(argc<4) {
		printf("./chatclient [server_address] [server_port] [user_name]\n");
		return -1;
	}
	char * server_addr = argv[1];
	char * port = argv[2];
	char * username = argv[3];

	int server_port = atoi(port);
	if((server_port <1) || (server_port >65535)) {
		printf("Invalid port number %d\n", server_port);
		return -1;
	}
	
	printf("Connecting to %s:%d as %s\n", server_addr, server_port, username);
	
	//initialize relevant data structures
	int sockfd;
	struct addrinfo hints;
	struct addrinfo * serv;
	struct addrinfo *p;
	struct sockaddr_in addr;
	
	//dest: used to for getaddrinfo to populate struct
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_addr, &addr.sin_addr);

	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;        
	
	int ret = getaddrinfo(server_addr, port, &hints, &serv);
	if(ret!=0) {
		printf("Error getaddrinfo with dest %d\n", ret);
		perror("lol");
		return -1;
	}
	//iterate through the linked list but accept the first valid entry and create a socket
	for(p = serv; p!=NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        	    perror("client: socket");
        	    continue;
        	}
        	break;
	}
	//exit if no entry found
	if(p==NULL) {
		printf("Error with socket()\n");
		return -1;
	}
	
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)  { 
        printf("\nConnection Failed \n"); 
        return -1; 
    }
    
	//send the username to server
    char buffer[1024];
	send(sockfd , username , strlen(username) , 0 ); 
    printf("username sent\n"); 
    int valread = read( sockfd , buffer, 1024); 
    
    //start the receive thread
    threadstruct ts;
    ts.sockfd = sockfd;
    pthread_t thread;
	pthread_create(&thread, NULL, recvf, (void *)&ts);

    printf("%s\n",buffer ); 
    
    //main loop for getting input and sending to server
    while(1) {
    	char command[200];
    	memset(command, 0, 200);
    	printf(">");
    	fgets(command,200,stdin);
    	send(sockfd , command , strlen(command) , 0 );
    	usleep(200);
    }
	return 0;
}