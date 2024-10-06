#include "server.h"
#include <stdio.h>
#include <string.h>
#include "util.h"

int main(int argc, char* argv[]) {
    char config_path[256];
    strcat(strcpy(config_path, getenv("HOME")), "/.config/ioswitch/config");

    int opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, ":c:")) != -1) {
        switch (opt) {
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

    int server_port = 16307;
    Server* server = create_server(config_path);

    if (start_server(server, server_port) < 0) {
        fprintf(stderr, "Could not start server\n");
        destroy_server(server);
        return -1;
    }

    if(run_server(server) < 0) {
        fprintf(stderr, "Error running server\n");
        destroy_server(server);
        return -1;
    }

    INFO("Clean exit\n");
    destroy_server(server);
    return 0;
}


