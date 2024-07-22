#ifdef __unix__
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/poll.h>
    #include <sys/socket.h>
#else
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define CLOSESOCK(s) closesocket(s); WSACleanup();
    #define CREATE_SOCKET(fd) \
    WSADATA wsa; \
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {\
        printf("\nError: Windows socket subsytsem could not be initialized. Error Code: %d. Exiting..\n", WSAGetLastError()); \
        exit(1);\
    }\
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {\
        printf("\n Socket creation error \n");\
        return -1;\
    }
#else
    #define CLOSESOCK(s) close(s);
    #define CREATE_SOCKET(fd) \
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {\
        printf("\n Socket creation error \n");\
        return -1;\
    }
#endif