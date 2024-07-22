
#include "ctl.h"
#include "server.h"
#include "client.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main (int argc, char** argv) {
    int port = LOCAL_PORT;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                if (1)
                    continue;
                    //fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                return 1;
                /*if (isprint(optopt)) {
                  fprintf(stderr, "Unknown option -%c.\n", optopt);
                  }*/
        }
    }

    // Server that handles receiving input and emitting to the computer
    // Can also receive messages that tell it to start a new sender
    Server server = { 0 };
    server.ctl_handlers[CTL_NONE] = none_command;
    server.ctl_handlers[CTL_LIST] = list_devices;
    server.ctl_handlers[CTL_RM_DEVICE] = rm_device;
    server.ctl_handlers[CTL_ADD_DEVICE] = add_device;
    server.ctl_handlers[CTL_ADD_BINDING] = add_binding;
    
    if (start_server(&server, port)){
        fprintf(stderr, "Could not start Server\n");
        return -1;
    }

    printf("Started Server on port %d\n", port);

    if (listen(server.fd, 5) < 0) {
        perror("Error listening for connections");
        return -1;
    }

    int running = 1; 
    while (running) {
        int newsockfd;
        if ((newsockfd = accept(server.fd, (struct sockaddr*)&server.address, &server.addrlen)) < 0) {
            perror("accept");
            return -1;
        }
        
        // Get the first packet that tells the server how to handle the connection
        SocketType type = UNKNOWN;
        ssize_t r = read(newsockfd, &type, sizeof(SocketType));
        if (r < 0) {
            close(newsockfd);
            fprintf(stderr, "Error reading initialization packet");
            continue;
        }
        const char hs[] = "good";
        r = write(newsockfd, hs, sizeof(hs));
        int pid;

        switch (type) {
            case INPUT:
                pid = fork();
                if (pid < 0) {
                    perror("Error creating fork");
                    return -1;
                }

                if (pid == 0) {
                    close(server.fd);
                    printf("\n");
                    printf("Server: New client connected\n");
                    printf("Creating new input listener\n");
                    create_input_listener(newsockfd);
                    exit(1);
                    return 0;
                } else {
                    close(newsockfd);
                }
                break;
            case MESSAGE:
                printf("\n");
                printf("Server: received ctl message\n");
                int res = process_command(&server, newsockfd);
                if (res == 1) {
                    running = 0;
                }
                close(newsockfd);
                break;
            default:
                fprintf(stderr, "\nError Unknown Conn Type\n");
                close(newsockfd);
                printf("\n");
                break;
        }

    }

    close(server.fd);
    printf("exited\n");
    return 0;
}
