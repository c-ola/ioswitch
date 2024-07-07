#include "server.h"
#include <asm-generic/socket.h>
#include <complex.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/uinput.h>

#include <linux/input-event-codes.h>

#define PORT 8080


struct virt_device {
    int fd;
};

int main(void) {

   
    int server_fd; // server file descriptor
    int new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024];

    Server server = { 0 };
    if (start_server(&server, PORT) < 0) {
        fprintf(stderr, "Could not start Server\n");
    }
    printf("Started Server\n");

    int running = 1;
    
    if (listen(server.fd, 3) < 0) {
        perror("Error listening for connections");
        return -1;
    }

    while (1) {
        int newsockfd;
        if ((newsockfd = accept(server.fd, (struct sockaddr*)&server.address, &server.addrlen)) < 0) {
            perror("accept");
            return -1;
        }

        int pid = fork();
        fflush(stdout);
        if (pid < 0) {
            perror("Error creating fork");
            return -1;
        }

        if (pid == 0) {
            printf("New client connected\n");
            close(server.fd);
            process_input(newsockfd);
            return 0;
        } else {
            close(newsockfd);
        }
    }

    close(server.fd);

    return 0;
}
