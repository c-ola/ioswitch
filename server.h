#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include "ctl.h"
#include <linux/uinput.h>
#include "client.h"

#define MAX_CONNS 4
#define LOCAL_PORT 8080

typedef enum {
    UNKNOWN,
    INPUT,
    MESSAGE,
} SocketType;

typedef struct server {
    int fd;
    int opt;
    struct sockaddr_in address;
    socklen_t addrlen;
} Server;

int start_server(Server* server, int port);

int create_input_listener(int socketfd);
int create_input_sender(int socketfd, char* device, char* ip, unsigned int port);

#endif
