#pragma once

#include "tokenizer/tokenizer.h"
#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>   //Provides declarations for tcp header
#include <netinet/ip.h>    //Provides declarations for ip header
#include "commands.h"

#define MAX_DEVICES 8 // the number of devices the 

typedef enum ConnectionType {
    INPUT_STREAM,
    COMMAND 
} ConnectionType;

typedef struct NewConnection {
    ConnectionType type;
    int num_devices;
    char device_names[8][64];
} NewConnection;


typedef struct Config {
    char dest_ip[16];
    uint16_t dest_port;
    char device_names[8][64];
    uint16_t num_devices;
} Config;



typedef struct VirtualDevice {
    int sockfd;
    int devfd;
    struct uinput_setup dev_setup;
    char name[64];
} VirtualDevice;

typedef struct ListenerArgs {
    int sockfd;
    int num_devices;
    VirtualDevice devices[MAX_DEVICES];
    pthread_mutex_t* mutex;
    int* connection;
} ListenerArgs;


typedef struct InputPacket {
    int dev_id; // index of the device in the listener device array
    struct input_event ie;
} InputPacket;

#define MAX_CONNECTIONS 8

typedef struct Server {
    int socket; // the server's socket's file descriptor
    int opt;
    int server_port;
    struct sockaddr_in address;
    socklen_t addrlen;
    Command commands[COMMANDS_LEN];


    // SHARED STUFF
    Config* config;

    // sender
    int* sending;
    pthread_t sender_handle;
    pthread_mutex_t* sender_mutex; 

    // listener
    // Manage sender connections to the listener
    int num_conns;
    
    pthread_mutex_t* listener_mutex;
    pthread_t listener_handle[MAX_CONNECTIONS];
    int connections[MAX_CONNECTIONS]; // list of sockets to be listened from
    

} Server;

VirtualDevice create_device(int sockfd, const char* name);
void destroy_device(VirtualDevice dev);

int start_server(Server* server, int port);
int run_server(Server* server);
Server* create_server(const char* config_path);
void destroy_server(Server* server);
Config* load_config(const char* config_path);

int none(Server* server, CommandArgs* args);
int list(Server* server, CommandArgs* args);
int start(Server* server, CommandArgs* args);
int stop(Server* server, CommandArgs* args);
int switch_state(Server* server, CommandArgs* args);
int reload(Server* server, CommandArgs* args);
int kill(Server* server, CommandArgs* args);

typedef struct SenderArgs {
    Config* config;
    int* sending;
    pthread_mutex_t* mutex;
} SenderArgs;

void* spawn_sender_thread(void* config);
void* spawn_listener_thread(void* config);
