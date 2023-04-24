all: server client

server: server.c
	gcc server.c -o server

client: tcp_client.c
	gcc tcp_client.c -o client

clean: 
	rm -f server client
