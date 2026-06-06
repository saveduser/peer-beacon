#ifndef PEERBEACON_UDP_SOCKET_H
#define PEERBEACON_UDP_SOCKET_H

#include "platform/socket_platform.h"
#include <string>
#include <chrono>
#include <cstdint>

class UdpSocket {
public:
    UdpSocket() noexcept = default;
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;

    bool open();
    void close() noexcept;
    bool isOpen() const noexcept { return sock_ != kInvalidSocket; }
    SocketHandle nativeHandle() const noexcept { return sock_; }

    bool setBroadcast(bool enable) noexcept;
    bool setReuseAddress(bool enable) noexcept;
    bool setReceiveTimeout(std::chrono::milliseconds timeout) noexcept;
    bool setMulticastTtl(uint8_t ttl) noexcept;
    bool setMulticastLoop(bool enable) noexcept;
    bool setMulticastInterface(const std::string& ifAddr) noexcept;

    bool bind(uint16_t port) noexcept;
    bool bind(const std::string& ip, uint16_t port) noexcept;

    bool joinMulticastGroup(const std::string& groupAddr,
                            const std::string& ifAddr = "") noexcept;
    bool leaveMulticastGroup(const std::string& groupAddr,
                             const std::string& ifAddr = "") noexcept;

    int sendTo(const void* data, size_t len,
               const struct sockaddr_in& dest) noexcept;
    int sendTo(const std::string& message,
               const struct sockaddr_in& dest) noexcept;
    int sendTo(const std::string& message,
               const std::string& ip, uint16_t port) noexcept;

    int receiveFrom(void* buffer, size_t bufferSize,
                    struct sockaddr_in& sender,
                    int flags = 0) noexcept;

private:
    SocketHandle sock_ = kInvalidSocket;
};

#endif
