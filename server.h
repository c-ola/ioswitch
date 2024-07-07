#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <linux/uinput.h>

typedef struct ie_packet {
    int count;
    struct input_event ie_buf[5];
} ie_packet;

#define MAX_CONNS 4

typedef struct server {
    int fd;
    int opt;
    struct sockaddr_in address;
    socklen_t addrlen;
} Server;

int start_server(Server* server, int port) {
    server->addrlen = sizeof(struct sockaddr_in);
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("creating the socket failed");
        return -1;
    }

    // Forcefully attaching socket to the port PORT
    // What does this actually do??
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &server->opt, sizeof(server->opt))) {
        perror("setsockopt");
        return -1;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(port);

    if (bind(server->fd, (struct sockaddr*)&server->address, server->addrlen) < 0) {
        perror("binding to port failed");
        return -1;
    }

    return 0;
}

void process_input(int socket) {

    struct uinput_setup usetup;
    int uin_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(uin_fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < KEY_KPDOT; i++) {
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
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Example device");

    ioctl(uin_fd, UI_DEV_SETUP, &usetup);
    ioctl(uin_fd, UI_DEV_CREATE);


    struct ie_packet data = { 0 };
    struct input_event ie = { 0 };
    while(1) {

        ssize_t read_bytes = read(socket, &ie, sizeof(struct input_event));
        if (read_bytes == sizeof(struct input_event)) {
            /*if (ie.type == EV_SYN) {
                printf("EV_SYN\n");
            } else if(ie.type == EV_REL) {
                printf("EV_REL\n");
            } else if(ie.type == EV_KEY) {
                printf("EV_KEY\n");
            } else {
                printf("Unknown\n");
            }*/
            write(uin_fd, &ie, sizeof(struct input_event));
        } else if (read_bytes <= 0) {
            break;
        }
        //ssize_t read_bytes = read(new_socket, &data, sizeof(struct ie_packet));
        /*if (read_bytes == sizeof(struct ie_packet)) {
          for (int i = 0; i < data.count; i++) {
          write(uin_fd, &data.ie_buf[i], sizeof(struct input_event));
          }
        //}*/
    }
    printf("Client disconnected\n");
    ioctl(uin_fd, UI_DEV_DESTROY);
    close(uin_fd);
}


#endif
