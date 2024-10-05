#pragma once
#include <stddef.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdio.h>

#define INFO(...) printf("INFO: "); printf(__VA_ARGS__);
#define ERROR(...) printf("ERROR: "); fprintf(stderr, __VA_ARGS__);

// Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);

int connect_to_server(int fd, const char *ip, unsigned int port);
int send_input_event(int id, struct pollfd* devfd, int dest_sock_fd);
