#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include "ctl.h"
#include <linux/uinput.h>

#define MAX_SENDERS 4
#define LOCAL_PORT 8080
#define MAX_NAME_LENGTH 80

typedef enum {
    UNKNOWN,
    INPUT,
    MESSAGE,
} SocketType;

typedef struct Sender {
    int pid;
    char name[MAX_NAME_LENGTH];
} Sender;


typedef struct Server Server;

typedef int (*CtlCommHandler)(Server* server, CtlCommand command);

typedef struct Server {
    int fd;
    int opt;
    struct sockaddr_in address;
    socklen_t addrlen;
    int num_senders;
    int sender_ids[MAX_SENDERS];
    char sender_names[MAX_SENDERS][MAX_NAME_LENGTH];
    //int (*ctl_handlers[CTL_ENUM_LENGTH])(struct Server* server, CtlCommand command); 
    CtlCommHandler ctl_handlers[CTL_ENUM_LENGTH];
} Server;

int start_server(Server* server, int port);
int process_command(Server* server, int socketfd);

int create_input_listener(int socketfd);
int list_devices(Server* server, CtlCommand command);
int rm_device(Server* server, CtlCommand command);
int add_device(Server* server, CtlCommand command);
int add_binding(Server *server, CtlCommand command);

int create_input_sender(char* device, char* ip, unsigned int port);
int create_input_binding(char *device, char *ip, unsigned int port);

#endif
