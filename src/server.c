#include "server.h"
#include "client.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int start_server(Server *server, int port) {
    server->addrlen = sizeof(struct sockaddr_in);
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("creating the socket failed");
        return -1;
    }

    // Forcefully attaching socket to the port PORT
    // What does this actually do??
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                &server->opt, sizeof(server->opt))) {
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
    CtlCommand command = { 0 };
    ssize_t r = recv(socketfd, &command, sizeof(struct CtlCommand), 0);
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

    int pid = fork();
    if (pid < 0) {
        perror("Error creating fork");
        return -1;
    }

    if (pid == 0) {
        printf("forked\n");
        close(server->fd);
        create_input_sender(command.device, command.ip, command.port);
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

    return 0;
}

// adds a device that removes itself if a binding is pressed
int add_binding(Server *server, CtlCommand command) {
    if (server->num_senders >= MAX_SENDERS) {
        fprintf(stderr, "Error: devices at max capacity\n");
        return -1;
    }

    int pid = fork();
    if (pid < 0) {
        perror("Error creating fork");
        return -1;
    }

    if (pid == 0) {
        printf("forked\n");
        close(server->fd);
        create_input_binding(command.device, command.ip, command.port);
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
            kill(sender_pid, SIGKILL);
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

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;  /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Example device");

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
    return 0;
}

int create_input_sender(char *device, char *ip, unsigned int port) {
    printf("Creating sender to %s:%d\n", ip, port);
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "error unable to open for reading %s\n", device);
        return -1;
    }
    printf("Opened device: %s\n", device);

    // Connect the client to the server
    Client client = {0};
    if ((client.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    if (connect_to_server(client, ip, port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    SocketType type = INPUT;
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
        if (res < 0) {
            fprintf(stderr, "Error occured, closing\n");
            connected = 0;
        }
    }
    // closing the connected socket
    close(client.fd);
    return 0;
}

int create_input_binding(char *device, char *ip, unsigned int port) {
    printf("Creating sender to %s:%d\n", ip, port);
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "error unable to open for reading %s\n", device);
        return -1;
    }
    printf("Opened device: %s\n", device);

    // Connect the client to the server
    Client client = {0};
    client.num_binds = 3;
    client.binds = malloc(client.num_binds * sizeof(int));
    client.binds_buf = malloc(client.num_binds * sizeof(int));
    client.binds[0] = KEY_LEFTMETA;
    client.binds[1] = KEY_LEFTSHIFT;
    client.binds[2] = KEY_COMMA;


    if ((client.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    if (connect_to_server(client, ip, port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    SocketType type = INPUT;
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
            printf("pressed the key binds wow\n");
            connected = 0;
        }

        if (res < 0) {
            fprintf(stderr, "Error occured, closing\n");
            connected = 0;
        }
    }
    free(client.binds);
    free(client.binds_buf);
    // closing the connected socket
    close(client.fd);
    char *const * args = { NULL };
    execvp("ioswitchstop", args);
    return 0;
}
