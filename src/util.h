#pragma once
#include <stddef.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <limits.h>
#include <string.h>
#include <crypt.h>
#include <unistd.h>
#include <values.h>

#include <linux/input-event-codes.h>
#define INFO(...) printf("INFO: "); printf(__VA_ARGS__);
#define ERROR(...) printf("ERROR: "); fprintf(stderr, __VA_ARGS__);

// Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);

int connect_to_server(int fd, const char *ip, unsigned int port);
int send_input_event(int id, struct pollfd* devfd, int dest_sock_fd);

typedef struct KeyBind {
    KeySym keys[8];
    int len;
    int idx;
} KeyBind;

typedef struct DisplayLock {
    Display* display;
    Window window;
    int locked;
} DisplayLock;

DisplayLock* lock();
int try_unlock(DisplayLock* disp_lock, KeyBind* bind);
void* spawn_lock_thread(void* args);
