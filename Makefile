all: client.c server.c
	gcc -g -o client client.c
	gcc -g -o server server.c
clean: 
	$(RM) client
	$(RM) server
