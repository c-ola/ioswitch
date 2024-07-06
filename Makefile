CC = gcc

all: server client

dir:
	mkdir -p build

server: server.c dir
	$(CC) server.c -o build/server

client: client.c dir
	$(CC) client.c -o build/client

clean:
	rm server client
