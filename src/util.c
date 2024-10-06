#include "util.h"
#include "server.h"
#include <X11/Xlib.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <pthread.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

// Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen){
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}

int connect_to_server(int fd, const char *ip, unsigned int port) {
    printf("Trying to connect with ip %s:%d\n", ip, port);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed \n");
        return -1;
    }
    return 0;
}

// returns 1 if binds have all been pressed
int send_input_event(int k, struct pollfd* devfd, int dest_sock_fd) {
    InputPacket packet = { 0 };
    // timeout should probably be a couple of milliseconds to not block everything
    int ret = poll(devfd, k, 5);
    if (ret < 0) {
        perror("Error Polling");
        return -1;
    }
    for (int i = 0; i < k; i++) {
        if (devfd[i].revents & POLLIN) {
            ssize_t r = read(devfd[i].fd, (void *)&packet.ie, sizeof(struct input_event));
            if (r < 0) {
                perror("error reading device");
                return -1;
            }
            packet.dev_id = i;
            if (send(dest_sock_fd, &packet, sizeof(InputPacket), 0) <= 0) {
                perror("failed to send message");
                return -1;
            }
        }
    }
    return 0;
}


// returns 1 if the fork has finished
DisplayLock* lock(){
    DisplayLock* disp_lock = malloc(sizeof(DisplayLock));
    XSetWindowAttributes attrib;
    Cursor cursor = { 0 };
    XColor csr_fg, csr_bg, dummy, black;
    int blank = 0;
    int ret, screen;

    struct timeval tv;
    int tvt, gs;

    if (getenv("WAYLAND_DISPLAY"))
        fprintf(stderr,"WARNING: Wayland X server detected: might be buggy\n");

    disp_lock->display = XOpenDisplay(0);

    if (disp_lock->display==NULL) {
        fprintf(stderr,"Error Creating Display\n");
        exit(1);
    }

    attrib.override_redirect= True;

    if (blank) {
        screen = DefaultScreen(disp_lock->display);
        attrib.background_pixel = BlackPixel(disp_lock->display, screen);
        disp_lock->window= XCreateWindow(disp_lock->display,DefaultRootWindow(disp_lock->display),
                0,0,DisplayWidth(disp_lock->display, screen),DisplayHeight(disp_lock->display, screen),
                0,DefaultDepth(disp_lock->display, screen), CopyFromParent, DefaultVisual(disp_lock->display, screen),
                CWOverrideRedirect|CWBackPixel,&attrib); 
        XAllocNamedColor(disp_lock->display, DefaultColormap(disp_lock->display, screen), "black", &black, &dummy);
    } else {
        disp_lock->window= XCreateWindow(disp_lock->display,DefaultRootWindow(disp_lock->display),
                0,0,1,1,0,CopyFromParent,InputOnly,CopyFromParent,
                CWOverrideRedirect,&attrib);
    }

    XSelectInput(disp_lock->display,disp_lock->window,KeyPressMask|KeyReleaseMask);

    ret = XAllocNamedColor(disp_lock->display,
            DefaultColormap(disp_lock->display, DefaultScreen(disp_lock->display)),
            "steelblue3",
            &dummy, &csr_bg);
    if (ret==0)
        XAllocNamedColor(disp_lock->display,
                DefaultColormap(disp_lock->display, DefaultScreen(disp_lock->display)),
                "black",
                &dummy, &csr_bg);

    ret = XAllocNamedColor(disp_lock->display,
            DefaultColormap(disp_lock->display,DefaultScreen(disp_lock->display)),
            "grey25",
            &dummy, &csr_fg);
    if (ret==0)
        XAllocNamedColor(disp_lock->display,
                DefaultColormap(disp_lock->display, DefaultScreen(disp_lock->display)),
                "white",
                &dummy, &csr_bg);

    XMapWindow(disp_lock->display,disp_lock->window);

    /*Sometimes the WM doesn't ungrab the keyboard quickly enough if
     *launching xtrlock from a keystroke shortcut, meaning xtrlock fails
     *to start We deal with this by waiting (up to 100 times) for 10,000
     *microsecs and trying to grab each time. If we still fail
     *(i.e. after 1s in total), then give up, and emit an error
     */

    gs=0; /*gs==grab successful*/
    for (tvt=0 ; tvt<100; tvt++) {
        ret = XGrabKeyboard(disp_lock->display,disp_lock->window,False,GrabModeAsync,GrabModeAsync,
                CurrentTime);
        if (ret == GrabSuccess) {
            gs=1;
            break;
        }
        /*grab failed; wait .01s*/
        tv.tv_sec=0;
        tv.tv_usec=10000;
        select(1,NULL,NULL,NULL,&tv);
    }
    if (gs==0){
        fprintf(stderr,"Error grabbing keyboard\n");
        exit(1);
    }

    if (XGrabPointer(disp_lock->display,disp_lock->window,False,(KeyPressMask|KeyReleaseMask)&0,
                GrabModeAsync,GrabModeAsync,None,
                cursor,CurrentTime)!=GrabSuccess) {
        XUngrabKeyboard(disp_lock->display,CurrentTime);
        fprintf(stderr,"cannot grab pointer\n");
        exit(1);
    }
    disp_lock->locked = 1;
    return disp_lock;
}

int try_unlock(DisplayLock *disp_lock, KeyBind* bind) {
    int counter = 0;
    XEvent ev;
    long timeout= 0;

    XNextEvent(disp_lock->display, &ev);
    counter++;
    if (counter == 100) {
        disp_lock->locked = 0;
    }
    switch (ev.type) {
        case KeyPress:
            if (ev.xkey.time < timeout) { XBell(disp_lock->display,0); break; }
            KeySym ks = XLookupKeysym(&ev.xkey, 0);
            //char* buf = XKeysymToString(ks);
            if (bind->keys[bind->idx] == ks) {
                printf("%d\n", bind->idx);
                bind->idx++;
            } else {
                bind->idx = 0;
            }
            if (bind->idx == bind->len) {
                disp_lock->locked = 0;
            }
            break;
        default:
            break;
    }
    if (disp_lock->locked == 0) {
        XUngrabKeyboard(disp_lock->display,CurrentTime);
        XCloseDisplay(disp_lock->display);
        printf("Exiting lock\n");
        return 1;
    }
    return 0;
}

void *spawn_lock_thread(void* args) {
    printf("[Lock Thread] Locked\n");
    // Once a proper connection is established, lock input
    DisplayLock* disp_lock = lock();
    KeyBind bind = {
        .keys = {XK_Super_L, XK_Control_L, XK_period},
        .len = 3,
        .idx = 0
    };
    while (disp_lock->locked){
        try_unlock(disp_lock, &bind);
    }

    free(disp_lock);
    printf("[Lock thread] Unlocked\n");
    pthread_exit(NULL);
}
