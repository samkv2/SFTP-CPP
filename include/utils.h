#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <openssl/ssl.h>
#include "common.h"

namespace Utils {
    void sendPacket(SSL* ssl, PacketType type, const std::vector<uint8_t>& payload);
    void sendPacket(SSL* ssl, PacketType type, const std::string& payload);
    struct Packet {
        PacketType type;
        std::vector<uint8_t> payload;
    };
    Packet recvPacket(SSL* ssl);
    std::string getFileChecksum(const std::string& filepath);
}

#endif // UTILS_H
