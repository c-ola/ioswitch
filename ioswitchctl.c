#include "server.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "client.h"

// CTL uses:
// - add another sending device to the daemon
// - tell the daemon to remove a device
// - list devices being sent/received
// - disable/enable devices

int main(int argc, char **argv) {
    // Get options (ip, port, device)
    int port = 8080;
    char *ip = "127.0.0.1";
    char *device = "none";
    opterr = 0;

    CtlType ctl_type = CTL_NONE;

    int opt;
    while ((opt = getopt(argc, argv, ":s:i:d:p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                ctl_type = atoi(optarg);
                break;
            case 'd':
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

    // initialization packet
    int status;
    SocketType type = MESSAGE;
    if ((status = send(client.fd, &type, sizeof(SocketType), 0)) <= 0) {
        return -1;
    }
    
    CtlCommand command = { 0 };
    command.type = ctl_type;
    if (strlcpy(command.device, device, 64) >= 64) return -1;
    if (strlcpy(command.ip, ip, 64) >= 64) return -1;
    command.port = port;
    if ((status = send(client.fd, &command, sizeof(struct CtlCommand), 0)) <= 0) {
        fprintf(stderr, "Failed to send command");
        return -1;
    }

    // set up for reading from /dev/input/eventX
    close(client.fd);
    return 0;
}
