#include "server.h"
#include "client.h"
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

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
    server->ctl_handlers[command.type](server, command);
    return 0;
}

int none_command(Server *server, CtlCommand command) {
    return 0;
}

int list_devices(Server *server, CtlCommand command) {
    printf("\n");
    if (server->num_senders == 0) {
        printf("There are 0 senders\n");
    }
    printf("Device Senders:\n");
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (server->sender_ids[i] != 0) {
            printf("id: %d, name: %s\n", server->sender_ids[i],
                    server->sender_names[i]);
        }
    }
    printf("\n");
    return 0;
}

int add_device(Server *server, CtlCommand command) {
    if (server->num_senders >= MAX_SENDERS) {
        fprintf(stderr, "Error: devices at max capacity\n");
        return -1;
    }

#ifdef __unix__
    int pid = fork();
    if (pid < 0) {
        perror("Error creating fork");
        return -1;
    }

    if (pid == 0) {
        close(server->fd);
        create_input_sender(command.device, command.ip, command.port, 0, NULL);
        exit(1);
    } else {
        // TODO check for connection to port first
        for (int i = 0; i < MAX_SENDERS; i++) {
            if (server->sender_ids[i] == 0) {
                server->sender_ids[i] = pid;
                strncpy(server->sender_names[i], command.device, MAX_NAME_LENGTH);
                server->num_senders++;
                break;
            }
        }
        printf("Added sender with id %d and name %s\n", pid, command.device);
    }
#endif

    return 0;
}

// adds a device that removes itself if a binding is pressed
int add_binding(Server *server, CtlCommand command) {
    if (server->num_senders >= MAX_SENDERS) {
        fprintf(stderr, "Error: devices at max capacity\n");
        return -1;
    }

#ifdef __unix__
    int pid = fork();
    if (pid < 0) {
        perror("Error creating fork");
        return -1;
    }

    if (pid == 0) {
        close(server->fd);
        int binds[] = {KEY_LEFTMETA, KEY_LEFTSHIFT, KEY_COMMA};
        create_input_sender(command.device, command.ip, command.port, 3, binds);
        exit(1);
    } else {
        // TODO check for connection to port first
        for (int i = 0; i < MAX_SENDERS; i++) {
            if (server->sender_ids[i] == 0) {
                server->sender_ids[i] = pid;
                strncpy(server->sender_names[i], command.device, MAX_NAME_LENGTH);
                server->num_senders++;
                break;
            }
        }
        printf("Added sender with id %d and name %s\n", pid, command.device);
    }
#endif
    return 0;
}

int rm_device(Server *server, CtlCommand command) {
    if (server->num_senders == 0) {
        fprintf(stderr, "No devices to remove\n");
        return -1;
    }

    for (int i = 0; i < MAX_SENDERS; i++) {
        int sender_pid = server->sender_ids[i];
        if (sender_pid == 0) {
            continue;
        }
        char *name = server->sender_names[i];
        if (strcmp(name, command.device) == 0) {
            printf("Removing device %s with pid %d\n", name, sender_pid);
            server->sender_ids[i] = 0;
            server->num_senders--;
#ifdef __unix__
            kill(sender_pid, SIGKILL);
#else
#endif
            return 0;
        }
    }
    printf("Could not find specified device\n");
    return 0;
}

// creates a new input device on the servers system that listens from socketfd
// maybe make this threaded instead of forked
// ALSO ABSTRACT AWAY THE INPUT DEVICE
int create_input_listener(int socketfd) {
#ifdef __linux__
    struct uinput_setup usetup;
    int uin_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(uin_fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < KEY_COMPOSE; i++) {
        ioctl(uin_fd, UI_SET_KEYBIT, i);
    }

    // touchpad not working???
    ioctl(uin_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uin_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uin_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uin_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uin_fd, UI_SET_RELBIT, REL_X);
    ioctl(uin_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uin_fd, UI_SET_KEYBIT, BTN_MIDDLE);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;  /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "IOswitch Device");

    ioctl(uin_fd, UI_DEV_SETUP, &usetup);
    ioctl(uin_fd, UI_DEV_CREATE);

    struct input_event ie = {0};
    while (1) {

        ssize_t read_bytes = read(socketfd, &ie, sizeof(struct input_event));
        if (read_bytes == sizeof(struct input_event)) {

            write(uin_fd, &ie, sizeof(struct input_event));
        } else if (read_bytes <= 0) {
            return -1;
        }
    }
    printf("Client disconnected\n");
    ioctl(uin_fd, UI_DEV_DESTROY);
    close(uin_fd);
#else
    printf("input listener for windows\n");
#endif

    return 0;
}

int create_input_sender(char *device, char *ip, unsigned int port, int num_binds, int* binds) {
    printf("Creating sender to %s:%d\n", ip, port);
#ifdef __unix__
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "error unable to open for reading %s\n", device);
        return -1;
    }
    printf("Opened device: %s\n", device);

    // Connect the client to the server
    Client client = {0};
    client.num_binds = num_binds;
    client.binds = binds;
    client.binds_buf = malloc(client.num_binds * sizeof(int));

    if ((client.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    if (connect_to_server(client, ip, port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    SocketType type = INPUT_CONN;
    if (send(client.fd, &type, sizeof(SocketType), 0) < 0) {
        return -1;
    }

    printf("Connected to server\n");
    // set up for reading from /dev/input/eventX

    int connected = 1;
    printf("started sending...\n");
    // i think it polls input events too fast so it messes things u
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    while (connected) {
        if (getppid() == 1) {
            connected = 0;
            break;
        }
        int res = send_input_event(pfd, client);
        if (res == 1) {
            printf("Received keybind inputs\n");
            connected = 0;
        }

        if (res < 0) {
            fprintf(stderr, "Error occured, closing\n");
            connected = 0;
        }
    }
    // closing the connected socket
    free(client.binds_buf);
    close(client.fd);

    if (client.num_binds == 0) return 0;

    char *const * args = { NULL };
    int pid = fork();
    if (pid == 0) {
        execvp("ioswitchstop", args);
        exit(1);
    }
#else
    printf("Creating input sender for windows\n");
#endif

    return 0;
}
