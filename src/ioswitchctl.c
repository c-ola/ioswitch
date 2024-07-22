#include "ctl.h"
#include "server.h"
#include "client.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"

#ifdef __unix__
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/poll.h>
    #include <sys/socket.h>
#else
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

// CTL uses:
// - add another sending device to the daemon
// - tell the daemon to remove a device
// - list devices being sent/received
// - disable/enable devices

int main(int argc, char **argv) {
    // Get options (ip, port, device)
    int port = 8080;
    int daemon_port = LOCAL_PORT;
    char *ip = "127.0.0.1";
    char *device = "none";
    opterr = 0;

    CtlType ctl_type = CTL_NONE;

    int opt;
#ifdef __unix__
    while ((opt = getopt(argc, argv, ":t:i:d:p:l:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'l':
                daemon_port = atoi(optarg);
                break;
            case 't':
                ctl_type = strtoctl(optarg);
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
#endif
    Client client = { 0 };
    CREATE_SOCKET(client.fd);

    if (connect_to_server(client, "127.0.0.1", daemon_port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    // initialization packet
    int status;
    SocketType type = MESSAGE;
    if ((status = send(client.fd, (const char*)&type, sizeof(SocketType), 0)) <= 0) {
        return -1;
    }

    char buffer[64];
    status = recv(client.fd, buffer, sizeof(buffer), 0);
    if (status < 0) {
        perror("Did not get handshake");
        return -1;
    }
    printf("hs: %s\n", buffer);


    CtlCommand command = { 0 };
    command.type = ctl_type;
    printf("sending %s command\n", ctltostr[ctl_type]);
    strcpy(command.device, device);
    strcpy(command.ip, ip);
    command.port = port;
    if ((status = send(client.fd, (const char*)&command, sizeof(struct CtlCommand), 0)) <= 0) {
        fprintf(stderr, "Failed to send command");
        return -1;
    }

    // set up for reading from /dev/input/eventX
    CLOSESOCK(client.fd);
    return 0;
}
