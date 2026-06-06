#ifndef PEERBEACON_DISCOVERY_H
#define PEERBEACON_DISCOVERY_H

#include <string>
#include <chrono>
#include "error.h"
#include <functional>

// Legacy API (backward-compatible signatures)
std::string discoverDeviceIpByName(const std::string& name, int timeoutSeconds);
void startResponder(const std::string& name);
int startResponderV2(const std::string& deviceName);
int startResponderV2(const std::string& deviceName, std::function<void()> onNetworkChange);

// Enhanced API
DiscoverResult discoverDeviceIpByName(const std::string& name,
                                       std::chrono::seconds timeout);
void stopResponder();
bool isResponderRunning();

#endif
