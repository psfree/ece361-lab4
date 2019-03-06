all: chatclient.c chatserver.c
	gcc -g -o chatclient chatclient.c
	gcc -g -o chatserver chatserver.c
clean: 
	$(RM) chatclient
	$(RM) chatserver
