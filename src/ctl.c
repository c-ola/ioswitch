#include "ctl.h"

#include <string.h>

const char* ctltostr[] = { "CTL_NONE", "CTL_LIST", "CTL_ADD_DEVICE", "CTL_ADD_BINDING", "CTL_RM_DEVICE", "CTL_DISABLE", "CTL_ENABLE", "CTL_KILL"};

const CtlType strtoctl(char* string) {
    if (strncasecmp("NONE", string, 4) == 0) return CTL_NONE;
    if (strncasecmp("LIST", string, 4) == 0) return CTL_LIST;
    if (strncasecmp("ADD", string, 3) == 0) return CTL_ADD_DEVICE;
    if (strncasecmp("BIND", string, 4) == 0) return CTL_ADD_BINDING;
    if (strncasecmp("RM", string, 2) == 0) return CTL_RM_DEVICE;
    if (strncasecmp("DISABLE", string, 7) == 0) return CTL_DISABLE;
    if (strncasecmp("ENABLE", string, 6) == 0) return CTL_ENABLE;
    if (strncasecmp("KILL", string, 4) == 0) return CTL_KILL;
    return CTL_NONE;
}
