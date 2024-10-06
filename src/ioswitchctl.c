#include "server.h"
#include "util.h"
#include "tokenizer/tokenizer.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct CtlConfig {
    CommandType command;
    uint16_t source_port;
    uint16_t daemon_port;
    char daemon_ip[16];
} CtlConfig;


const CommandType str_to_command(char* string){
    if (strncasecmp("NONE", string, 4) == 0) return NONE;
    if (strncasecmp("LIST", string, 4) == 0) return LIST;
    if (strncasecmp("START", string, 5) == 0) return START;
    if (strncasecmp("STOP", string, 4) == 0) return STOP;
    if (strncasecmp("SWITCH", string, 5) == 0) return SWITCH;
    if (strncasecmp("RELOAD", string, 6) == 0) return RELOAD;
    if (strncasecmp("KILL", string, 4) == 0) return KILL;
    return NONE;
}

void parse_config(CtlConfig* config, const char* config_path) {
    Tokenizer t = load_tokens(config_path);
    Values* vals;

    if ((vals = get_variable(t, "daemon_ip")) != NULL) {
        strcpy(config->daemon_ip, vals->tokens[0].val);
        free(vals);
    }

    if ((vals = get_variable(t, "daemon_port")) != NULL) {
        int temp_port;
        if ((temp_port = atoi(vals->tokens[0].val)) != 0) {
            config->daemon_port = temp_port;
        };
        free(vals);
    }

    if ((vals = get_variable(t, "source_port")) != NULL) {
        int temp_port;
        if ((temp_port = atoi(vals->tokens[0].val)) != 0) {
            config->source_port = temp_port;
        };
        free(vals);
    }
}


int send_command(CtlConfig config) {
    // Create the CTL socket
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    // The daemon you're sending the command to
    struct sockaddr_in daemon_addr;
    daemon_addr.sin_family = AF_INET;
    daemon_addr.sin_port = htons(config.daemon_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, config.daemon_ip, &daemon_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    char buf[32];
    printf("Attempting to connect to daemon on %s:%hu\n", get_ip_str((struct sockaddr*)&daemon_addr, buf, 32), ntohs(daemon_addr.sin_port));
    // Connect the CTL to the Daemo
    if (connect(fd, (struct sockaddr *)&daemon_addr, sizeof(daemon_addr)) < 0) {
        perror("Connection To Daemon Failed");
        return -1;
    }
    printf("Connected\n");

    NewConnection conn = {
        .type = COMMAND,
    };

    // Tell the Daemon what kind of connection you're starting (it's always COMMAND from the CTL)
    if (send(fd, (const char*)&conn, sizeof(NewConnection), 0) <= 0) {
        perror("Error Sending Command");
        return -1;
    }
    
    printf("Sending Command\n");
    // Send the command
    CommandArgs args = { 0 };
    args.reload_config = 0;
    args.type = config.command;
    if (send(fd, (const char*)&args, sizeof(CommandArgs), 0) <= 0) {
        perror("Error Sending Command");
        return -1;
    }

    close(fd);
    return 0;
}

int main(int argc, char **argv) {
    char config_path[256];
    strcat(strcpy(config_path, getenv("HOME")), "/.config/ioswitch/config");

    CtlConfig config = {
        .source_port = 8080,
        .daemon_port = 8080,
        .daemon_ip = "127.0.0.1",
        .command = NONE
    };
    
    int do_parse_config = 1;

    int opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, ":t:i:d:p:l:c:n:")) != -1) {
        switch (opt) {
            case 'd':
                config.daemon_port = atoi(optarg);
                break;
            case 't':
                config.command = str_to_command(optarg);
                break;
            case 'i':
                strcpy(config.daemon_ip, optarg);
                break;
            case 'n':
                do_parse_config = 0;
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

    if (do_parse_config) {
        parse_config(&config, config_path);
    }

    if (send_command(config) < 0) {
        fprintf(stderr, "Failed to send Command\n");
    }

    return 0;
}
