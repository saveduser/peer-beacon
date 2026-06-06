#ifndef PEERBEACON_DISCOVERY_CONFIG_H
#define PEERBEACON_DISCOVERY_CONFIG_H

#include <string>
#include <chrono>

struct DiscoveryConfig {
    std::string multicastGroup = "239.255.42.99";
    uint16_t port = 7843;

    std::chrono::seconds discoverTimeout{5};
    std::chrono::milliseconds responderPollInterval{1000};
    std::chrono::seconds networkMonitorInterval{5};

    uint8_t multicastTtl = 4;
    bool multicastLoop = false;
};

#endif
