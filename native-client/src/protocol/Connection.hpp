#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include "PacketBuffer.hpp"

namespace mc {

enum class ConnectionState {
    Disconnected,
    Handshaking,
    Login,
    Play
};

class Connection {
public:
    Connection();
    ~Connection();

    bool connect(const std::string& host, uint16_t port);
    void disconnect();

    void sendPacket(int packetId, const PacketBuffer& payload);
    void sendHandshake(const std::string& host, uint16_t port, int protocolVersion, int nextState);
    void sendLoginStart(const std::string& username);

    void update();

    bool isConnected() const { return socketFd >= 0 && state != ConnectionState::Disconnected; }
    ConnectionState getState() const { return state; }

    std::function<void(int packetId, PacketBuffer& buffer)> onPacket;
    std::function<void(const std::string& reason)> onDisconnect;

private:
    int socketFd{-1};
    std::atomic<ConnectionState> state{ConnectionState::Disconnected};
    std::thread networkThread;
    std::atomic<bool> running{false};

    std::mutex incomingMutex;
    std::queue<std::pair<int, std::vector<uint8_t>>> incomingPackets;

    void networkLoop();
    bool rawSend(const uint8_t* data, size_t len);
};

} // namespace mc
