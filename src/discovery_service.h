#ifndef PEERBEACON_DISCOVERY_SERVICE_H
#define PEERBEACON_DISCOVERY_SERVICE_H

#include "udp_socket.h"
#include "packet.h"
#include "discovery_config.h"
#include "network_monitor.h"
#include "error.h"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>

class DiscoveryService {
public:
    DiscoveryService();
    ~DiscoveryService();

    DiscoveryService(const DiscoveryService&) = delete;
    DiscoveryService& operator=(const DiscoveryService&) = delete;

    void configure(const DiscoveryConfig& config);

    bool startResponder(const std::string& deviceName);
    bool startResponder(const std::string& deviceName, std::function<void()> onNetworkChange);
    void stopResponder();
    bool isResponderRunning() const noexcept;

    DiscoverResult discoverDeviceIpByName(
        const std::string& deviceName,
        std::chrono::seconds timeout);

private:
    void responderLoop(const std::string& deviceName) noexcept;
    bool rebindResponder(const std::string& deviceName) noexcept;

    DiscoveryConfig config_;

    std::thread responderThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> needsRebind_{false};

    UdpSocket responderSocket_;

    NetworkMonitor networkMonitor_;
};

#endif
