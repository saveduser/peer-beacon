# PeerBeacon — Agent Guide

## Build

Builds both apps (responder + client):
```bash
make                     # bin-linux/register_bin + bin-linux/checkin_bin
make test                # builds + runs test binaries
make clean
```

Cross-compilation:
```bash
make OS_TARGET=windows   # bin-windows/register.exe + bin-windows/checkin.exe (needs MinGW)
./build_android.sh       # builds 3 Android .so → build/android/
```

## Architecture

- **LAN discovery** using **UDP multicast** on `239.255.42.99:7843`
- **Public API** (`src/discovery.h`): `discoverDeviceIpByName()`, `startResponder()`, `stopResponder()`, `isResponderRunning()`. Legacy signatures preserved.
- **Orchestrator** (`DiscoveryService` singleton) owns: responder thread, UdpSocket, NetworkMonitor. Only one responder runs at a time.
- **Class hierarchy** (all have .cpp implementations):
  - `UdpSocket` — RAII socket wrapper, move-only, owns open/bind/join/leave/send/recv
  - `DiscoveryPacket` — wire protocol serialization (legacy text + binary)
  - `NetworkMonitor` — background thread polls interfaces, fires callback on change → triggers responder re-bind
  - `socket_platform.h/.cpp` — all platform divergence (`#ifdef _WIN32`), interface enumeration, error helpers
- `register/main.cpp` — responder app (signal-safe shutdown); `checkin/main.cpp` — combined responder + discovery client
- **C++17**, compiler: `g++` (Linux), `x86_64-w64-mingw32-g++` (Windows), NDK clang (Android)
- No linter, formatter, CI, or package manager config exists.

## Windows multicast gotchas

Discovery failing on Windows? Two required fixes were applied to `discovery_service.cpp`:

1. **`IP_MULTICAST_IF`**: On Windows, sending to a multicast group requires the outgoing interface to be set explicitly via `setsockopt(IP_MULTICAST_IF)`. Without it, packets may go out the wrong NIC (e.g. Ethernet instead of WiFi). The discovery client now enumerates interfaces and sets this.

2. **Per-interface multicast join**: Using `INADDR_ANY` for `IP_ADD_MEMBERSHIP` may not bind the group to the active WiFi interface on multi-NIC machines. `rebindResponder()` now enumerates active non-loopback interfaces and joins multicast on each explicitly, falling back to `INADDR_ANY` only if enumeration fails.

These are in `discovery_service.cpp` — see `rebindResponder()` and `discoverDeviceIpByName()`.

## Gotchas

- `-Isrc` required for any compilation — resolves `#include "discovery.h"`, `#include "platform/socket_platform.h"` etc.
- `obj-<target>/` and `bin-<target>/` are used per OS target (`obj-linux`, `bin-windows`, etc.) to prevent stale-object contamination. Always `make clean` between platform switches.
- `register/main.cpp` uses `std::signal` for graceful shutdown; `stopResponder()` joins the responder thread
- Android build script uses `-Isrc` (not `-Iinclude` — old bug fixed) and compiles both `src/*.cpp` and `src/platform/*.cpp`
- `.gitignore` covers `obj*`, `bin*`, `build`
