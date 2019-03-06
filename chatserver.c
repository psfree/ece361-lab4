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

#define MAX_CONN 50

typedef struct {
	int sd;
	char * username;
} connection;

int main(int argc, char *argv[]) {

	//get port argument from command line
	if(argc<2) {
		printf("./chatserver [server_port]\n");
		return -1;
	}
	
	char * port = argv[1];
	int server_port = atoi(port);
	if((server_port <1) || (server_port >65535)) {
		printf("Invalid port number %d\n", server_port);
		return -1;
	}
	
	//create an array of clients and initialize
	int sockfd;
	connection clients[MAX_CONN];
	for (int i = 0; i < MAX_CONN; i++)   
    {   
        clients[i].sd = 0;   
        clients[i].username = NULL;
    }
    
    //create the socket, set options, the bind and listen
	struct sockaddr_in addr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    	printf("Unable to make socket\n");
        perror("server:"); 
        return -1;
    } 
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
    	printf("error setting socket options\n");
    	return -1;
    }
    
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(server_port);
    
    if (bind(sockfd, (struct sockaddr *)&addr,  sizeof(addr))<0) {
    	printf("bind error\n");
    	return -1;
    }
    
    if (listen(sockfd, 3) < 0) { 
        printf("listen error \n");
       	return -1;
    } 
    
    int addrsz = sizeof(addr);
    fd_set readfds;   

    
    //the main loop of the server. it checks for new connections and handles incoming messages
    while(1) {
    
    	//set up data and use select() to handle multiple clients
    	FD_ZERO(&readfds);
    	FD_SET(sockfd, &readfds);
    	int max_sd = sockfd;
    	
    	for(int i=0; i< MAX_CONN; i++){
    		int cursock = clients[i].sd;
    		if(cursock>0) {
    			FD_SET(cursock, &readfds);
    		}
    		if(cursock> max_sd) {
    			max_sd = cursock;
    		}
    	}
    	
    	int activity = select(max_sd +1, &readfds, NULL,NULL,NULL);
    	
    	if(activity <0) {
    		printf("select() error\n");
    		return -1;
    	}
    	
    	//a new client is connecting, populate data structure and add it to array
    	if (FD_ISSET(sockfd, &readfds)) {
    		int newsock;
    		if((newsock = accept(sockfd, (struct sockaddr*)&addr, (socklen_t*)&addrsz)) <0) {
    			printf("accept() error \n");
    			return -1;
    		}
    		printf("New connection , socket fd is %d , ip is : %s , port : %d  \n" , newsock , inet_ntoa(addr.sin_addr) , ntohs (addr.sin_port));
    		
    		//get the username sent by the client
    		char uname[50];
    		int valread = read(newsock , uname, 50);
			uname[valread] = 0;
			
			printf("username joined: %s\n", uname);
			
			//broadcast newuser to all clients
			for(int i=0; i< MAX_CONN; i++){
				int cursock = clients[i].sd;
				char buf[100];
				snprintf(buf, 100, "SERVER: %s joined!", uname);
				if(cursock>0) {
					if(send(cursock, buf, strlen(buf), 0) != strlen(buf) )   
            		{   
                		printf("error sending joining broadcast\n");
                		return -1;   
            		}  
				}
			}
			
			//send the welcome message and list of commands
    		char * message  = "Welcome to the ece361 chat zone!\n Commands: broadcast [msg], [username] [msg], list and exit\n";
            if(send(newsock, message, strlen(message), 0) != strlen(message) )   
            {   
                printf("error sending join message\n");
                return -1;   
            }  
            
            //find an open slot and add the data
            for(int i=0; i< MAX_CONN; i++) {
            	if(clients[i].sd==0) {
            		clients[i].sd = newsock;
            		clients[i].username = malloc(valread);
            		memcpy(clients[i].username, uname, valread);
            		break;
            	}
            }
    	}
    	
    	//this loop handles new messages sent by already connected clients
    	for(int i=0; i<MAX_CONN; i++) {
    	
    		int cursock = clients[i].sd;
    		char buffy[1024];
    		if (FD_ISSET(cursock , &readfds))  {
    			int ret = read(cursock, buffy,1024);
    			
    			if(ret ==0 ) { //disconnection
    				close(cursock);
    				clients[i].sd=0;
    				free(clients[i].username);
    				clients[i].username=NULL;
    			}
    			else {
    				buffy[ret]=0;
    				
    				//parse message for command and arguments
    				char command[25];
    				memset(command, 0,25);
    				char arg[200];
    				memset(arg, 0,200);
    				sscanf(buffy, "%s %s", command, arg);
    				if(strlen(command)<2)
    					continue;
    				memcpy(arg, buffy+strlen(command)+1, 200);
    				printf("msg: %s %s\n", command, arg);
    				
    				
    				//handle the various command types
    				if(strncmp(command, "broadcast", 10)==0) {
    					if(strlen(arg)>0) {
    						char bc[250];
    						snprintf(bc, 250, "%s: %s", clients[i].username, arg);
    						for(int i=0; i< MAX_CONN; i++){
    								int cursock = clients[i].sd;
    								if(cursock>0) {
    									if(send(cursock, bc, strlen(bc), 0) != strlen(bc) )   
    				            		{   
					                		printf("error sending broadcast message\n");
    						                return -1;   
    						        	}  
    								}
    						}

    					}
    				}
    				else if(strncmp(command, "list", 5)==0) {
    					char listbuf[1024];
    					memset(listbuf, 0, 1024);
    					for(int i=0; i< MAX_CONN; i++){
    						if(clients[i].sd > 0) {
    							snprintf(listbuf+strlen(listbuf), 1024, "%s\n", clients[i].username);
    						}
    					}
    					if(send(cursock, listbuf, strlen(listbuf), 0) != strlen(listbuf)) {
    						printf("error sending user list!\n");
    						return -1;
    					}
    				
    				}
    				else if(strncmp(command, "exit", 5)==0) {
    					close(cursock);
    					clients[i].sd=0;
    					free(clients[i].username);
    					clients[i].username=NULL;
    				}
    				else {  //this checks to see if the user is attempting a private msg
    					for(int j=0; i< MAX_CONN; i++){
    						if(clients[j].sd > 0) {
    							if(strncmp(clients[j].username, command, strlen(command))==0) {
    								char pm[250];
    								snprintf(pm, 250, "%s says: %s", clients[i].username, arg);
    								if(send(clients[j].sd, pm, strlen(pm), 0) != strlen(pm)) {
    									printf("error sending private message!\n");
    									return -1;
    								} 
    								break;
    							}
    						}
    						
    					}
    				
    				}
    			}
    			
    		}
    	
    	}

    }   
	return 0;
}