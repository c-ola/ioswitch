#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#ifdef __linux__
#include <linux/uinput.h>
#elif defined(WIN32) || defined(WIN64)
#endif

#ifdef __unix__
    #include <sys/poll.h>
#elif defined(WIN32) || defined(WIN64)
    #include <winsock2.h>
#endif

#ifdef __unix__
typedef struct pollfd device_reader;
#elif defined(WIN32) || defined(WIN64)
typedef int device_reader;
#endif

typedef struct _Sender {
    int sockfd;
    device_reader devfd;
    int* keybind;
    int* keybind_buf;
    int num_keys;
} Sender;

Sender* create_senderv(int sockfd, device_reader devfd, int* keybind, int num_keys);
Sender* create_sendera(SenderArgs* args);
void destroy_sender(Sender* sender);

int connect_to_server(int fd, const char *ip, unsigned int port);
int send_input_event(Sender* sender);

#ifdef __linux__
typedef struct uinput_setup vdev_setup;
#elif defined(WIN32) || defined(WIN64)
typedef int vdevdev_setup;
#endif

typedef struct _Listener {
    int sockfd;
    int virtdevfd;
    vdev_setup dev_setup;
} Listener;

Listener* create_listenera(ListenerArgs* args);
void destroy_listener(Listener* listener);

#endif
