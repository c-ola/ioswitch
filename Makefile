CC = gcc
FLAGS := -g -Wall
PLATFORM?=UNIX
ifeq ($(PLATFORM), WIN)
	LIBS := -lws2_32
else
	LIBS := -lpthread
endif

TARGET?=DEBUG
ifeq ($(TARGET), RELEASE)
	FLAGS := $(FLAGS) -O3
else
	FLAGS := $(FLAGS) -fsanitize=address -fno-omit-frame-pointer -O1
endif

DAEMON_SRC_FILES = src/client.c src/server.c src/ctl.c src/common.c src/tokenizer/tokenizer.c
CTL_SRC_FILES = src/client.c src/ctl.c src/common.c src/tokenizer/tokenizer.c

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
