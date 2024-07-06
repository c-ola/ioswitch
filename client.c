// Client side C program to demonstrate Socket
// programming
#include "linux/input-event-codes.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_DEVICES 3

typedef struct _client {
    int fd;
    int status;
} Client;

typedef struct dev_reader {
    struct pollfd fds[MAX_DEVICES];
    struct input_event ies[MAX_DEVICES];
    int count;
} dev_reader;

int open_device(dev_reader *dr, const char *dev_path) {
    if (dr->count >= MAX_DEVICES) {
        fprintf(stderr, "Device Reader at max Capacity, Cannot add more\n");
        return -1;
    }
    int *fd = &dr->fds[dr->count].fd;
    *fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    if (*fd < 0) {
        fprintf(stderr, "error unable to open for reading %s\n", dev_path);
        return -1;
    }
    dr->fds[dr->count].events = POLLIN;
    dr->count++;
    return 0;
}

int connect_to_server(Client client, const char *ip, unsigned int port) {

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client.status = connect(client.fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return 0;
}


typedef struct ie_packet {
    int count;
    struct input_event ie_buf[8];
} ie_packet;

// Sends an input event from the specified device
// returns -1 on error
// otherwise returns the number of events sent in the packet
int send_input_event_packet(Client client, dev_reader dr, int device) {
    int eop = 0; // end of input event packet
    ie_packet pkt = { 0 };
    while (!eop) {
        // timeout should probably be a couple of milliseconds to not block everything
        struct pollfd pfd = dr.fds[device];
        struct input_event ie = dr.ies[device];

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
        
        pkt.ie_buf[pkt.count++] = ie;

        if (ie.type == EV_SYN) {
            printf("EV_SYN\n");
            eop = 1;
        } else if(ie.type == EV_REL) {
            printf("EV_REL\n");
        } else {
            printf("unknown");
        }

        int status;
        if ((status = send(client.fd, &pkt, sizeof(struct ie_packet), 0)) <= 0) {
            perror("failed to send message");
            return -1;
        }
    }
    return pkt.count;
}

int main(int argc, char **argv) {

    // Get options (ip, port, device)
    int port = 8080;
    char *ip = "127.0.0.1";
    char *device = NULL;
    char* devices[MAX_DEVICES];
    int dev_count = 0; 
    opterr = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":i:d:p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                devices[0] = optarg;
                dev_count++;
                device = optarg;
                break;
            case 'i':
                ip = optarg;
                break;
            case '?':
                if (optopt == 'd')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                return 1;
                /*if (isprint(optopt)) {
                  fprintf(stderr, "Unknown option -%c.\n", optopt);
                  }*/
        }
    }
    // leftover args are devices
    for (int i = optind; i < argc; i++) {
        devices[dev_count] = argv[i];
        dev_count++;
    }

    printf("ip: %s:%d, device: %s\n", ip, port, device);
    for (int i = 0; i < dev_count; i++) {
        printf("%d: %s\n", i, devices[i]);
    }

    
    dev_reader dev_reader = {0};
    for (int i = 0; i < dev_count; i++) {
        int status = open_device(&dev_reader, devices[i]);
        if (status != 0) {
            perror("Error opening device");
        }
        printf("Opened device: %s\n", devices[i]);
    }

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
    printf("Connected to server\n");
    // set up for reading from /dev/input/eventX

    int status, connected = 1;
    printf("started sending...\n");
    // i think it polls input events too fast so it messes things up
    while (connected) {
        for (int i = 0; i < dev_reader.count; i++) {
            int res = send_input_event_packet(client, dev_reader, i);
            printf("%d events sent\n", res);
        }
    }
    // closing the connected socket
    close(client.fd);
    return 0;
}
