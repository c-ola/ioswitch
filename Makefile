CC = gcc
FLAGS = -g -Wall
DAEMON_SRC_FILES = src/client.c src/server.c src/ctl.c
CTL_SRC_FILES = src/client.c src/ctl.c

all: daemon ctl

dir:
	mkdir -p build

daemon: src/ioswitchd.c dir
	$(CC) $(FLAGS) src/ioswitchd.c $(DAEMON_SRC_FILES) -o build/ioswitchd

ctl: src/ioswitchctl.c dir
	$(CC) $(FLAGS) src/ioswitchctl.c $(CTL_SRC_FILES) -o build/ioswitchctl

clean:
	rm build/ioswitchd build/ioswitchctl
