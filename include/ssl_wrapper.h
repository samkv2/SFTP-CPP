#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <iostream>

class SSLWrapper {
public:
    static void initOpenSSL();
    static void cleanupOpenSSL();
    static SSL_CTX* createServerContext();
    static SSL_CTX* createClientContext();
    static void configureContext(SSL_CTX* ctx, const std::string& certPath, const std::string& keyPath);
    static void logErrors();
};

#endif // SSL_WRAPPER_H
