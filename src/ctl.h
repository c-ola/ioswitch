#ifndef CTL_H
#define CTL_H

#include <unistd.h>
#define MAX_DEVICES 5

typedef enum CtlType {
    CTL_NONE = 0,
    CTL_LIST,
    CTL_ADD_DEVICE,
    //CTL_ADD_BINDING,
    CTL_RM_DEVICE,
    CTL_DISABLE,
    CTL_ENABLE,
    CTL_KILL,
    CTL_ENUM_LENGTH,
} CtlType;

extern const char* ctltostr[];
const CtlType strtoctl(char* string);


typedef enum DataType {
    DATA_KEYBIND = 0,
    DATA_CONN,
    DATA_DEV,
} DataType;

typedef struct KeyBind {
    int num_keys;
    int keys[8];
} KeyBind;

typedef struct ConnInfo {
    unsigned int port;
    char ip[16];
} ConnInfo;

typedef struct DeviceName {
    char device[32];
} DeviceName;

typedef struct Data {
    DataType type;
    char data[];
} Data;

// data = 00 00 00 00 00 00 00 00 00
//        ^0       ^1          ^2
// data_arr = data_type1, data2, data_type2, data2, etc
typedef struct CtlCommand {
    CtlType type; // 4 bytes
    ConnInfo conn;
    DeviceName name;
    KeyBind bind;
    //int num_data_frags; // 4 bytes
    //int size;  // 4 bytes 
    //Data data_arr[];
} CtlCommand;


// most of this is useless lmfao (messing around with storing different structs in one array, could just make a linked list thats contiguous in memory)
CtlCommand* create_command(CtlType type);
void destroy_command(CtlCommand* command);

Data* get_data_type(CtlCommand* command, DataType type);

CtlCommand* add_data_to_command(CtlCommand* command, DataType type, char* data);
size_t command_size_to_data_block(CtlCommand* command, int data_idx);

size_t command_size(CtlCommand* command);
size_t data_type_size(DataType type);

void print_command(CtlCommand* command);

#endif
