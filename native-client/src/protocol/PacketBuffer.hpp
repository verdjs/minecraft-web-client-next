#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>

namespace mc {

class PacketBuffer {
public:
    std::vector<uint8_t> buffer;
    size_t readPos{0};

    PacketBuffer() = default;
    explicit PacketBuffer(std::vector<uint8_t> data) : buffer(std::move(data)) {}

    // Writing primitives
    void writeByte(uint8_t val) { buffer.push_back(val); }
    void writeBytes(const uint8_t* data, size_t len) {
        buffer.insert(buffer.end(), data, data + len);
    }

    void writeVarInt(int32_t val) {
        uint32_t uval = static_cast<uint32_t>(val);
        while ((uval & ~0x7Fu) != 0) {
            buffer.push_back(static_cast<uint8_t>((uval & 0x7Fu) | 0x80u));
            uval >>= 7;
        }
        buffer.push_back(static_cast<uint8_t>(uval));
    }

    void writeString(const std::string& str) {
        writeVarInt(static_cast<int32_t>(str.size()));
        writeBytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    void writeUShort(uint16_t val) {
        buffer.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(val & 0xFF));
    }

    void writeDouble(double val) {
        uint64_t raw;
        std::memcpy(&raw, &val, sizeof(double));
        for (int i = 7; i >= 0; --i) {
            buffer.push_back(static_cast<uint8_t>((raw >> (i * 8)) & 0xFF));
        }
    }

    void writeFloat(float val) {
        uint32_t raw;
        std::memcpy(&raw, &val, sizeof(float));
        for (int i = 3; i >= 0; --i) {
            buffer.push_back(static_cast<uint8_t>((raw >> (i * 8)) & 0xFF));
        }
    }

    void writeBool(bool val) {
        writeByte(val ? 1 : 0);
    }

    // Reading primitives
    uint8_t readByte() {
        if (readPos >= buffer.size()) throw std::runtime_error("Buffer underflow");
        return buffer[readPos++];
    }

    int32_t readVarInt() {
        int32_t val = 0;
        int position = 0;
        uint8_t byte = 0;

        while (true) {
            byte = readByte();
            val |= (byte & 0x7F) << position;
            if ((byte & 0x80) == 0) break;
            position += 7;
            if (position >= 32) throw std::runtime_error("VarInt is too big");
        }
        return val;
    }

    std::string readString(size_t maxLen = 32767) {
        int32_t len = readVarInt();
        if (len < 0 || static_cast<size_t>(len) > maxLen || readPos + len > buffer.size()) {
            throw std::runtime_error("Invalid string length in packet");
        }
        std::string str(reinterpret_cast<const char*>(buffer.data() + readPos), len);
        readPos += len;
        return str;
    }

    double readDouble() {
        if (readPos + 8 > buffer.size()) throw std::runtime_error("Buffer underflow reading double");
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) {
            raw = (raw << 8) | buffer[readPos++];
        }
        double val;
        std::memcpy(&val, &raw, sizeof(double));
        return val;
    }

    float readFloat() {
        if (readPos + 4 > buffer.size()) throw std::runtime_error("Buffer underflow reading float");
        uint32_t raw = 0;
        for (int i = 0; i < 4; ++i) {
            raw = (raw << 8) | buffer[readPos++];
        }
        float val;
        std::memcpy(&val, &raw, sizeof(float));
        return val;
    }

    bool readBool() {
        return readByte() != 0;
    }

    size_t remaining() const {
        return buffer.size() > readPos ? buffer.size() - readPos : 0;
    }
};

} // namespace mc
