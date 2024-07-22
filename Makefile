CC = gcc
TARGET?=UNIX
FLAGS = -g -Wall

ifeq ($(TARGET), WIN)
LIBS = -lws2_32
endif
DAEMON_SRC_FILES = src/client.c src/server.c src/ctl.c
CTL_SRC_FILES = src/client.c src/ctl.c

all: daemon ctl

daemon: src/ioswitchd.c
	$(CC) $(FLAGS) src/ioswitchd.c $(DAEMON_SRC_FILES) -o build/ioswitchd $(LIBS)

ctl: src/ioswitchctl.c
	$(CC) $(FLAGS) src/ioswitchctl.c $(CTL_SRC_FILES) -o build/ioswitchctl $(LIBS)

install: all
	cp ioswitchrun /usr/local/bin/
	cp ioswitchstop /usr/local/bin/
	cp build/ioswitchctl /usr/local/bin/
	cp build/ioswitchd /usr/local/bin/

clean:
	rm build/ioswitchd build/ioswitchctl
