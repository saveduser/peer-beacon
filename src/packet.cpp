#include "packet.h"
#include <cstring>

std::vector<uint8_t> DiscoveryPacket::serialize() const {
    std::vector<uint8_t> data;
    data.push_back(0x01); // version
    data.push_back(static_cast<uint8_t>(type));
    uint16_t nameLen = static_cast<uint16_t>(deviceName.size());
    data.push_back(static_cast<uint8_t>(nameLen >> 8));
    data.push_back(static_cast<uint8_t>(nameLen & 0xFF));
    data.insert(data.end(), deviceName.begin(), deviceName.end());
    return data;
}

bool DiscoveryPacket::deserialize(const std::vector<uint8_t>& data,
                                   DiscoveryPacket& out) {
    if (data.size() < 4) return false;
    if (data[0] != 0x01) return false;
    out.type = static_cast<PacketType>(data[1]);
    uint16_t nameLen = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    if (data.size() < static_cast<size_t>(4) + nameLen) return false;
    out.deviceName.assign(data.begin() + 4, data.begin() + 4 + nameLen);
    return true;
}

std::string DiscoveryPacket::serializeLegacy() const {
    std::string prefix = (type == PacketType::DISCOVERY_REQUEST)
                             ? "find:"
                             : "iam:";
    return prefix + deviceName;
}

bool DiscoveryPacket::deserializeLegacy(const std::string& raw,
                                         DiscoveryPacket& out) {
    if (raw.size() < 5) return false;
    if (raw.compare(0, 5, "find:") == 0) {
        out.type = PacketType::DISCOVERY_REQUEST;
        out.deviceName = raw.substr(5);
        return true;
    }
    if (raw.compare(0, 4, "iam:") == 0) {
        out.type = PacketType::DISCOVERY_RESPONSE;
        out.deviceName = raw.substr(4);
        return true;
    }
    return false;
}

DiscoveryPacket DiscoveryPacket::makeRequest(const std::string& name) {
    return {PacketType::DISCOVERY_REQUEST, name};
}

DiscoveryPacket DiscoveryPacket::makeResponse(const std::string& name) {
    return {PacketType::DISCOVERY_RESPONSE, name};
}
