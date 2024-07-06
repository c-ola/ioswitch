// Client side C program to demonstrate Socket
// programming
#include "linux/input-event-codes.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_DEVICES 3

typedef struct _client {
  int fd;
  int status;
} Client;

typedef struct dev_reader {
  struct pollfd fds[MAX_DEVICES];
  struct input_event ies[MAX_DEVICES];
  int count;
} dev_reader;

int open_device(dev_reader *dr, const char *dev_path) {
  if (dr->count >= MAX_DEVICES) {
    fprintf(stderr, "Device Reader at max Capacity, Cannot add more\n");
    return -1;
  }
  int *fd = &dr->fds[dr->count].fd;
  *fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (*fd < 0) {
    fprintf(stderr, "error unable to open for reading %s\n", dev_path);
    return -1;
  }
  dr->fds[dr->count].events = POLLIN;
  dr->count++;
  return 0;
}

int connect_to_server(Client client, const char *ip, unsigned int port) {

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if ((client.status = connect(client.fd, (struct sockaddr *)&serv_addr,
                               sizeof(serv_addr))) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }

  return 0;
}

// Sends an input event from the specified device
// returns -1 for a timeout and revent error
// returns 0 for normal behaviour
// returns 1 for a really bad error
int emit_input_event(Client client, dev_reader dr, int device) {

  // timeout should probably be a couple of milliseconds to not block everything
  struct pollfd pfd = dr.fds[device];
  struct input_event ie = dr.ies[device];

  int ret = poll(&pfd, 1, -1);
  if (ret < 0) {
    printf("timeout\n");
    return -1;
  }
  if (!pfd.revents) {
    printf("error somehow\n");
    return -1;
  }
  ssize_t r = read(pfd.fd, (void *)&ie, sizeof(struct input_event));
  if (r < 0) {
    perror("error reading device\n");
    return -1;
  }
  int status;
  if ((status = send(client.fd, &ie, sizeof(struct input_event), 0)) <= 0) {
    printf("failed to send message\n");
    return 1;
  }
  return 0;
}

void emit(int fd, int type, int code, int val) {
  struct input_event ie;

  ie.type = type;
  ie.code = code;
  ie.value = val;
  /* timestamp values below are ignored */
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;

  send(fd, &ie, sizeof(ie), 0);
}

int main(int argc, char **argv) {

  // Get options (ip, port, device)
  int port = 8080;
  char *ip = "127.0.0.1";
  char *device = NULL;
  opterr = 0;

  int opt;
  while ((opt = getopt(argc, argv, ":d:p:")) != -1) {
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'd':
      device = optarg;
      break;
    case '?':
      if (optopt == 'd')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else
        fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
      return 1;
      /*if (isprint(optopt)) {
          fprintf(stderr, "Unknown option -%c.\n", optopt);
      }*/
    }
  }
  if (optind < argc) {
    ip = argv[optind];
  }
  printf("ip: %s:%d, device: %s\n", ip, port, device);

  // Connect the client to the server
  Client client = {0};
  if ((client.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  if (connect_to_server(client, ip, port) < 0) {
    perror("Error connecting to server");
    return -1;
  }

  // set up for reading from /dev/input/eventX
  dev_reader dev_reader = {0};
  open_device(&dev_reader, device);

  int status, connected = 1;
  printf("started sending...\n");
  while (connected) {
    for (int i = 0; i < dev_reader.count; i++) {
      emit_input_event(client, dev_reader, i);
    }
  }
  // closing the connected socket
  close(client.fd);
  return 0;
}
