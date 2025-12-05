#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    using SocketType = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    
    using SocketType = int;
    #define CLOSE_SOCKET(s) close(s)
    #define IS_VALID_SOCKET(s) ((s) >= 0)
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#include <iostream>

inline void initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        exit(1);
    }
#endif
}

inline void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

#endif // PLATFORM_H
