CC = gcc
FLAGS = -g

all: server client

dir:
	mkdir -p build

server: server.c dir
	$(CC) $(FLAGS) server.c -o build/server

client: client.c dir
	$(CC) $(FLAGS) client.c -o build/client

clean:
	rm server client
