#include "server.h"
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include "commands.h"
#include "tokenizer/tokenizer.h"
#include "util.h"

int start_server(Server* server, int port) {
    if ((server->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {\
        perror("Socket creation error");
        return -1;\
    }
    if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &server->opt, sizeof(server->opt))) {
        perror("setsockopt error");
        return -1;
    }
    server->address.sin_family = AF_INET;
    //server->address.sin_addr.s_addr = htons(INADDR_ANY);
    server->address.sin_port = htons(port);
    server->addrlen = sizeof(struct sockaddr_in);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server->address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (bind(server->socket, (struct sockaddr *)&server->address, server->addrlen) < 0) {
        perror("binding to port failed");
        return -1;
    }

    char buf[32];
    INFO("Started Daemon on %s:%hu\n", get_ip_str((struct sockaddr*)&server->address, buf, 32), ntohs(server->address.sin_port));
    return 0;
}


int run_server(Server* server) {
    if (listen(server->socket, 5) < 0) {
        perror("Error listening for connections");
        return -1;
    }


    int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < KEY_COMPOSE; i++) {
        ioctl(uinput_fd, UI_SET_KEYBIT, i);
    }

    // touchpad not working???
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);

    struct uinput_setup dev_setup;
    memset(&dev_setup, 0, sizeof(dev_setup));
    dev_setup.id.bustype = BUS_USB;
    dev_setup.id.vendor = 0x6969;
    dev_setup.id.product = 0x0420;
    strcpy(dev_setup.name, "IOswitch Device");

    ioctl(uinput_fd, UI_DEV_SETUP, &dev_setup);
    ioctl(uinput_fd, UI_DEV_CREATE);
    struct input_event ie = { 0 };
    ssize_t read_bytes;

    int running = 1; 
    while (running) {
        int new_sock_fd;
        if ((new_sock_fd = accept(server->socket, (struct sockaddr*)&server->address, &server->addrlen)) < 0) {
            perror("Error accepting new connection");
            return -1;
        }

        // Get packet that tells the server how to handle the connection
        NewConnection conn;
        if (recv(new_sock_fd, &conn, sizeof(NewConnection), 0) < 0) {
            perror("Failed to receive start connection message");
            return -1;
        }

        switch (conn.type) {
            case INPUT_STREAM:
                read_bytes = read(new_sock_fd, &ie, sizeof(struct input_event));
                if (read_bytes == sizeof(struct input_event)) {
                    write(uinput_fd, &ie, sizeof(struct input_event));
                }
                break;
            case COMMAND:
                printf("Command Received\n");
                CommandArgs args;
                if (recv(new_sock_fd, &args, sizeof(CommandArgs), 0) < 0) {
                    perror("Failed to receive start connection message");
                    return -1;
                }
                int result = server->commands[args.type](server, &args);
                if (result < 0) {
                    fprintf(stderr, "Error processing command\n");
                } else if (result == 1) {
                    printf("Received kill command\n");
                    running = 0;
                }
                break;
            default:
                fprintf(stderr, "Invalid Connection Request\n");
                break;
        }

    }


    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    return 0;
}

Server* create_server(const char* config_path){
    Server* server = malloc(sizeof(Server));
    memset(server, 0, sizeof(Server));
    server->config = load_config(config_path);
    // initialize commands table
    server->commands[NONE] = none;
    server->commands[LIST] = list;
    server->commands[START] = start;
    server->commands[STOP] = stop;
    server->commands[SWITCH] = switch_state;
    server->commands[RELOAD] = reload;
    server->commands[KILL] = kill;

    server->sending = malloc(sizeof(int));
    *server->sending = 0;
    server->sender_mutex = malloc(sizeof(pthread_mutex_t));
    *server->sender_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    return server;
}

void destroy_server(Server* server) {
    pthread_mutex_lock(server->sender_mutex);
    pthread_mutex_unlock(server->sender_mutex);

    close(server->socket);
    free(server->sending);
    free(server->config);
    free(server->sender_mutex);
    free(server);
}

Config* load_config(const char* config_path) {
    Config* config = malloc(sizeof(Config));

    Tokenizer t = load_tokens(config_path);
    Values* vals;

    if ((vals = get_variable(t, "dest_ip")) != NULL) {
        strcpy(config->dest_ip, vals->tokens[0].val);
        free(vals);
    }

    if ((vals = get_variable(t, "dest_port")) != NULL) {
        int temp_port;
        if ((temp_port = atoi(vals->tokens[0].val)) != 0) {
            config->dest_port = temp_port;
        };
        free(vals);
    }

    if ((vals = get_variable(t, "devices")) != NULL) {
        for (int i = 0; i < vals->num_values; i++) {
            printf("%s\n", vals->tokens[i].val);
            strcpy(config->device_names[i], vals->tokens[i].val);
        }
        config->num_devices = vals->num_values;
        free(vals);
    }

    return config;
}

int none(Server* server, CommandArgs* args) { return 0; }

int list(Server* server, CommandArgs* args) {
    return 0;
}

int start(Server* server, CommandArgs* args){
    pthread_mutex_lock(server->sender_mutex);
    if (*server->sending) {
        printf("Already Sending\n");
        return 0;
    }
    *server->sending = 1;
    pthread_mutex_unlock(server->sender_mutex);

    if (args->reload_config) {
        // parse new config
    }
    printf("start command\n");
    SenderArgs* sender_args = malloc(sizeof(SenderArgs));
    sender_args->sending = server->sending;
    sender_args->config = server->config;
    sender_args->mutex = server->sender_mutex;
    pthread_create(&server->sender_handle, NULL, &spawn_sender_thread, (void*)sender_args);
    pthread_detach(server->sender_handle);
    return 0;
}

int stop(Server* server, CommandArgs* args){
    pthread_mutex_lock(server->sender_mutex);
    *server->sending = 0;
    pthread_mutex_unlock(server->sender_mutex);
    return 0;
}

int switch_state(Server* server, CommandArgs* args){
    pthread_mutex_lock(server->sender_mutex);
    if (*server->sending){
        pthread_mutex_unlock(server->sender_mutex);
        stop(server, args);
    } else {
        pthread_mutex_unlock(server->sender_mutex);
        start(server, args);
    }
    return 0;
}

int reload(Server* server, CommandArgs* args){
    pthread_mutex_lock(server->sender_mutex);
    if (args->reload_config) {
        free(server->config);
        server->config = load_config(args->config_path);
    }
    pthread_mutex_unlock(server->sender_mutex);
    stop(server, args);
    start(server, args);
    return 0;
}

int kill(Server* server, CommandArgs* args){
    stop(server, args);
    return 1;
}

void* spawn_sender_thread(void* args) {
    Config* config = ((SenderArgs*)args)->config;
    pthread_mutex_t* mutex = ((SenderArgs*)args)->mutex;
    int* sending = ((SenderArgs*)args)->sending;

    pthread_mutex_lock(mutex);
    struct pollfd* device_fds = malloc(sizeof(struct pollfd)*config->num_devices);
    for(int i = 0; i < config->num_devices; i++) {
        char* name = config->device_names[i];
        printf("Sending for device: %s\n", name);
        device_fds[i].fd = open(config->device_names[i], O_RDONLY | O_NONBLOCK);
        device_fds[i].events = POLLIN;
        if (device_fds[i].fd < 0) {
            fprintf(stderr, "error unable to open for reading %s", name);
            perror("");
            pthread_mutex_unlock(mutex);
            pthread_exit(NULL);
        }
    }
    pthread_mutex_unlock(mutex);

    // Create the file descriptor
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        pthread_exit(NULL);
    }

    // Connect to server
    pthread_mutex_lock(mutex);
    if (connect_to_server(fd, config->dest_ip, config->dest_port) < 0) {
        fprintf(stderr, "Error Connecting to the server\n");
        pthread_mutex_unlock(mutex);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(mutex);

    printf("Connected to Destination Daemon\n");

    int connected = 1;
    while (connected) {
        pthread_mutex_lock(mutex);
        for(int i = 0; i < config->num_devices; i++) {
            // Send initial packet
            NewConnection conn = {
                .type = INPUT_STREAM,
            };

            if (send(fd, &conn, sizeof(NewConnection), 0) < 0) {
                perror("Error Sending Conn Packet");
                pthread_exit(NULL);
            }
            printf("here\n");
            if (send_input_event(&device_fds[i], fd) < 0) {
                fprintf(stderr, "Failed sending input event\n");
                connected = 0;
            }
        }
        if (!*sending) {
            connected = 0;
        }
        pthread_mutex_unlock(mutex);
    }
    printf("Stopped Sending\n");

    free(args);
    free(device_fds);
    pthread_exit(NULL);
}
