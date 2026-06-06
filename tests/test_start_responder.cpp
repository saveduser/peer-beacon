#include "discovery.h"
#include <string>
#include <cstdio>
#include <thread>
#include <chrono>

int main() {
    printf("starting responder\n");
    startResponder("test_device");
    if (!isResponderRunning()) {
        printf("responder already running or failed\n");
        return 1;
    }
    printf("responder started, waiting 3 seconds...\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    printf("stopping responder\n");
    stopResponder();
    printf("done\n");
    return 0;
}
