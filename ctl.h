#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>

#define MAX_DEVICES 5


typedef enum CtlType {
    CTL_LIST = 1,
    CTL_ADD_DEVICE,
    CTL_RM_DEVICE,
    CTL_DISABLE,
    CTL_ENABLE,
    CTL_KILL,
    CTL_NONE,
} CtlType;

typedef struct CtlCommand {
    CtlType type;
    char device[64];
    char ip[64];
    unsigned int port;
} CtlCommand;
