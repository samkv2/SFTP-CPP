#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <openssl/ssl.h>
#include <string>
#include <openssl/ssl.h>
#include <vector>
#include "platform.h"

class SFTPServer {
public:
    SFTPServer(int port);
    ~SFTPServer();
    void start();

private:
    int port;
    SocketType serverSocket;
    SSL_CTX* ctx;
    std::string storage_dir = "server_storage";

    void initStorage();
    void handleClient(SocketType clientSocket, struct sockaddr_in addr);
    
    // Command Handlers
    void handleList(SSL* ssl);
    void handleUpload(SSL* ssl, const std::vector<uint8_t>& initialPayload);
    void handleDownload(SSL* ssl, const std::vector<uint8_t>& initialPayload);
};

#endif // SERVER_H
