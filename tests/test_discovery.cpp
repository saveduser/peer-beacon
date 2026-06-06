#include "discovery.h"
#include <string>
#include <cstdio>

int main() {
    std::string foundIP = discoverDeviceIpByName("resgiter", 5);
    if (!foundIP.empty()) {
        printf("found ip: %s\n", foundIP.c_str());
    } else {
        printf("device not found\n");
    }
    return 0;
}
