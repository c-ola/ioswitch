
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <linux/uinput.h>
#include "linux/input-event-codes.h"

int main(int argc, const char* argv[]) {
    const char* device = argv[1];
    struct pollfd fds[1];
    fds[0].fd = open(device, O_RDONLY|O_NONBLOCK);
    if (fds[0].fd < 0) {
        printf("error unable to open for reading %s\n", device);
        return -1;
    }

    const int input_size = sizeof(struct input_event);
    fds[0].events = POLLIN;
    struct input_event* input_data = (struct input_event*)malloc(sizeof(struct input_event));
    while(1) {
        int ret = poll(fds, 1, -1);
        if (ret < 0) {
            printf("timeout\n");
            continue;
        }
        if (!fds[0].revents) {
            printf("error somehow\n");
            continue;
        }
        errno = 0;
        ssize_t r = read(fds[0].fd, (void*)input_data, sizeof(struct input_event));
        if (r < 0) {
            perror("error reading device\n");
            //printf("error %d\n", (int)r);
            //break;
        }
        if (input_data->type == EV_KEY) {
            printf("Keyboard press\n");
            if (input_data->code == KEY_BACKSPACE) {
                printf("BACKSPACE\n");
            }
        }

        printf("time=%ld.%06lu type=%hu code=%hu value=%u\n", input_data->time.tv_sec, input_data->time.tv_usec, input_data->type, input_data->code, input_data->value);
    }



    return 0;
}
