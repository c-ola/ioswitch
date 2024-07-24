
#include "ctl.h"
#include "server.h"
#include "common.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>


#ifdef __linux__
    #include <linux/input.h>
    #include <linux/uinput.h>
#endif

#ifdef __unix__
    #include <pthread.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/poll.h>
    #include <sys/socket.h>
#else
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

int main (int argc, char** argv) {
    int port = LOCAL_PORT;
    opterr = 0;
    int opt;
#ifdef __unix__
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
#endif

    // Server that handles receiving input and emitting to the computer
    // Can also receive messages that tell it to start a new sender
    Server server = { 0 };
    server.ctl_handlers[CTL_NONE] = none_command;
    server.ctl_handlers[CTL_LIST] = list_devices;
    server.ctl_handlers[CTL_RM_DEVICE] = rm_device;
    server.ctl_handlers[CTL_ADD_DEVICE] = add_device;
    //server.ctl_handlers[CTL_ADD_BINDING] = add_binding;
    server.listener_mutex = malloc(sizeof(pthread_mutex_t));
    server.sender_mutex = malloc(sizeof(pthread_mutex_t));
    *server.listener_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    *server.sender_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    server.sender_flags = malloc(MAX_SENDERS * sizeof(int));
    server.listen_flags = malloc(MAX_LISTENERS * sizeof(int));

    memset(server.listen_flags, 0, MAX_LISTENERS * sizeof(int));
    memset(server.sender_flags, 0, MAX_SENDERS * sizeof(int));
    
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
        ssize_t r = recv(newsockfd, (char*)&type, sizeof(SocketType), 0);
        if (r < 0) {
            close(newsockfd);
            perror("Error reading initialization packet");
            continue;
        }
        const char hs[] = "good";
        r = send(newsockfd, hs, sizeof(hs), 0);

        switch (type) {
            case INPUT_CONN:
#ifdef __unix__
                {
                    int num_listeners = 0;
                    for (int i = 0; i < MAX_LISTENERS; i++) {
                        if (server.listen_flags[i] == 0) {
                            printf("Server: New client connected\n");
                            ListenerArgs* args = malloc(sizeof(ListenerArgs));
                            args->newsockfd = newsockfd;
                            args->flag = &server.listen_flags[i];
                            args->mut_ptr = server.listener_mutex;

                            pthread_mutex_lock(server.listener_mutex);
                            server.listen_flags[i] = 1;
                            pthread_mutex_unlock(server.listener_mutex);
                            pthread_create(&server.listen_handles[i], NULL, &spawn_listener_thread, (void*)args);
                            pthread_detach(server.listen_handles[i]);
                            printf("Created new input listener\n");
                            break;
                        } else {
                            num_listeners++;
                        }
                    }
                    if (num_listeners == MAX_LISTENERS) {
                        printf("Listeners at Max Capacity\n");
                    }
                }
#endif
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
            case UNKNOWN:
                fprintf(stderr, "\nError Unknown Conn Type\n");
                close(newsockfd);
                break;
        }

    }

    pthread_mutex_lock(server.listener_mutex);
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (server.listen_flags[i] != 0) {
            pthread_cancel(server.listen_handles[i]);
        }
    }
    pthread_mutex_unlock(server.listener_mutex);
    
    free(server.sender_mutex);
    free(server.sender_flags);
    free(server.listener_mutex);
    free(server.listen_flags);

    CLOSESOCK(server.fd);
    printf("exited\n");
    return 0;
}
