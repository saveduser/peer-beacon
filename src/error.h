#ifndef PEERBEACON_ERROR_H
#define PEERBEACON_ERROR_H

#include <string>

enum class DiscoveryError {
    SUCCESS = 0,
    TIMEOUT,
    NETWORK_ERROR,
    ALREADY_RUNNING,
    NOT_RUNNING,
    INVALID_ARGUMENT,
    INTERNAL_ERROR
};

struct DiscoverResult {
    std::string ipAddress;
    DiscoveryError error = DiscoveryError::SUCCESS;
    std::string errorMessage;

    bool isSuccess() const { return error == DiscoveryError::SUCCESS; }
    explicit operator bool() const { return isSuccess(); }
};

#endif
