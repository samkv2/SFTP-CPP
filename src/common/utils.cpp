#include "utils.h"
#include "utils.h"
#include "platform.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip> // For hex output if needed

namespace Utils {
    
    void sendPacket(SSL* ssl, PacketType type, const std::vector<uint8_t>& payload) {
        PacketHeader header;
        header.type = type;
        header.length = htonl(payload.size()); // Network byte order

        // Send Header
        int bytes = SSL_write(ssl, &header, sizeof(header));
        if (bytes <= 0) {
            throw std::runtime_error("Failed to send header");
        }

        // Send Payload
        if (!payload.empty()) {
            bytes = SSL_write(ssl, payload.data(), payload.size());
            if (bytes <= 0) {
                 throw std::runtime_error("Failed to send payload");
            }
        }
    }

    void sendPacket(SSL* ssl, PacketType type, const std::string& payload) {
        std::vector<uint8_t> data(payload.begin(), payload.end());
        sendPacket(ssl, type, data);
    }

    Packet recvPacket(SSL* ssl) {
        PacketHeader header;
        int bytes = SSL_read(ssl, &header, sizeof(header));
        if (bytes <= 0) {
             throw std::runtime_error("Connection closed or error reading header");
        }

        header.length = ntohl(header.length); 
        
        Packet packet;
        packet.type = header.type;

        if (header.length > 0) {
            packet.payload.resize(header.length);
            int totalRead = 0;
            while (totalRead < header.length) {
                bytes = SSL_read(ssl, packet.payload.data() + totalRead, header.length - totalRead);
                if (bytes <= 0) {
                    throw std::runtime_error("Error reading payload");
                }
                totalRead += bytes;
            }
        }
        return packet;
    }
}
