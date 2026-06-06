# PeerBeacon

PeerBeacon is a lightweight C++17 local network peer discovery library using UDP multicast.

Devices on the same LAN can advertise themselves by name and discover peers by name,
resolving them to local IP addresses — no central server, DNS, or configuration needed.

## Features

- **Zero-config** LAN discovery via UDP multicast (`239.255.42.99:7843`)
- **Simple C API** — single-header include, no external dependencies
- **Dual-mode** — responder (advertise) and client (discover) in one library
- **Network-aware** — background `NetworkMonitor` polls interfaces and auto-rebinds on change
- **Cross-platform** — Linux (g++), Windows (MinGW cross-compile), Android (NDK shared lib)
- **Configurable** — multicast group, port, timeouts, TTL via `DiscoveryConfig`

## Building

```bash
make                     # builds both register_bin and checkin_bin (Linux)
make test                # builds and runs test binaries
make clean

make OS_TARGET=windows   # cross-compile for Windows (requires MinGW)
./build_android.sh       # builds 3 Android .so → build/android/
```

Output goes to `bin-linux/` or `bin-windows/`; object files to `obj-linux/` or `obj-windows/`.

## Applications

Two standalone apps are built:

| App        | Source           | Description                                    |
|------------|------------------|------------------------------------------------|
| `register` | `register/main.cpp` | Pure responder — broadcasts a name until Ctrl+C handles SIGINT/SIGTERM |
| `checkin`  | `checkin/main.cpp`  | Combined responder + client — advertises itself and discovers another peer |

### Example: checkin

```bash
./bin-linux/checkin_bin
# Enter name to start broadcasting: my-laptop
# Enter name to search: printer
# Enter wait time in seconds: 5
# Searching for printer...
# found ip 192.168.1.42
```

## API

```cpp
#include "discovery.h"
```

### Discovery

```cpp
// Legacy — returns empty string on failure
std::string discoverDeviceIpByName(const std::string& name, int timeoutSeconds);

// Enhanced — returns DiscoverResult with error info
DiscoverResult discoverDeviceIpByName(const std::string& name,
                                      std::chrono::seconds timeout);
```

### Responder

```cpp
// Legacy responder (single instance)
void startResponder(const std::string& name);

// Enhanced — returns 0 on success, -1 if name already taken, -2 on error
int startResponderV2(const std::string& deviceName);
int startResponderV2(const std::string& deviceName,
                     std::function<void()> onNetworkChange);

void stopResponder();
bool isResponderRunning();
```

### Result type

```cpp
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

    bool isSuccess() const;
    explicit operator bool() const;   // true when SUCCESS
};
```

### Configuration

```cpp
#include "discovery_config.h"

struct DiscoveryConfig {
    std::string multicastGroup = "239.255.42.99";
    uint16_t port = 7843;
    std::chrono::seconds discoverTimeout{5};
    std::chrono::milliseconds responderPollInterval{1000};
    std::chrono::seconds networkMonitorInterval{5};
    uint8_t multicastTtl = 4;
    bool multicastLoop = false;
};
```

## Architecture

The library is built around a `DiscoveryService` singleton that owns:

- **`UdpSocket`** — RAII socket wrapper, move-only, encapsulates open/bind/join/leave/send/recv
- **`DiscoveryPacket`** — wire protocol serialization (legacy text + binary)
- **`NetworkMonitor`** — background thread polls network interfaces, fires callback on change → triggers responder re-bind
- **Responder thread** — periodically broadcasts device name to the multicast group

Only one responder runs at a time.

### Platform layer

All platform divergence lives in `src/platform/socket_platform.h/.cpp`:
- Winsock vs POSIX socket APIs (via `#ifdef _WIN32`)
- Interface enumeration
- Error helpers

## Use cases

- IoT / smart home device discovery
- Local multiplayer game lobby discovery
- LAN file transfer peer finding
- Zero-config service discovery

## Agent / AI notes

See [`AGENTS.md`](./AGENTS.md) for build gotchas, Windows multicast fixes, and agent guidance.
