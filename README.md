# PeerBeacon

PeerBeacon is a lightweight C++ local network peer discovery library.

It allows devices on the same LAN to:

- Advertise themselves with a custom name
- Discover devices by name
- Resolve peer names into local IP addresses

Perfect for:
- IoT systems
- Smart home devices
- Local multiplayer games
- Device synchronization
- Zero-config networking
- Service discovery

---

# Features

- Lightweight
- Simple API
- No central server required
- LAN-based peer discovery
- Android-compatible
- Cross-platform architecture
- Fast discovery response

---

# API

```cpp
#include <string>

std::string discoverDeviceIpByName(
    const std::string& name,
    int timeout
);

void startResponder(
    const std::string& name
);