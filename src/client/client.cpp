#include "client.h"
#include "ssl_wrapper.h"
#include "utils.h"
#include "common.h"
#include "platform.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <cmath>

namespace fs = std::filesystem;

// ANSI Colors
const std::string CLEAR_SCREEN = "\033[2J\033[1;1H";
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";

SFTPClient::SFTPClient(const std::string& host, int port) : host(host), port(port) {
    SSLWrapper::initOpenSSL();
    ctx = SSLWrapper::createClientContext();
    // In a real scenario, we'd verify the server's cert against a CA.
    // For this self-signed demo, we load the CA we generated to verify the server.
    if (SSL_CTX_load_verify_locations(ctx, "certs/keys/ca.crt", nullptr) != 1) {
         std::cerr << "Warning: Failed to load CA cert. Verification might fail." << std::endl;
    }
}

SFTPClient::~SFTPClient() {
    if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
    if (IS_VALID_SOCKET(socketFd)) CLOSE_SOCKET(socketFd);
    if (ctx) SSL_CTX_free(ctx);
    SSLWrapper::cleanupOpenSSL();
}

void SFTPClient::connectToServer() {
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (!IS_VALID_SOCKET(socketFd)) throw std::runtime_error("Socket creation failed");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }

    std::cout << BLUE << "Connecting to " << host << ":" << port << "..." << RESET << std::endl;
    if (connect(socketFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("Connection failed");
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socketFd);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("SSL Handshake failed");
    }

    std::cout << GREEN << "Connected securely using " << SSL_get_cipher(ssl) << RESET << std::endl;
    authenticate();
}

void SFTPClient::authenticate() {
    Utils::sendPacket(ssl, PacketType::AUTH, "user:pass"); // Dummy auth
    Utils::Packet response = Utils::recvPacket(ssl);
    if (response.type != PacketType::SUCCESS) {
        throw std::runtime_error("Authentication failed");
    }
}

void SFTPClient::run() {
    while (true) {
        printMenu();
        std::string choice = getLine("Select an option");

        try {
            if (choice == "1") listFiles();
            else if (choice == "2") uploadFile();
            else if (choice == "3") downloadFile();
            else if (choice == "4") break;
            else std::cout << RED << "Invalid option." << RESET << std::endl;
        } catch (const std::exception& e) {
            std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
        }
    }
    Utils::sendPacket(ssl, PacketType::END_OF_TRANSFER, std::vector<uint8_t>{}); // Bye
}

void SFTPClient::printMenu() {
    std::cout << "\n" << BOLD << CYAN << "=== Secure File Transfer Client ===" << RESET << std::endl;
    std::cout << "1. " << YELLOW << "List Remote Files" << RESET << std::endl;
    std::cout << "2. " << GREEN << "Upload File" << RESET << std::endl;
    std::cout << "3. " << BLUE << "Download File" << RESET << std::endl;
    std::cout << "4. " << RED << "Exit" << RESET << std::endl;
    std::cout << "===================================" << std::endl;
}

std::string SFTPClient::getLine(const std::string& prompt) {
    std::cout << prompt << "> ";
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void SFTPClient::listFiles() {
    Utils::sendPacket(ssl, PacketType::LIST_REQ, std::vector<uint8_t>{});
    Utils::Packet resp = Utils::recvPacket(ssl);
    
    if (resp.type == PacketType::LIST_RESP) {
        std::string list(resp.payload.begin(), resp.payload.end());
        std::cout << "\n" << BOLD << "--- Remote Files ---" << RESET << std::endl;
        if (list.empty()) std::cout << "(No files found)" << std::endl;
        else std::cout << list;
        std::cout << "--------------------" << std::endl;
    } else {
        std::cout << RED << "Failed to retrieve file list." << RESET << std::endl;
    }
}

void SFTPClient::uploadFile() {
    std::string filepath = getLine("Enter path to file (e.g. ./docs/report.pdf)");
    if (!fs::exists(filepath)) {
        std::cout << RED << "File does not exist!" << RESET << std::endl;
        return;
    }

    std::string filename = fs::path(filepath).filename().string();
    uintmax_t filesize = fs::file_size(filepath);

    // 1. Send Upload Request (Filename)
    std::cout << YELLOW << "[!] Requesting upload for: " << filename << "..." << RESET << std::endl;
    Utils::sendPacket(ssl, PacketType::UPLOAD_REQ, filename);

    // 2. Wait for ACK
    std::cout << YELLOW << "[!] Waiting for server approval..." << RESET << std::endl;
    Utils::Packet ack = Utils::recvPacket(ssl);
    if (ack.type != PacketType::SUCCESS) {
        std::cout << RED << "[X] Server rejected upload: " << std::string(ack.payload.begin(), ack.payload.end()) << RESET << std::endl;
        return;
    }

    // 3. Send Chunks
    std::ifstream infile(filepath, std::ios::binary);
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uintmax_t totalSent = 0;

    std::cout << GREEN << "[*] Starting transfer..." << RESET << std::endl;
    
    // Draw initial empty bar
    drawProgressBar(0.0f);

    while (infile.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || infile.gcount() > 0) {
        std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + infile.gcount());
        Utils::sendPacket(ssl, PacketType::FILE_CHUNK, chunk);
        
        totalSent += chunk.size();
        drawProgressBar((float)totalSent / filesize);
    }
    drawProgressBar(1.0f); // Ensure 100% at end
    std::cout << std::endl;
    
    std::cout << YELLOW << "[!] Finalizing transfer..." << RESET << std::endl;
    Utils::sendPacket(ssl, PacketType::END_OF_TRANSFER, std::vector<uint8_t>{});
    
    // 4. Final confirmation
    Utils::Packet done = Utils::recvPacket(ssl);
    if (done.type == PacketType::SUCCESS) {
        std::cout << GREEN << "[OK] Upload Successful!" << RESET << std::endl;
    } else {
        std::cout << RED << "[X] Upload failed server-side." << RESET << std::endl;
    }
}

void SFTPClient::downloadFile() {
    std::string filename = getLine("Enter filename to download");
    
    // 1. Send Download Request
    Utils::sendPacket(ssl, PacketType::DOWNLOAD_REQ, filename);

    // 2. Check response
    Utils::Packet resp = Utils::recvPacket(ssl);
    if (resp.type != PacketType::SUCCESS) {
        std::cout << RED << "Error: " << (resp.payload.empty() ? "Unknown Error" : std::string(resp.payload.begin(), resp.payload.end())) << RESET << std::endl;
        return;
    }

    std::cout << "Downloading " << filename << "..." << std::endl;
    
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile.is_open()) {
        std::cout << RED << "Cannot create local file!" << RESET << std::endl;
        return;
    }

    size_t totalBytes = 0;
    bool transferring = true;
    while(transferring) {
        Utils::Packet chunk = Utils::recvPacket(ssl);
        if (chunk.type == PacketType::FILE_CHUNK) {
            outfile.write(reinterpret_cast<const char*>(chunk.payload.data()), chunk.payload.size());
            totalBytes += chunk.payload.size();
            std::cout << "\rReceived: " << totalBytes << " bytes" << std::flush;
        } else if (chunk.type == PacketType::END_OF_TRANSFER) {
            transferring = false;
        } else {
            std::cout << RED << "\nProtocol Error!" << RESET << std::endl;
            return;
        }
    }
    std::cout << "\n" << GREEN << "Download Complete!" << RESET << std::endl;
}

void SFTPClient::drawProgressBar(float percentage) {
    int barWidth = 50;
    std::cout << "\r" << CYAN << "[";
    int pos = barWidth * percentage;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "━";
        else if (i == pos) std::cout << "╸";
        else std::cout << " ";
    }
    std::cout << "] " << int(percentage * 100.0) << " %" << RESET;
    std::cout.flush();
}
