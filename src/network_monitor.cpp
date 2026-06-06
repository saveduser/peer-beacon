#include "network_monitor.h"
#include <algorithm>

NetworkMonitor::NetworkMonitor(std::chrono::seconds pollInterval)
    : pollInterval_(pollInterval) {}

NetworkMonitor::~NetworkMonitor() {
    stop();
}

bool NetworkMonitor::start(Callback onChange) noexcept {
    if (running_.load()) return false;
    callback_ = std::move(onChange);
    running_ = true;

    platform::enumerateInterfaces(cachedInterfaces_);

    thread_ = std::thread(&NetworkMonitor::threadFunc, this);
    return true;
}

void NetworkMonitor::stop() noexcept {
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}

bool NetworkMonitor::hasNetworkChanged() {
    std::vector<platform::InterfaceInfo> current;
    if (!platform::enumerateInterfaces(current))
        return false;

    std::lock_guard<std::mutex> lock(cacheMutex_);
    if (current.size() != cachedInterfaces_.size()) {
        cachedInterfaces_ = std::move(current);
        return true;
    }
    for (size_t i = 0; i < current.size(); i++) {
        if (current[i].name != cachedInterfaces_[i].name ||
            current[i].ipAddress != cachedInterfaces_[i].ipAddress ||
            current[i].isUp != cachedInterfaces_[i].isUp) {
            cachedInterfaces_ = std::move(current);
            return true;
        }
    }
    return false;
}

void NetworkMonitor::threadFunc() noexcept {
    while (running_.load()) {
        if (hasNetworkChanged()) {
            if (callback_)
                callback_();
        }
        for (int i = 0; i < 10 && running_.load(); i++) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(pollInterval_.count() * 100));
        }
    }
}
