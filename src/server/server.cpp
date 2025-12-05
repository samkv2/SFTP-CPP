#include "server.h"
#include "ssl_wrapper.h"
#include "utils.h"
#include "common.h"
#include "platform.h"
#include <iostream>
#include <thread>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

SFTPServer::SFTPServer(int port) : port(port) {
    SSLWrapper::initOpenSSL();
    ctx = SSLWrapper::createServerContext();
    SSLWrapper::configureContext(ctx, "certs/keys/server.crt", "certs/keys/server.key");
    initStorage();
}

SFTPServer::~SFTPServer() {
    if (IS_VALID_SOCKET(serverSocket)) CLOSE_SOCKET(serverSocket);
    SSL_CTX_free(ctx);
    SSLWrapper::cleanupOpenSSL();
}

void SFTPServer::initStorage() {
    if (!fs::exists(storage_dir)) {
        fs::create_directory(storage_dir);
    }
}

void SFTPServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (!IS_VALID_SOCKET(serverSocket)) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        SocketType clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &len);

        if (!IS_VALID_SOCKET(clientSocket)) {
            perror("Accept failed");
            continue;
        }

        std::thread clientThread(&SFTPServer::handleClient, this, clientSocket, clientAddr);
        clientThread.detach();
    }
}

void SFTPServer::handleClient(SocketType clientSocket, struct sockaddr_in addr) {
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, (int)clientSocket); // Cast strictly for OpenSSL on Linux/Win

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        std::cout << "[" << inet_ntoa(addr.sin_addr) << "] Connected securely via " << SSL_get_cipher(ssl) << std::endl;

        try {
            bool running = true;
            while (running) {
                Utils::Packet packet = Utils::recvPacket(ssl);

                switch (packet.type) {
                case PacketType::AUTH:
                    // Simple Auth implementation: Always accept for now
                    Utils::sendPacket(ssl, PacketType::SUCCESS, "Auth Successful");
                    break;
                case PacketType::LIST_REQ:
                    handleList(ssl);
                    break;
                case PacketType::UPLOAD_REQ:
                    handleUpload(ssl, packet.payload);
                    break;
                case PacketType::DOWNLOAD_REQ:
                    handleDownload(ssl, packet.payload);
                    break;
                case PacketType::END_OF_TRANSFER: // Explicit disconnect
                     running = false;
                     break;
                default:
                    std::cerr << "Unknown packet type" << std::endl;
                    running = false;
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Client Disconnected: " << e.what() << std::endl;
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    CLOSE_SOCKET(clientSocket);
}

void SFTPServer::handleList(SSL* ssl) {
    std::string fileList;
    for (const auto& entry : fs::directory_iterator(storage_dir)) {
        fileList += entry.path().filename().string() + "\n";
    }
    Utils::sendPacket(ssl, PacketType::LIST_RESP, fileList);
}

void SFTPServer::handleUpload(SSL* ssl, const std::vector<uint8_t>& initialPayload) {
    // Protocol:
    // 1. Receive Filename (Already in initialPayload)
    // 2. Send ready ACK
    // 3. Loop recv chunks until END_OF_TRANSFER

    try {
        std::string filename(initialPayload.begin(), initialPayload.end());
        // Basic Security: prevent directory traversal
        filename = fs::path(filename).filename().string();
        std::string filepath = storage_dir + "/" + filename;

        std::cout << "Receiving file: " << filename << std::endl;
        Utils::sendPacket(ssl, PacketType::SUCCESS, "Ready");

        std::ofstream outfile(filepath, std::ios::binary);
        if (!outfile.is_open()) {
             Utils::sendPacket(ssl, PacketType::ERROR, "Cannot open file on server");
             return;
        }

        bool transferring = true;
        while(transferring) {
             Utils::Packet chunk = Utils::recvPacket(ssl);
             if (chunk.type == PacketType::FILE_CHUNK) {
                 outfile.write(reinterpret_cast<const char*>(chunk.payload.data()), chunk.payload.size());
             } else if (chunk.type == PacketType::END_OF_TRANSFER) {
                 transferring = false;
             } else {
                 throw std::runtime_error("Unexpected packet during upload");
             }
        }
        outfile.close();
        std::cout << "File received: " << filename << std::endl;
        Utils::sendPacket(ssl, PacketType::SUCCESS, "Upload Complete");

    } catch (const std::exception& e) {
        std::cerr << "Upload Error: " << e.what() << std::endl;
        Utils::sendPacket(ssl, PacketType::ERROR, std::string("Upload Failed: ") + e.what());
    }
}

void SFTPServer::handleDownload(SSL* ssl, const std::vector<uint8_t>& initialPayload) {
    // Protocol:
    // 1. Receive Filename (Already in initialPayload)
    // 2. Check exist -> Send SUCCESS/ERROR
    // 3. Send chunks -> Send END_OF_TRANSFER
    
    try {
        std::string filename(initialPayload.begin(), initialPayload.end());
        filename = fs::path(filename).filename().string(); // Security sanitization
        std::string filepath = storage_dir + "/" + filename;

        if (!fs::exists(filepath)) {
            Utils::sendPacket(ssl, PacketType::ERROR, "File not found");
            return;
        }

        Utils::sendPacket(ssl, PacketType::SUCCESS, "Starting Download");
        
        std::ifstream infile(filepath, std::ios::binary);
        std::vector<uint8_t> buffer(BUFFER_SIZE);

        while (infile.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || infile.gcount() > 0) {
            std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + infile.gcount());
            Utils::sendPacket(ssl, PacketType::FILE_CHUNK, chunk);
        }
        
        Utils::sendPacket(ssl, PacketType::END_OF_TRANSFER, std::vector<uint8_t>{});
        std::cout << "Sent file: " << filename << std::endl;

    } catch (const std::exception& e) {
         std::cerr << "Download Error: " << e.what() << std::endl;
    }
}
