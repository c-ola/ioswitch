#ifndef SERVER_H
#define SERVER_H

#include "ctl.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#ifdef __unix__
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <linux/uinput.h>
    #include <pthread.h>
#else
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif


#define MAX_SENDERS 4
#define MAX_LISTENERS 8
#define LOCAL_PORT 8080
#define MAX_NAME_LENGTH 80

typedef enum {
    UNKNOWN,
    INPUT_CONN,
    MESSAGE,
} SocketType;


typedef struct Server Server;

typedef int (*CtlCommHandler)(Server* server, CtlCommand* command);

typedef struct Server {
    int fd;
    int opt;
    struct sockaddr_in address;
    socklen_t addrlen;
    
    int* listen_flags; // shared array of flags
    pthread_mutex_t* listener_mutex;
    pthread_t listen_handles[MAX_LISTENERS];
    
    int* sender_flags; // shared array of flags
    char sender_names[MAX_SENDERS][MAX_NAME_LENGTH];
    pthread_mutex_t* sender_mutex;
    pthread_t sender_handles[MAX_SENDERS];

    CtlCommHandler ctl_handlers[CTL_ENUM_LENGTH];
} Server;

int start_server(Server* server, int port);
int process_command(Server* server, int socketfd);

int none_command(Server *server, CtlCommand* command);
int list_devices(Server* server, CtlCommand* command);
int rm_device(Server* server, CtlCommand* command);
int add_device(Server* server, CtlCommand* command);
//int add_binding(Server *server, CtlCommand* command);

void* spawn_listener_thread(void* listener_args);
void* spawn_sender_thread(void* sender_args);


#endif
