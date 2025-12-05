#include "client.h"
#include "platform.h"
#include <iostream>

int main(int argc, char* argv[]) {
    initSockets();
    try {
        std::string host = "127.0.0.1";
        int port = 8080;
        
        if (argc > 1) host = argv[1];

        SFTPClient client(host, port);
        client.connectToServer();
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Client Error: " << e.what() << std::endl;
        cleanupSockets();
        return 1;
    }
    cleanupSockets();
    return 0;
}
