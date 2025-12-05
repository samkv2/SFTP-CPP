#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <string>
#include <vector>

// Protocol Constants
const uint16_t SERVER_PORT = 8080;
const int BUFFER_SIZE = 4096;

enum class PacketType : uint8_t {
    AUTH = 0x01,
    LIST_REQ = 0x02,
    LIST_RESP = 0x03,
    UPLOAD_REQ = 0x04,
    DOWNLOAD_REQ = 0x05,
    SUCCESS = 0x06,
    ERROR = 0x07,
    FILE_CHUNK = 0x08,
    END_OF_TRANSFER = 0x09
};

// Protocol Header
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t length; // Length of the payload
    PacketType type;
};
#pragma pack(pop)

#endif // COMMON_H
