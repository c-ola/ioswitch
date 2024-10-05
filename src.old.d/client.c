#include "client.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __unix__
    #include <linux/input.h>
    #include <linux/uinput.h>
    #include <sys/poll.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/poll.h>
    #include <sys/socket.h>
    #include <sys/types.h>
#else 
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

int connect_to_server(int fd, const char *ip, unsigned int port) {
    printf("Trying to connect with ip %s:%d\n", ip, port);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed \n");
        return -1;
    }
    return 0;
}

// returns 1 if binds have all been pressed
int send_input_event(Sender* sender) {
    // timeout should probably be a couple of milliseconds to not block everything
    struct input_event ie = { 0 };

    int ret = poll(&sender->devfd, 1, -1);
    if (ret < 0) {
        printf("timeout\n");
        return -1;
    }
    if (!sender->devfd.revents) {
        printf("error somehow\n");
        return -1;
    }
    ssize_t r = read(sender->devfd.fd, (void *)&ie, sizeof(struct input_event));
    if (r < 0) {
        perror("error reading device");
        return -1;
    }
    
    if (sender->num_keys > 0) {
        int binds_pressed = 0;
        for (int i = 0; i < sender->num_keys; i++) {
            if (sender->keybind[i] == ie.code) {
                sender->keybind_buf[i] = ie.value;
            }
            if (sender->keybind_buf[i] != 0) {
                binds_pressed += 1;
            }
        }
        if (binds_pressed == sender->num_keys) return 1;
    }

    if (send(sender->sockfd, &ie, sizeof(struct input_event), 0) <= 0) {
        perror("failed to send message");
        return -1;
    }
    return 0;
}

Sender* create_sendera(SenderArgs* args) {
    Sender* sender = malloc(sizeof(Sender));
    sender->sockfd = args->cfd;
    sender->devfd.fd = args->dfd;
    sender->devfd.events = POLLIN;
    sender->num_keys = args->num_keys;
    sender->keybind = args->keybind;
    sender->keybind_buf = malloc(sender->num_keys * sizeof(int));
    memset(sender->keybind_buf, 0, sender->num_keys * sizeof(int));
    return sender;
}

void destroy_sender(Sender *sender) {
    free(sender->keybind);
    free(sender->keybind_buf);
    close(sender->sockfd);
    close(sender->devfd.fd);
    free(sender);
}

Listener* create_listenera(ListenerArgs* args) {
    Listener* l = malloc(sizeof(Listener));
    l->virtdevfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    
    ioctl(l->virtdevfd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < KEY_COMPOSE; i++) {
        ioctl(l->virtdevfd, UI_SET_KEYBIT, i);
    }

    // touchpad not working???
    ioctl(l->virtdevfd, UI_SET_EVBIT, EV_KEY);
    ioctl(l->virtdevfd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(l->virtdevfd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(l->virtdevfd, UI_SET_EVBIT, EV_REL);
    ioctl(l->virtdevfd, UI_SET_RELBIT, REL_X);
    ioctl(l->virtdevfd, UI_SET_RELBIT, REL_Y);
    ioctl(l->virtdevfd, UI_SET_KEYBIT, BTN_MIDDLE);

    memset(&l->dev_setup, 0, sizeof(l->dev_setup));
    l->dev_setup.id.bustype = BUS_USB;
    l->dev_setup.id.vendor = 0x6969;
    l->dev_setup.id.product = 0x0420;
    strcpy(l->dev_setup.name, "IOswitch Device");

    ioctl(l->virtdevfd, UI_DEV_SETUP, &l->dev_setup);
    ioctl(l->virtdevfd, UI_DEV_CREATE);

    return l;
}

void destroy_listener(Listener *listener) {
    printf("Client disconnected\n");
    ioctl(listener->virtdevfd, UI_DEV_DESTROY);
    close(listener->virtdevfd);
    free(listener);
}

