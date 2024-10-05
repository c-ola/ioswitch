#include "ctl.h"
#include "server.h"
#include "client.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"
#include "tokenizer/tokenizer.h"

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
    // Get options (ip, local_port, device)
    int local_port = 8080;
    int daemon_port = LOCAL_PORT;
    char ip[16] = "127.0.0.1";
    int num_devices = 0;
    char devices[MAX_DEVICES][64] = {};
    char config_path[256];
    strcat(strcpy(config_path, getenv("HOME")), "/.config/ioswitch/config");

    CtlType ctl_type = CTL_NONE;

#ifdef __unix__
    int opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, ":t:i:d:p:l:c:")) != -1) {
        switch (opt) {
            case 'p':
                local_port = atoi(optarg);
                break;
            case 'l':
                daemon_port = atoi(optarg);
                break;
            case 't':
                ctl_type = strtoctl(optarg);
                break;
            case 'd':
                strcpy(devices[num_devices++], optarg);
                break;
            case 'i':
                strcpy(ip, optarg);
                break;
            case 'c':
                getcwd(config_path, sizeof(config_path));
                strcat(config_path, "/");
                strcat(config_path, optarg);
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

    printf("here %s\n", config_path);
    // Parse the config file
    Tokenizer t = load_tokens(config_path);
    Values* vals;

    if ((vals = get_variable(t, "ip")) != NULL) {
        strcpy(ip, vals->tokens[0].val);
        free(vals);
    }
    
    if ((vals = get_variable(t, "target_port")) != NULL) {
        int temp_port;
        if ((temp_port = atoi(vals->tokens[0].val)) != 0) {
            daemon_port = temp_port;
        };
        free(vals);
    }
    
    if ((vals = get_variable(t, "local_port")) != NULL) {
        int temp_port;
        if ((temp_port = atoi(vals->tokens[0].val)) != 0) {
            local_port = temp_port;
        };
        free(vals);
    }

    if ((vals = get_variable(t, "devices")) != NULL) {
        for (int i = 0; i < vals->num_values; i++) {
            strcpy(devices[num_devices++], vals->tokens[i].val);
        }
        free(vals);
    }
    
    // send the commands for each device
    for (int i = 0; i < num_devices; i++) {
        // Connect to the server
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

        // Get the handshake that you are connected
        char buffer[64];
        status = recv(fd, buffer, sizeof(buffer), 0);
        if (status < 0) {
            perror("Did not get handshake");
            return -1;
        }
        printf("hs: %s\n", buffer);

        ConnInfo conn_info = { .port = local_port };
        strcpy(conn_info.ip, ip);
        DeviceName dev_name;
        strcpy(dev_name.device, devices[i]);

        CtlCommand command = {
            .type = ctl_type,
            .conn = conn_info,
            .name = dev_name,
            .bind = { 0 },
        };

        printf("sending %s command with device: %s\n", ctltostr[ctl_type], dev_name.device);
        if ((status = send(fd, (const char*)&command, sizeof(CtlCommand), 0)) <= 0) {
            fprintf(stderr, "Failed to send command");
            return -1;
        }
        CLOSESOCK(fd);
        if (command.type != CTL_ADD_DEVICE && command.type != CTL_RM_DEVICE) {
            break;
        }
    }

    // set up for reading from /dev/input/eventX
    return 0;
}
