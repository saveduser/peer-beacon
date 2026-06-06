#include "udp_socket.h"
#include <cstring>

#ifdef _WIN32
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

static void disableUdpConnReset(SocketHandle sock) {
    DWORD bytesReturned = 0;
    BOOL newBehavior = FALSE;
    WSAIoctl(sock, SIO_UDP_CONNRESET,
             &newBehavior, sizeof(newBehavior),
             NULL, 0, &bytesReturned, NULL, NULL);
}
#endif

UdpSocket::~UdpSocket() {
    close();
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : sock_(other.sock_) {
    other.sock_ = kInvalidSocket;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        sock_ = other.sock_;
        other.sock_ = kInvalidSocket;
    }
    return *this;
}

bool UdpSocket::open() {
    close();
    sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == kInvalidSocket) return false;
#ifdef _WIN32
    disableUdpConnReset(sock_);
#endif
    return true;
}

void UdpSocket::close() noexcept {
    if (sock_ != kInvalidSocket) {
        platform::closeSocket(sock_);
        sock_ = kInvalidSocket;
    }
}

bool UdpSocket::setBroadcast(bool enable) noexcept {
    int opt = enable ? 1 : 0;
    return ::setsockopt(sock_, SOL_SOCKET, SO_BROADCAST,
                        (const char*)&opt, sizeof(opt)) == 0;
}

bool UdpSocket::setReuseAddress(bool enable) noexcept {
    int opt = enable ? 1 : 0;
    return ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR,
                        (const char*)&opt, sizeof(opt)) == 0;
}

bool UdpSocket::setReceiveTimeout(std::chrono::milliseconds timeout) noexcept {
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout.count());
#else
    struct timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
#endif
    return ::setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO,
                        (const char*)&tv, sizeof(tv)) == 0;
}

bool UdpSocket::setMulticastTtl(uint8_t ttl) noexcept {
#ifdef _WIN32
    DWORD opt = ttl;
#else
    int opt = ttl;
#endif
    return ::setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_TTL,
                        (const char*)&opt, sizeof(opt)) == 0;
}

bool UdpSocket::setMulticastLoop(bool enable) noexcept {
#ifdef _WIN32
    DWORD opt = enable ? 1 : 0;
#else
    int opt = enable ? 1 : 0;
#endif
    return ::setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_LOOP,
                        (const char*)&opt, sizeof(opt)) == 0;
}

bool UdpSocket::setMulticastInterface(const std::string& ifAddr) noexcept {
    struct in_addr addr{};
    if (inet_pton(AF_INET, ifAddr.c_str(), &addr) != 1)
        return false;
    return ::setsockopt(sock_, IPPROTO_IP, IP_MULTICAST_IF,
                        (const char*)&addr, sizeof(addr)) == 0;
}

bool UdpSocket::bind(uint16_t port) noexcept {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return ::bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

bool UdpSocket::bind(const std::string& ip, uint16_t port) noexcept {
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    addr.sin_port = htons(port);
    return ::bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

bool UdpSocket::joinMulticastGroup(const std::string& groupAddr,
                                   const std::string& ifAddr) noexcept {
    struct ip_mreq mreq{};
    inet_pton(AF_INET, groupAddr.c_str(), &mreq.imr_multiaddr);
    if (!ifAddr.empty()) {
        inet_pton(AF_INET, ifAddr.c_str(), &mreq.imr_interface);
    } else {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    return ::setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        (const char*)&mreq, sizeof(mreq)) == 0;
}

bool UdpSocket::leaveMulticastGroup(const std::string& groupAddr,
                                    const std::string& ifAddr) noexcept {
    struct ip_mreq mreq{};
    inet_pton(AF_INET, groupAddr.c_str(), &mreq.imr_multiaddr);
    if (!ifAddr.empty()) {
        inet_pton(AF_INET, ifAddr.c_str(), &mreq.imr_interface);
    } else {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    return ::setsockopt(sock_, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                        (const char*)&mreq, sizeof(mreq)) == 0;
}

int UdpSocket::sendTo(const void* data, size_t len,
                      const struct sockaddr_in& dest) noexcept {
    return ::sendto(sock_, (const char*)data, len, 0,
                    (const struct sockaddr*)&dest, sizeof(dest));
}

int UdpSocket::sendTo(const std::string& message,
                      const struct sockaddr_in& dest) noexcept {
    return sendTo(message.data(), message.size(), dest);
}

int UdpSocket::sendTo(const std::string& message,
                      const std::string& ip, uint16_t port) noexcept {
    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);
    dest.sin_port = htons(port);
    return sendTo(message, dest);
}

int UdpSocket::receiveFrom(void* buffer, size_t bufferSize,
                           struct sockaddr_in& sender,
                           int flags) noexcept {
    socklen_t fromLen = sizeof(sender);
    return ::recvfrom(sock_, (char*)buffer, bufferSize, flags,
                      (struct sockaddr*)&sender, &fromLen);
}
