#ifndef PEERBEACON_SOCKET_PLATFORM_H
#define PEERBEACON_SOCKET_PLATFORM_H

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SocketHandle = SOCKET;
    constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <net/if.h>
    #if !defined(__ANDROID__) || __ANDROID_API__ >= 24
        #include <ifaddrs.h>
    #endif
    using SocketHandle = int;
    constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace platform {

struct InterfaceInfo {
    std::string name;
    std::string ipAddress;
    bool isUp;
    bool isLoopback;
};

void initializeSockets();
void cleanupSockets();
void closeSocket(SocketHandle sock);
bool isSocketTimeout();
std::string getLastSocketError();
int getLastSocketErrorCode();
bool enumerateInterfaces(std::vector<InterfaceInfo>& out);

} // namespace platform

#endif
