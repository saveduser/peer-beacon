#include "discovery.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

static std::atomic<bool> g_stop{false};

extern "C" void signalHandler(int) {
    g_stop = true;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "main started\n";
    std::string name;
    std::cout << "Enter name to start broadcasting\n";
    std::cin >> name;

    int result = startResponderV2(name, []() {
        std::cout << "Network change detected, rebinding responder...\n";
    });
    if (result != 0) {
        if (result == -1) {
            std::cerr << "A device with the same name is already broadcasting\n";
            return 1;
        } else {
            std::cerr << "Failed to start responder\n";
                   return 1;
        }
 
    }

    std::cout << "Broadcaster started with name " << name << "\n";
    std::cout << "Press Ctrl+C to stop\n";

    while (!g_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down...\n";
    stopResponder();
    return 0;
}
