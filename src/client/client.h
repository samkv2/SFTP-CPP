#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <openssl/ssl.h>
#include <vector>

class SFTPClient {
public:
    SFTPClient(const std::string& host, int port);
    ~SFTPClient();
    void connectToServer();
    void run();

private:
    std::string host;
    int port;
    int socketFd;
    SSL_CTX* ctx;
    SSL* ssl;

    void authenticate();
    void listFiles();
    void uploadFile();
    void downloadFile();
    
    // UI Helpers
    void printMenu();
    void drawProgressBar(float percentage);
    std::string getLine(const std::string& prompt);
};

#endif // CLIENT_H
