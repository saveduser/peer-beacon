#include "discovery.h"
#include "discovery_service.h"
#include "discovery_config.h"
#include "error.h"

static DiscoveryService& service() {
    static DiscoveryService s;
    return s;
}

std::string discoverDeviceIpByName(const std::string& name, int timeoutSeconds) {
    auto result = service().discoverDeviceIpByName(
        name, std::chrono::seconds(timeoutSeconds));
        if (!result.isSuccess()) {
            std::cerr << "Discovery failed: " << result.errorMessage << "\n";
            return "";
        }
    return result.ipAddress;
}

DiscoverResult discoverDeviceIpByName(const std::string& name,
                                       std::chrono::seconds timeout) {
    return service().discoverDeviceIpByName(name, timeout);
}

void startResponder(const std::string& name) {
    service().startResponder(name);
}


int startResponderV2(const std::string& name) {
    if(service().discoverDeviceIpByName(name, std::chrono::seconds(5)).isSuccess()) {
        return -1;
    }
    service().startResponder(name);
    return 0;
}
int startResponderV2(const std::string& deviceName, std::function<void()> onNetworkChange){
    if(service().discoverDeviceIpByName(deviceName, std::chrono::seconds(5)).isSuccess()) {
        return -1;
    }
    service().startResponder(deviceName, onNetworkChange);
    return 0;
}
void stopResponder() {
    service().stopResponder();
}

bool isResponderRunning() {
    return service().isResponderRunning();
}
