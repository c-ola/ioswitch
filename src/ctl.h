#ifndef CTL_H
#define CTL_H

#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>

#define MAX_DEVICES 5


typedef enum CtlType {
    CTL_NONE = 0,
    CTL_LIST,
    CTL_ADD_DEVICE,
    CTL_RM_DEVICE,
    CTL_DISABLE,
    CTL_ENABLE,
    CTL_KILL,
    CTL_ENUM_LENGTH,
} CtlType;

extern const char* ctltostr[];
const CtlType strtoctl(char* string);

typedef struct CtlCommand {
    CtlType type;
    char device[64];
    char ip[64];
    unsigned int port;
    int data;
} CtlCommand;

#endif
