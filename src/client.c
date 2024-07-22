#include "client.h"

#include <linux/input.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


int connect_to_server(Client client, const char *ip, unsigned int port) {
    printf("Trying to connect with ip %s:%d\n", ip, port);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    if ((client.status = connect(client.fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("Connection Failed \n");
        return -1;
    }

    return 0;
}

// returns 1 if binds have all been pressed
int send_input_event(struct pollfd pfd, Client client) {
    // timeout should probably be a couple of milliseconds to not block everything
    struct input_event ie = { 0 };

    int ret = poll(&pfd, 1, -1);
    if (ret < 0) {
        printf("timeout\n");
        return -1;
    }
    if (!pfd.revents) {
        printf("error somehow\n");
        return -1;
    }
    ssize_t r = read(pfd.fd, (void *)&ie, sizeof(struct input_event));
    if (r < 0) {
        perror("error reading device");
        return -1;
    }
    
    if (client.num_binds > 0) {
        int binds_pressed = 0;
        for (int i = 0; i < client.num_binds; i++) {
            if (client.binds[i] == ie.code) {
                client.binds_buf[i] = ie.value;
            }
            if (client.binds_buf[i] != 0) {
                binds_pressed += 1;
            }
        }
        if (binds_pressed == client.num_binds) return 1;
    }


    int status;
    if ((status = send(client.fd, &ie, sizeof(struct input_event), 0)) <= 0) {
        perror("failed to send message");
        return -1;
    }
    return 0;
}
