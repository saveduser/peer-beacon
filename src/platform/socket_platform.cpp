#include "socket_platform.h"
#include <cstring>
#include <map>

#if !defined(_WIN32) && defined(__ANDROID__) && __ANDROID_API__ < 24
    #include <sys/ioctl.h>
#endif

#ifdef _WIN32
#include <iphlpapi.h>

struct WinSockGlobal {
    WinSockGlobal() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
    }
    ~WinSockGlobal() {
        WSACleanup();
    }
};
static WinSockGlobal g_winsock;
#endif

namespace platform {

void initializeSockets() {}
void cleanupSockets() {}

void closeSocket(SocketHandle sock) {
    if (sock != kInvalidSocket) {
#ifdef _WIN32
        closesocket(sock);
#else
        ::close(sock);
#endif
    }
}

bool isSocketTimeout() {
#ifdef _WIN32
    int err = WSAGetLastError();
    return err == WSAETIMEDOUT || err == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

int getLastSocketErrorCode() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string getLastSocketError() {
#ifdef _WIN32
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, WSAGetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (wchar_t*)&buf, 0, NULL);
    std::wstring ws(buf);
    LocalFree(buf);
    return std::string(ws.begin(), ws.end());
#else
    return std::string(::strerror(errno));
#endif
}

bool enumerateInterfaces(std::vector<InterfaceInfo>& out) {
    out.clear();
#ifdef _WIN32
    ULONG bufLen = 15000;
    std::vector<char> buf(bufLen);
    PIP_ADAPTER_ADDRESSES adapters = (PIP_ADAPTER_ADDRESSES)buf.data();
    DWORD ret = GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME,
        NULL, adapters, &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(bufLen);
        adapters = (PIP_ADAPTER_ADDRESSES)buf.data();
        ret = GetAdaptersAddresses(AF_INET,
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME,
            NULL, adapters, &bufLen);
    }
    if (ret != NO_ERROR) return false;
    for (auto* a = adapters; a; a = a->Next) {
        InterfaceInfo info;
        info.name = a->AdapterName;
        info.isLoopback = (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK);
        info.isUp = (a->OperStatus == IfOperStatusUp);
        for (auto* addr = a->FirstUnicastAddress; addr; addr = addr->Next) {
            if (addr->Address.lpSockaddr &&
                addr->Address.lpSockaddr->sa_family == AF_INET) {
                char buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,
                    &((sockaddr_in*)addr->Address.lpSockaddr)->sin_addr,
                    buf, sizeof(buf));
                info.ipAddress = buf;
                break;
            }
        }
        if (!info.ipAddress.empty())
            out.push_back(std::move(info));
    }
    return true;
#elif defined(__ANDROID__) && __ANDROID_API__ < 24
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    int num = 10;
    std::vector<char> buf;
    struct ifconf ifc{};
    for (;;) {
        buf.resize(sizeof(struct ifreq) * num);
        ifc.ifc_len = static_cast<int>(buf.size());
        ifc.ifc_buf = buf.data();
        if (::ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
            ::close(sock);
            return false;
        }
        if (ifc.ifc_len < static_cast<int>(buf.size())) break;
        num *= 2;
    }

    struct ifreq* ifr = ifc.ifc_req;
    int count = ifc.ifc_len / sizeof(struct ifreq);
    for (int i = 0; i < count; i++) {
        InterfaceInfo info;
        info.name = ifr[i].ifr_name;

        struct ifreq flagsReq = ifr[i];
        if (::ioctl(sock, SIOCGIFFLAGS, &flagsReq) == 0) {
            info.isUp = (flagsReq.ifr_flags & IFF_UP) != 0;
            info.isLoopback = (flagsReq.ifr_flags & IFF_LOOPBACK) != 0;
        }

        if (ifr[i].ifr_addr.sa_family == AF_INET) {
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,
                &((struct sockaddr_in*)&ifr[i].ifr_addr)->sin_addr,
                addr, sizeof(addr));
            info.ipAddress = addr;
        }

        if (!info.ipAddress.empty())
            out.push_back(std::move(info));
    }

    ::close(sock);
    return true;
#else
    struct ifaddrs* ifa = nullptr;
    if (getifaddrs(&ifa) < 0) return false;
    std::map<std::string, InterfaceInfo> map;
    for (struct ifaddrs* i = ifa; i; i = i->ifa_next) {
        if (!i->ifa_name) continue;
        InterfaceInfo& info = map[i->ifa_name];
        if (info.name.empty()) info.name = i->ifa_name;
        info.isUp = (i->ifa_flags & IFF_UP) != 0;
        info.isLoopback = (i->ifa_flags & IFF_LOOPBACK) != 0;
        if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,
                &((struct sockaddr_in*)i->ifa_addr)->sin_addr,
                buf, sizeof(buf));
            info.ipAddress = buf;
        }
    }
    freeifaddrs(ifa);
    for (auto& [name, info] : map) {
        if (!info.ipAddress.empty())
            out.push_back(std::move(info));
    }
    return true;
#endif
}

} // namespace platform
