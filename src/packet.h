#ifndef PEERBEACON_PACKET_H
#define PEERBEACON_PACKET_H

#include <string>
#include <vector>
#include <cstdint>

enum class PacketType : uint8_t {
    DISCOVERY_REQUEST = 0x01,
    DISCOVERY_RESPONSE = 0x02
};

struct DiscoveryPacket {
    PacketType type;
    std::string deviceName;

    std::vector<uint8_t> serialize() const;
    static bool deserialize(const std::vector<uint8_t>& data,
                            DiscoveryPacket& out);

    std::string serializeLegacy() const;
    static bool deserializeLegacy(const std::string& raw,
                                  DiscoveryPacket& out);

    static DiscoveryPacket makeRequest(const std::string& name);
    static DiscoveryPacket makeResponse(const std::string& name);
};

#endif
