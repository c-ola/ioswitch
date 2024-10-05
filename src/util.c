#include "util.h"
#include <arpa/inet.h>
#include <linux/input.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

// Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen){
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}

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
int send_input_event(struct pollfd* devfd, int dest_sock_fd) {
    // timeout should probably be a couple of milliseconds to not block everything
    struct input_event ie = { 0 };

    int ret = poll(devfd, 1, -1);
    if (ret < 0) {
        printf("timeout\n");
        return -1;
    }
    if (!devfd->revents) {
        printf("error somehow\n");
        return -1;
    }

    ssize_t r = read(devfd->fd, (void *)&ie, sizeof(struct input_event));
    if (r < 0) {
        perror("error reading device");
        return -1;
    }

    if (send(dest_sock_fd, &ie, sizeof(struct input_event), 0) <= 0) {
        perror("failed to send message");
        return -1;
    }
    return 0;
}
