#include "server.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/poll.h>

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
    server->address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server->address.sin_port = htons(port);

    if (bind(server->fd, (struct sockaddr*)&server->address, server->addrlen) < 0) {
        perror("binding to port failed");
        return -1;
    }

    return 0;
}

// creates a new input device on the servers system that listens from socketfd
int create_input_listener(int socketfd) {
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


    struct input_event ie = { 0 };
    while(1) {

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


int create_input_sender(int socketfd, char* device, char* ip, unsigned int port) {
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
        if (getpid() == 1) {
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

