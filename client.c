// Client side C program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "linux/input-event-codes.h"
#include <linux/uinput.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8080


typedef struct _client {
    int fd;
    int status;
} Client;

int connect_to_server(Client client, struct sockaddr_in serv_addr) {

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "192.168.2.37", &serv_addr.sin_addr)<= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client.status = connect(client.fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return 0;
}

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   send(fd, &ie, sizeof(ie), 0);
}

int main(int argc, char const* argv[]) {
    struct sockaddr_in serv_addr;
    
    Client client = { 0 };
    if ((client.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (connect_to_server(client, serv_addr) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    // set up for reading from /dev/input/eventX
    struct pollfd fds[1];
    const char* device = argv[1];
    fds[0].fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fds[0].fd < 0) {
        printf("error unable to open for reading %s\n", device);
        return -1;
    }
    fds[0].events = POLLIN;
    struct input_event* input_data = malloc(sizeof(struct input_event));

    int status, connected = 1;
    char buffer[1024] = { 0 };
    printf("started sending...\n");
    while (connected) {
        // timeout should probably be a couple of milliseconds to not block everything
        int ret = poll(fds, 1, -1);
        if (ret < 0) {
            printf("timeout\n");
            continue;
        }
        if (!fds[0].revents) {
            printf("error somehow\n");
            continue;
        }
        ssize_t r = read(fds[0].fd, (void*)input_data, sizeof(struct input_event));
        if (r < 0) {
            perror("error reading device\n");
            return -1;
        }

        if ((status = send(client.fd, input_data, sizeof(struct input_event), 0)) <= 0) {
            printf("failed to send message\n");
            connected = 0;
        }
    }

    // closing the connected socket
    close(client.fd);
    return 0;
}

