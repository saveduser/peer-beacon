#include "discovery.h"
#include <string>
#include <thread>
#include <iostream>

int main() {
    std::cout << "main started\n";
    std::string selfName;
    std::cout << "Enter name to start broadcasting\n";
    std::cin >> selfName;

    std::string searchName;
    std::cout << "Enter name to search\n";
    std::cin >> searchName;

    int waitTime = 5;
    std::cout << "Enter wait time in seconds\n";
    std::cin >> waitTime;
    if (waitTime < 1) waitTime = 1;

    startResponder(selfName);
    if (!isResponderRunning()) {
        std::cerr << "Failed to start responder\n";
        return 1;
    }

    std::cout << "Searching for " << searchName << "...\n";
    DiscoverResult res = discoverDeviceIpByName(searchName, std::chrono::seconds(waitTime));

    if (res.isSuccess()) {
        std::cout << "found ip " << res.ipAddress << "\n";
    } else {
        std::cout << "device not found\n";
    }

    stopResponder();
    return 0;
}
