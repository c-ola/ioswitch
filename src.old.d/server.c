#include "server.h"
#include "client.h"
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "ctl.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <arpa/inet.h>
    #include <linux/input-event-codes.h>
    #include <netinet/in.h>
#endif


Server* create_server() {
    Server* server = malloc(sizeof(Server));
    server->ctl_handlers[CTL_NONE] = none_command;
    server->ctl_handlers[CTL_LIST] = list_devices;
    server->ctl_handlers[CTL_RM_DEVICE] = rm_device;
    server->ctl_handlers[CTL_ADD_DEVICE] = add_device;

    char config_path[256];
    strcat(strcpy(config_path, getenv("HOME")), "/.config/ioswitch/config");
    printf("Config: %s\n", config_path);

    server->config = load_tokens(config_path);
    server->listener_mutex = malloc(sizeof(pthread_mutex_t));
    server->sender_mutex = malloc(sizeof(pthread_mutex_t));
    *server->listener_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    *server->sender_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    server->sender_flags = malloc(MAX_SENDERS * sizeof(int));
    server->listen_flags = malloc(MAX_LISTENERS * sizeof(int));
    memset(server->listen_flags, 0, MAX_LISTENERS * sizeof(int));
    memset(server->sender_flags, 0, MAX_SENDERS * sizeof(int));
    return server;
}

void destroy_server(Server *server) {
    pthread_mutex_lock(server->listener_mutex);
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (server->listen_flags[i] != 0) {
            pthread_cancel(server->listen_handles[i]);
        }
    }
    pthread_mutex_unlock(server->listener_mutex);
    
    free(server->sender_mutex);
    free(server->sender_flags);
    free(server->listener_mutex);
    free(server->listen_flags);

    CLOSESOCK(server->fd);
    free(server);
}

int start_server(Server *server, int port) {

    CREATE_SOCKET(server->fd);
    server->addrlen = sizeof(struct sockaddr_in);

    // Forcefully attaching socket to the port PORT
    // What does this actually do??
    server->opt = 0;
#ifdef __unix__
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &server->opt, sizeof(server->opt)))
#else
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&server->opt, sizeof(int)))
#endif
    {
        perror("setsockopt");
        return -1;
    }

    server->address.sin_family = AF_INET;
    //server->address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server->address.sin_addr.s_addr = htons(INADDR_ANY);
    server->address.sin_port = htons(port);

    if (bind(server->fd, (struct sockaddr *)&server->address, server->addrlen) < 0) {
        perror("binding to port failed");
        return -1;
    }

    return 0;
}

int process_command(Server *server, int socketfd) {
    CtlCommand command;
    ssize_t r = recv(socketfd, (char*)&command, sizeof(struct CtlCommand), 0);
    if (r < 0) {
        fprintf(stderr, "Error reading command packet");
        return -1;
    }
    usleep(1000);
    printf("Received command %s\n", ctltostr[command.type]);
    if (command.type == CTL_KILL) return 1;
    server->ctl_handlers[command.type](server, &command);
    return 0;
}

int none_command(Server *server, CtlCommand* command) {
    return 0;
}

int list_devices(Server *server, CtlCommand* command) {
    printf("\n");
    printf("Senders:\n");
    pthread_mutex_lock(server->sender_mutex);
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (server->sender_flags[i] != 0) {
            printf("id: %d, name: %s\n", server->sender_flags[i], server->sender_names[i]);
        }
    }
    pthread_mutex_unlock(server->sender_mutex);

    int num_listeners = 0;
    pthread_mutex_lock(server->listener_mutex);
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (server->listen_flags[i] != 0) {
            num_listeners++;
        }
    }
    pthread_mutex_unlock(server->listener_mutex);
    printf("There are %d listeners\n", num_listeners);

    printf("\n");
    return 0;
}

int add_device(Server *server, CtlCommand* command) {
    DeviceName* name = &command->name;
    ConnInfo* conn = &command->conn;

    int dfd = open(name->device, O_RDONLY | O_NONBLOCK);
    if (dfd < 0) {
        fprintf(stderr, "error unable to open for reading %s\n", name->device);
        perror("Error:");
        pthread_exit(NULL);
    }
    printf("Opened device: %s\n", name->device);

    printf("Creating sender to %s:%d\n", conn->ip, conn->port);
    // Create the file descriptor
    int cfd;
    if ((cfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error");
        return -1;
    }

    // connect to the server
    if (connect_to_server(cfd, conn->ip, conn->port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    SocketType type = INPUT_CONN;
    if (send(cfd, &type, sizeof(SocketType), 0) < 0) {
        perror("Error Sending Conn Packet");
        return -1;
    }

    printf("Connected to server\n");
    
    // Find open slot
    int slot;
    pthread_mutex_lock(server->sender_mutex);
    for (slot = 0; slot < MAX_SENDERS; slot++) {
        if (server->sender_flags[slot] == 0) {
            break;
        }
    }
    if (slot == MAX_SENDERS) {
        printf("At Max Capacity for Senders (%d)\n", MAX_SENDERS);
        return -1;
    }
    server->sender_flags[slot] = slot + 1;
    pthread_mutex_unlock(server->sender_mutex);
    
    strncpy(server->sender_names[slot], name->device, MAX_NAME_LENGTH);
    // Create the sender thread
    SenderArgs* args = malloc(sizeof(SenderArgs));
    args->dfd = dfd;
    args->cfd = cfd;
    
    Values* binds_toks = get_variable(server->config, "key_binds");
    args->num_keys = binds_toks->num_values;
    args->keybind = malloc(args->num_keys * sizeof(int));
    for (int i = 0; i < args->num_keys; i++) {
        args->keybind[i] = str_to_key(binds_toks->tokens[i].val);
    }
    free(binds_toks);

    args->flag = &server->sender_flags[slot];
    args->mut_ptr = server->sender_mutex;

    pthread_create(&server->sender_handles[slot], NULL, &spawn_sender_thread, (void*) args);
    pthread_detach(server->sender_handles[slot]);
    printf("Added sender for device %s\n", name->device);

    return 0;
}

int rm_device(Server *server, CtlCommand* command) {
    DeviceName* dev_name = &command->name;
    pthread_mutex_lock(server->sender_mutex);
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (server->sender_flags[i] != 0) {
            char *name = server->sender_names[i];
            if (strcmp(name, dev_name->device) == 0) {
                printf("Removing device %s with id %d\n", name, i);
                server->sender_flags[i] = 0;
                pthread_mutex_unlock(server->sender_mutex);
                return 0;
            }
        }
    }
    pthread_mutex_unlock(server->sender_mutex);
    printf("Could not find specified device\n");
    return 0;
}

// creates a new input device on the servers system that listens from socketfd
// ALSO ABSTRACT AWAY THE INPUT DEVICE
void* spawn_listener_thread(void* listener_args) {
    ListenerArgs* args = (ListenerArgs*) listener_args;
    int socketfd = args->newsockfd;
    pthread_mutex_t* mutex = args->mut_ptr;
    int* flag = args->flag;

    Listener* listener = create_listenera(args);

    struct input_event ie = {0};
    while (1) {
        ssize_t read_bytes = read(socketfd, &ie, sizeof(struct input_event));
        if (read_bytes == sizeof(struct input_event)) {
            write(listener->virtdevfd, &ie, sizeof(struct input_event));
        } else if (read_bytes <= 0) {
            break;
        }
    }

    pthread_mutex_lock(mutex);
    *flag = 0;
    pthread_mutex_unlock(mutex);

    destroy_listener(listener);
    free(listener_args);

    pthread_exit(NULL);
}

void* spawn_sender_thread(void* sender_args) {
    SenderArgs* args = (SenderArgs*) sender_args;
    Sender* sender = create_sendera(args);

    printf("started sending...\n");
    int connected = 1;
    int unbound = 0;
    while (connected) {
        int res = send_input_event(sender);
        if (res == 1) {
            printf("Received keybind inputs\n");
            connected = 0;
            unbound = 1;
        }

        pthread_mutex_lock(args->mut_ptr);
        if (*(args->flag) == 0) {
            connected = 0;
        }
        pthread_mutex_unlock(args->mut_ptr);

        if (res < 0) {
            fprintf(stderr, "Error occured, closing\n");
            connected = 0;
        }
    }

    printf("stopped device\n");

    if (unbound) {
        printf("\nrunning stop script\n");
        system("ioswitchstop");
    };

    destroy_sender(sender);
    free(sender_args);

    pthread_exit(NULL);
}
