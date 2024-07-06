#include <asm-generic/socket.h>
#include <complex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/uinput.h>

#include <linux/input-event-codes.h>

#define PORT 8080

int main(void) {

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

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Example device");

    ioctl(uin_fd, UI_DEV_SETUP, &usetup);
    ioctl(uin_fd, UI_DEV_CREATE);

    int server_fd; // server file descriptor
    int new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024];
    
    // assign the server fd to a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("creating the socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port PORT
    // What does this actually do??
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("binding to port failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    struct input_event* input_data = malloc(sizeof(struct input_event));
    int running = 1;
    while (running) {
        valread = read(new_socket, input_data, sizeof(struct input_event));
        if (input_data->type == EV_KEY) {
            write(uin_fd, input_data, sizeof(struct input_event));
        }
    }

    close(new_socket);
    close(server_fd);
    ioctl(uin_fd, UI_DEV_DESTROY);
    close(uin_fd);;

    return 0;
}
