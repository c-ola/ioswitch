CC = gcc
FLAGS = -g -Wall

all: server client

server: server.c
	mkdir -p build
	$(CC) $(FLAGS) server.c -o build/server

client: client.c
	mkdir -p build
	$(CC) $(FLAGS) client.c -o build/client

clean:
	rm build/server build/client
