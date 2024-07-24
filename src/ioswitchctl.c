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

    CtlType ctl_type = CTL_NONE;

#ifdef __unix__
    int opt;
    opterr = 0;
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
        }
    }
#endif
    int fd;
    CREATE_SOCKET(fd);

    if (connect_to_server(fd, "127.0.0.1", daemon_port) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    // initialization packet
    int status;
    SocketType type = MESSAGE;
    if ((status = send(fd, (const char*)&type, sizeof(SocketType), 0)) <= 0) {
        return -1;
    }

    char buffer[64];
    status = recv(fd, buffer, sizeof(buffer), 0);
    if (status < 0) {
        perror("Did not get handshake");
        return -1;
    }
    printf("hs: %s\n", buffer);

    ConnInfo conn_info = { .port = port };
    strcpy(conn_info.ip, ip);
    DeviceName dev_name;
    strcpy(dev_name.device, device);
    
    CtlCommand command = {
        .type = ctl_type,
        .conn = conn_info,
        .name = dev_name,
        .bind = { 0 },
    };

    printf("sending %s command\n", ctltostr[ctl_type]);
    if ((status = send(fd, (const char*)&command, sizeof(CtlCommand), 0)) <= 0) {
        fprintf(stderr, "Failed to send command");
        return -1;
    }

    // set up for reading from /dev/input/eventX
    CLOSESOCK(fd);
    return 0;
}
