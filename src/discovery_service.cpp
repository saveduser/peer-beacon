#include "discovery_service.h"
#include <chrono>

DiscoveryService::DiscoveryService()
    : networkMonitor_(config_.networkMonitorInterval) {}

DiscoveryService::~DiscoveryService() {
    stopResponder();
}

void DiscoveryService::configure(const DiscoveryConfig& config) {
    config_ = config;
}
bool DiscoveryService::startResponder(const std::string& deviceName) {
    if (running_.load()) {
        std::cerr << "Responder already running\n";
        return false;
    }

    if (deviceName.empty()) {
        std::cerr << "Device name cannot be empty\n";
        return false;
    }

    running_ = true;
    needsRebind_ = false;

    networkMonitor_.start([this]() {
        needsRebind_ = true;
    });

    responderThread_ = std::thread(&DiscoveryService::responderLoop, this, deviceName);
    return true;
}
bool DiscoveryService::startResponder(const std::string& deviceName, std::function<void()> onNetworkChange) {
    if (running_.load()) {
        std::cerr << "Responder already running\n";
        return false;
    }

    if (deviceName.empty()) {
        std::cerr << "Device name cannot be empty\n";
        return false;
    }

    running_ = true;
    needsRebind_ = false;

    networkMonitor_.start([this, onNetworkChange]() {
        needsRebind_ = true;
        onNetworkChange();
    });

    responderThread_ = std::thread(&DiscoveryService::responderLoop, this, deviceName);
    return true;
}

void DiscoveryService::stopResponder() {
    running_ = false;
    networkMonitor_.stop();
    if (responderThread_.joinable())
        responderThread_.join();
}

bool DiscoveryService::isResponderRunning() const noexcept {
    return running_.load();
}

bool DiscoveryService::rebindResponder(const std::string& deviceName) noexcept {
    responderSocket_.close();

    if (!responderSocket_.open()) {
        std::cerr << "Failed to create responder socket\n";
        return false;
    }

    responderSocket_.setReuseAddress(true);
    responderSocket_.setMulticastTtl(config_.multicastTtl);
    responderSocket_.setMulticastLoop(config_.multicastLoop);

    if (!responderSocket_.bind(config_.port)) {
        std::cerr << "Failed to bind responder to port " << config_.port << "\n";
        responderSocket_.close();
        return false;
    }

    // Join multicast on each active non-loopback interface explicitly.
    // On Windows, INADDR_ANY may not bind to the active WiFi interface.
    std::vector<platform::InterfaceInfo> ifaces;
    bool joinedAny = false;
    if (platform::enumerateInterfaces(ifaces)) {
        for (const auto& iface : ifaces) {
            if (iface.isUp && !iface.isLoopback && !iface.ipAddress.empty()) {
                responderSocket_.setMulticastInterface(iface.ipAddress);
                if (responderSocket_.joinMulticastGroup(
                        config_.multicastGroup, iface.ipAddress)) {
                    joinedAny = true;
                }
            }
        }
    }
    if (!joinedAny) {
        // Fallback: join with INADDR_ANY
        if (!responderSocket_.joinMulticastGroup(config_.multicastGroup)) {
            std::cerr << "Failed to join multicast group " << config_.multicastGroup << "\n";
            responderSocket_.close();
            return false;
        }
    }

    responderSocket_.setReceiveTimeout(config_.responderPollInterval);

    std::cout << "advertising " << deviceName << " to local lan on "
              << config_.multicastGroup << ":" << config_.port << "\n";
    return true;
}

void DiscoveryService::responderLoop(const std::string& deviceName) noexcept {
    if (!rebindResponder(deviceName))
        return;

    char buffer[1024];

    while (running_.load()) {
        if (needsRebind_.load()) {
            needsRebind_ = false;
            rebindResponder(deviceName);
            continue;
        }

        struct sockaddr_in clientAddr{};
        int len = responderSocket_.receiveFrom(buffer, sizeof(buffer) - 1, clientAddr);

        if (len < 0) {
            if (platform::isSocketTimeout())
                continue;

            std::cerr << "recvfrom failed: " << platform::getLastSocketError() << "\n";
            continue;
        }

        if (len == 0)
            continue;

        buffer[len] = '\0';
        std::string msg(buffer);

        DiscoveryPacket packet;
        if (DiscoveryPacket::deserializeLegacy(msg, packet) &&
            packet.type == PacketType::DISCOVERY_REQUEST &&
            packet.deviceName == deviceName) {

            DiscoveryPacket response = DiscoveryPacket::makeResponse(deviceName);
            std::string reply = response.serializeLegacy();
            responderSocket_.sendTo(reply, clientAddr);
            std::cout << "responded to discovery request\n";
        }
    }

    if (responderSocket_.isOpen()) {
        responderSocket_.leaveMulticastGroup(config_.multicastGroup);
        responderSocket_.close();
    }
}

DiscoverResult DiscoveryService::discoverDeviceIpByName(
    const std::string& deviceName,
    std::chrono::seconds timeout) {

    if (deviceName.empty()) {
        return {"", DiscoveryError::INVALID_ARGUMENT, "Device name is empty"};
    }

    UdpSocket sock;
    if (!sock.open()) {
        return {"", DiscoveryError::NETWORK_ERROR,
                "Failed to create socket: " + platform::getLastSocketError()};
    }

    sock.setBroadcast(true);
    sock.setMulticastTtl(config_.multicastTtl);
    sock.setReceiveTimeout(timeout);

    // Set outgoing interface explicitly for multicast send.
    // On Windows, the default interface may not be the active one (WiFi vs Ethernet).
    std::vector<platform::InterfaceInfo> ifaces;
    if (platform::enumerateInterfaces(ifaces)) {
        for (const auto& iface : ifaces) {
            if (iface.isUp && !iface.isLoopback && !iface.ipAddress.empty()) {
                sock.setMulticastInterface(iface.ipAddress);
                break;
            }
        }
    }

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(config_.port);
    inet_pton(AF_INET, config_.multicastGroup.c_str(), &dest.sin_addr);

    std::string req = DiscoveryPacket::makeRequest(deviceName).serializeLegacy();
    if (sock.sendTo(req, dest) < 0) {
        return {"", DiscoveryError::NETWORK_ERROR,
                "Failed to send discovery request: " + platform::getLastSocketError()};
    }

    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed >= timeout) {
            return {"", DiscoveryError::TIMEOUT,
                    "No response received within " + std::to_string(timeout.count()) + "s"};
        }

        struct sockaddr_in from{};
        char buffer[1024];
        int len = sock.receiveFrom(buffer, sizeof(buffer) - 1, from);

        if (len < 0) {
            if (platform::isSocketTimeout())
                continue;

            return {"", DiscoveryError::NETWORK_ERROR,
                    "Receive error: " + platform::getLastSocketError()};
        }

        if (len == 0)
            continue;

        buffer[len] = '\0';
        std::string response(buffer);

        DiscoveryPacket packet;
        if (DiscoveryPacket::deserializeLegacy(response, packet) &&
            packet.type == PacketType::DISCOVERY_RESPONSE &&
            packet.deviceName == deviceName) {

            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, ipStr, sizeof(ipStr));
            return {ipStr, DiscoveryError::SUCCESS, ""};
        }
    }
}
