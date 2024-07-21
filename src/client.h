#ifndef CLIENT_H
#define CLIENT_H

#include <sys/poll.h>

typedef struct _client {
    int fd;
    int status;
    int* binds;
    int* binds_buf;
    int num_binds;
} Client;

int connect_to_server(Client client, const char *ip, unsigned int port);

// sends an input event from the specified device
// returns -1 on error
int send_input_event(struct pollfd pfd, Client client);

#endif
