#ifndef PEERBEACON_NETWORK_MONITOR_H
#define PEERBEACON_NETWORK_MONITOR_H

#include "platform/socket_platform.h"
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>
#include <mutex>

class NetworkMonitor {
public:
    using Callback = std::function<void()>;

    explicit NetworkMonitor(
        std::chrono::seconds pollInterval = std::chrono::seconds{5});
    ~NetworkMonitor();

    NetworkMonitor(const NetworkMonitor&) = delete;
    NetworkMonitor& operator=(const NetworkMonitor&) = delete;

    bool start(Callback onChange) noexcept;
    void stop() noexcept;
    bool isRunning() const noexcept { return running_.load(); }

private:
    void threadFunc() noexcept;
    bool hasNetworkChanged();

    std::thread thread_;
    std::atomic<bool> running_{false};
    Callback callback_;
    std::chrono::seconds pollInterval_;

    std::mutex cacheMutex_;
    std::vector<platform::InterfaceInfo> cachedInterfaces_;
};

#endif
