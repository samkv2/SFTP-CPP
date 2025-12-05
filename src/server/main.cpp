#include "server.h"
#include "common.h"
#include "platform.h"
#include <iostream>

int main() {
    initSockets();
    try {
        SFTPServer server(SERVER_PORT);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server Crashed: " << e.what() << std::endl;
        cleanupSockets();
        return 1;
    }
    cleanupSockets();
    return 0;
}
