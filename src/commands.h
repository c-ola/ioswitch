#pragma once

#include <stdint.h>
typedef struct Server Server;

typedef enum CommandType {
    NONE,
    LIST,
    START,
    STOP,
    SWITCH,
    RELOAD,
    KILL,
    COMMANDS_LEN
} CommandType;

typedef struct CommandArgs {
    CommandType type;
    uint8_t reload_config;
    char config_path[256];
} CommandArgs;

typedef int (*Command)(Server* server, CommandArgs* args);
