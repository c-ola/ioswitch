CC = gcc
FLAGS = -g -Wall

all: daemon ctl

daemon: ioswitchd.c
	mkdir -p build
	$(CC) $(FLAGS) ioswitchd.c client.c server.c -o build/ioswitchd

ctl: ioswitchctl.c
	mkdir -p build
	$(CC) $(FLAGS) ioswitchctl.c client.c -o build/ioswitchctl

clean:
	rm build/ioswitchd build/ioswitchctl
