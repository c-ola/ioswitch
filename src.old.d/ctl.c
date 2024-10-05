#include "ctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* ctltostr[] = { "CTL_NONE", "CTL_LIST", "CTL_ADD_DEVICE", "CTL_RM_DEVICE", "CTL_DISABLE", "CTL_ENABLE", "CTL_KILL"};

const CtlType strtoctl(char* string) {
    if (strncasecmp("NONE", string, 4) == 0) return CTL_NONE;
    if (strncasecmp("LIST", string, 4) == 0) return CTL_LIST;
    if (strncasecmp("ADD", string, 3) == 0) return CTL_ADD_DEVICE;
    //if (strncasecmp("BIND", string, 4) == 0) return CTL_ADD_BINDING;
    if (strncasecmp("RM", string, 2) == 0) return CTL_RM_DEVICE;
    if (strncasecmp("DISABLE", string, 7) == 0) return CTL_DISABLE;
    if (strncasecmp("ENABLE", string, 6) == 0) return CTL_ENABLE;
    if (strncasecmp("KILL", string, 4) == 0) return CTL_KILL;
    return CTL_NONE;
}

/*CtlCommand* create_command(CtlType type) {
    CtlCommand* c = malloc(sizeof(CtlCommand));
    c->type = type;
    c->num_data_frags = 0;
    c->size = sizeof(CtlCommand);
    return c;
}

size_t data_type_size(DataType type) {
    switch (type) {
        case DATA_KEYBIND:
            return sizeof(KeyBind);
            break;
        case DATA_CONN:
            return sizeof(ConnInfo);
            break;
        case DATA_DEV:
            return sizeof(DeviceName);
            break;
    }
    return 0;
}

size_t command_size(CtlCommand* command) {
    size_t size = 0;
    for (int i = 0; i < command->num_data_frags; i++) {
        Data* data = (Data*)((char*)command->data_arr + size);
        size += data_type_size(data->type) + sizeof(DataType);
    }
    size += sizeof(CtlCommand);
    return size;
}

size_t command_size_to_data_block(CtlCommand* command, int data_idx) {
    size_t size = 0;
    for (int i = 0; i < command->num_data_frags; i++) {
        size += data_type_size(command->data_arr[i].type) + sizeof(DataType);
    }
    return size;
}

void destroy_command(CtlCommand* command) {
    free(command);
}

CtlCommand* add_data_to_command(CtlCommand* command, DataType type, char* data) {
    size_t new_data_size = data_type_size(type) + sizeof(DataType);
    size_t new_data_offset = command_size(command) - sizeof(CtlCommand);
    size_t new_size = command_size(command) + new_data_size;
    //printf("%d, %d, %d\n", (int)command_size(command), (int)new_data_size, (int)new_size);
    CtlCommand* nc = realloc(command, new_size);
    nc->size = new_size;
    Data* data_ptr = (Data*)((char*)nc->data_arr + new_data_offset);
    data_ptr->type = type;
    memcpy(data_ptr->data, data, data_type_size(type));
    nc->num_data_frags++;
    return nc;
}

Data* get_data_type(CtlCommand* command, DataType type) {
    print_command(command);
    Data* data = command->data_arr;
    for (int i = 0; i < command->num_data_frags; i++) {
        printf("%d\n", i);
        if (data->type == type) {
            return data;
        } else {
            data = (Data*)((char*)data + data_type_size(data->type) + sizeof(DataType));
        }
    }
    return NULL;
}

void print_command(CtlCommand* command) {
    for (int i = 0; i < command->size; i++) {
        if (i % 16 == 0) printf("\n");
        if (i % 4 == 0) printf(" | ");
        printf(" 0x%02hhx ", ((char*)command)[i]);
    }
    
    printf("\n");
    printf("type: %s\n", ctltostr[command->type]);
    printf("num_data_frags: %d\n", command->num_data_frags);
    
    size_t offset = 0;
    for(int i = 0; i < command->num_data_frags; i++) {
        Data* data_ptr = (Data*)((char*)command->data_arr + offset);
        printf("data_type: %d\n", data_ptr->type);
        char* data = data_ptr->data;
        DataType type = data_ptr->type;
        if (type == DATA_KEYBIND) {
            KeyBind bind = *(KeyBind*)data;
            printf("num_keys: %d\n", bind.num_keys);
        }
        else if (type == DATA_CONN) {
            ConnInfo info = *(ConnInfo*)data;
            printf("ip:port %s:%d\n", info.ip, info.port);
        }
        else if(type == DATA_DEV) {
            DeviceName dev = *(DeviceName*)data;
            printf("name: %s\n", dev.device);
        }
        offset += data_type_size(type) + sizeof(DataType);
    }
    printf("\n");
}*/
