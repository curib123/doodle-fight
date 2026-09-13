#pragma once

#include <cstdint>

namespace net {

constexpr std::uint32_t kProtocolMagic = 0x44464E31u; // "DFN1"
constexpr std::uint16_t kProtocolVersion = 1;
constexpr std::uint16_t kDefaultLanPort = 42042;
constexpr float kInputSendInterval = 1.0f / 30.0f;
constexpr float kSnapshotSendInterval = 1.0f / 20.0f;
constexpr float kDiscoveryInterval = 0.75f;
constexpr float kRoomTimeout = 3.0f;
constexpr float kConnectionTimeout = 6.0f;

enum class PacketType : std::uint8_t {
    DiscoveryRequest = 1,
    DiscoveryOffer = 2,
    JoinRequest = 3,
    JoinAccept = 4,
    Input = 5,
    Snapshot = 6,
    Disconnect = 7,
    Ping = 8,
    Pong = 9
};

#pragma pack(push, 1)

struct PacketHeader {
    std::uint32_t magic{kProtocolMagic};
    std::uint16_t version{kProtocolVersion};
    PacketType type{PacketType::Ping};
    std::uint8_t reserved{0};
    std::uint32_t sequence{0};
};

struct DiscoveryRequestPacket {
    PacketHeader header{};
};

struct DiscoveryOfferPacket {
    PacketHeader header{};
    char roomName[32]{};
    std::uint16_t gamePort{kDefaultLanPort};
    std::uint8_t playerCount{1};
    std::uint8_t maxPlayers{2};
};

struct JoinRequestPacket {
    PacketHeader header{};
    char playerName[24]{};
};

struct JoinAcceptPacket {
    PacketHeader header{};
    std::uint32_t assignedPlayerId{2};
};

struct InputPacket {
    PacketHeader header{};
    float moveAxis{0.0f};
    float aimX{1.0f};
    float aimY{0.0f};
    std::uint8_t jetpack{0};
    std::uint8_t firing{0};
    std::uint8_t reload{0};
    std::uint8_t reserved{0};
    std::uint32_t inputSequence{0};
};

struct EntityStatePacket {
    float x{0.0f};
    float y{0.0f};
    float velocityX{0.0f};
    float velocityY{0.0f};
    float aimX{1.0f};
    float aimY{0.0f};
    float health{100.0f};
    std::uint8_t alive{1};
};

struct SnapshotPacket {
    PacketHeader header{};
    EntityStatePacket host{};
    EntityStatePacket peer{};
    std::uint32_t acknowledgedInputSequence{0};
    std::uint32_t hostKOs{0};
    std::uint32_t peerKOs{0};
    std::uint32_t hostShotCounter{0};
    std::uint32_t peerShotCounter{0};
};

struct DisconnectPacket {
    PacketHeader header{};
};

struct PingPacket {
    PacketHeader header{};
};

#pragma pack(pop)

} // namespace net
