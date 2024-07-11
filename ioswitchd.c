
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


int add_device(Server *server, CtlCommand command, int socketfd) {
    int pid = fork();
    if (pid < 0) {
        perror("Error creating fork");
        return -1;
    }

    if (pid == 0) {
        printf("forked\n");
        close(server->fd);
        create_input_sender(socketfd, command.device, command.ip, command.port);
        exit(1);
    } else {
        close(socketfd);
    }
    return 0;
}

int process_command(Server *server, int socketfd) {
    CtlCommand command = { 0 };
    ssize_t r = read(socketfd, &command, sizeof(struct CtlCommand));
    if (r < 0) {
        fprintf(stderr, "Error reading command packet");
        return -1;
    }

    // WHY DOES THIS WORK LIKE THIS.
    // THIS IS NEEDED SO THAT THE SOCKET PROPERLY
    fflush(stdout);

    switch (command.type) {
        case CTL_LIST:
            break;
        case CTL_ADD_DEVICE:
            add_device(server, command, socketfd);
            break;
        case CTL_RM_DEVICE:
            break;
        case CTL_DISABLE:
            break;
        case CTL_ENABLE:
            break;
        case CTL_KILL:
            return 1;
            break;
        case CTL_NONE:
            break;
        default:
            break;
    }
    return 0;
}

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
        if (listen(server.fd, 5) < 0) {
            perror("Error listening for connections");
            return -1;
        }
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
        fflush(stdout);
        int pid;
        switch (type) {
            case INPUT:
                pid = fork();
                fflush(stdout);
                if (pid < 0) {
                    perror("Error creating fork");
                    return -1;
                }

                if (pid == 0) {
                    close(server.fd);
                    create_input_listener(newsockfd);
                    return 0;
                } else {
                    printf("Server: New client connected\n");
                    close(newsockfd);
                }
                break;
            case MESSAGE:
                printf("Server: received ctl message\n");

                int res = process_command(&server, newsockfd);
                if (res == 1) {
                    running = 0;
                }

                close(newsockfd);
                break;
            default:
                fprintf(stderr, "Error Unknown type Defaulted\n");
                close(newsockfd);
                break;
        }

    }

    close(server.fd);
    printf("exited\n");
    return 0;
}
