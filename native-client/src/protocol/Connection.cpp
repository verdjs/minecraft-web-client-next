#include "Connection.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace mc {

Connection::Connection() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

Connection::~Connection() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool Connection::connect(const std::string& host, uint16_t port) {
    disconnect();

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        std::cerr << "[Network] Failed to resolve host: " << host << std::endl;
        return false;
    }

    socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketFd < 0) {
        freeaddrinfo(res);
        return false;
    }

    if (::connect(socketFd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[Network] Failed to connect to: " << host << ":" << port << std::endl;
#ifdef _WIN32
        closesocket(socketFd);
#else
        close(socketFd);
#endif
        socketFd = -1;
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    state = ConnectionState::Handshaking;
    running = true;

    networkThread = std::thread(&Connection::networkLoop, this);
    return true;
}

void Connection::disconnect() {
    running = false;
    if (socketFd >= 0) {
#ifdef _WIN32
        closesocket(socketFd);
#else
        close(socketFd);
#endif
        socketFd = -1;
    }
    if (networkThread.joinable()) {
        networkThread.join();
    }
    state = ConnectionState::Disconnected;
}

bool Connection::rawSend(const uint8_t* data, size_t len) {
    if (socketFd < 0) return false;
    size_t totalSent = 0;
    while (totalSent < len) {
        int sent = send(socketFd, reinterpret_cast<const char*>(data + totalSent), static_cast<int>(len - totalSent), 0);
        if (sent <= 0) return false;
        totalSent += sent;
    }
    return true;
}

void Connection::sendPacket(int packetId, const PacketBuffer& payload) {
    PacketBuffer packet;
    PacketBuffer packetBody;

    packetBody.writeVarInt(packetId);
    packetBody.writeBytes(payload.buffer.data(), payload.buffer.size());

    // Framed with VarInt total length
    packet.writeVarInt(static_cast<int32_t>(packetBody.buffer.size()));
    packet.writeBytes(packetBody.buffer.data(), packetBody.buffer.size());

    rawSend(packet.buffer.data(), packet.buffer.size());
}

void Connection::sendHandshake(const std::string& host, uint16_t port, int protocolVersion, int nextState) {
    PacketBuffer buf;
    buf.writeVarInt(protocolVersion);
    buf.writeString(host);
    buf.writeUShort(port);
    buf.writeVarInt(nextState);

    sendPacket(0x00, buf);
    state = (nextState == 2) ? ConnectionState::Login : ConnectionState::Play;
}

void Connection::sendLoginStart(const std::string& username) {
    PacketBuffer buf;
    buf.writeString(username);
    // UUID flag: false
    buf.writeBool(false);
    sendPacket(0x00, buf);
}

void Connection::networkLoop() {
    std::vector<uint8_t> recvBuf(65536);

    while (running) {
        int bytes = recv(socketFd, reinterpret_cast<char*>(recvBuf.data()), static_cast<int>(recvBuf.size()), 0);
        if (bytes <= 0) {
            break;
        }

        PacketBuffer buf(std::vector<uint8_t>(recvBuf.begin(), recvBuf.begin() + bytes));
        try {
            while (buf.remaining() > 0) {
                size_t startPos = buf.readPos;
                int32_t packetLen = buf.readVarInt();
                if (buf.remaining() < static_cast<size_t>(packetLen)) {
                    // Incomplete packet in this read slice
                    break;
                }

                int32_t packetId = buf.readVarInt();
                size_t headerBytes = buf.readPos - startPos;
                size_t dataLen = packetLen - (headerBytes - (buf.readPos - startPos));

                std::vector<uint8_t> packetData(buf.buffer.begin() + buf.readPos, buf.buffer.begin() + buf.readPos + dataLen);
                buf.readPos += dataLen;

                std::lock_guard<std::mutex> lock(incomingMutex);
                incomingPackets.emplace(packetId, std::move(packetData));
            }
        } catch (...) {
            // Malformed frame or buffer slice
        }
    }

    state = ConnectionState::Disconnected;
}

void Connection::update() {
    std::queue<std::pair<int, std::vector<uint8_t>>> packets;
    {
        std::lock_guard<std::mutex> lock(incomingMutex);
        packets.swap(incomingPackets);
    }

    while (!packets.empty()) {
        auto [id, data] = std::move(packets.front());
        packets.pop();

        PacketBuffer buf(std::move(data));
        if (onPacket) {
            onPacket(id, buf);
        }
    }
}

} // namespace mc
