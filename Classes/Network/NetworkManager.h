#pragma once

#include "Network/Protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace net {

enum class NetworkRole : std::uint8_t {
    Offline,
    Host,
    Peer
};

enum class ConnectionState : std::uint8_t {
    Offline,
    Discovering,
    Hosting,
    Connecting,
    Connected,
    Error
};

struct RoomInfo {
    std::string name;
    std::string address;
    std::uint16_t port{kDefaultLanPort};
    std::uint8_t playerCount{1};
    std::uint8_t maxPlayers{2};
    float age{0.0f};
};

struct InputState {
    float moveAxis{0.0f};
    float aimX{1.0f};
    float aimY{0.0f};
    bool jetpack{false};
    bool firing{false};
    bool reload{false};
    std::uint32_t sequence{0};
};

struct EntityState {
    float x{0.0f};
    float y{0.0f};
    float velocityX{0.0f};
    float velocityY{0.0f};
    float aimX{1.0f};
    float aimY{0.0f};
    float health{100.0f};
    bool alive{true};
};

struct MatchSnapshot {
    EntityState host{};
    EntityState peer{};
    std::uint32_t acknowledgedInputSequence{0};
    std::uint32_t hostKOs{0};
    std::uint32_t peerKOs{0};
    std::uint32_t hostShotCounter{0};
    std::uint32_t peerShotCounter{0};
};

class NetworkManager final {
public:
    static NetworkManager& instance();

    bool hostLan(std::uint16_t port = kDefaultLanPort, const std::string& roomName = "Doodle Room");
    bool beginDiscovery(std::uint16_t port = kDefaultLanPort);
    bool joinLan(const std::string& address, std::uint16_t port = kDefaultLanPort);
    bool joinFirstDiscoveredRoom();
    void disconnect();
    void update(float dt);

    void setLocalInput(const InputState& input);
    const InputState& latestPeerInput() const { return latestPeerInput_; }

    void publishSnapshot(const MatchSnapshot& snapshot);
    bool consumeLatestSnapshot(MatchSnapshot& outSnapshot);

    NetworkRole role() const { return role_; }
    ConnectionState state() const { return state_; }
    bool connected() const { return connected_; }
    bool peerConnected() const { return role_ == NetworkRole::Host && connected_; }
    bool discovering() const { return discovering_; }

    const std::vector<RoomInfo>& rooms() const { return rooms_; }
    const std::string& statusText() const { return statusText_; }
    std::uint16_t boundPort() const { return boundPort_; }
    float secondsSincePacket() const { return secondsSincePacket_; }

private:
    NetworkManager() = default;
    ~NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    bool openSocket(std::uint16_t bindPort);
    void closeSocket();
    void pollSocket();

    void sendDiscoveryRequest();
    void sendDiscoveryOffer(const std::uint8_t* address, int addressLength);
    void sendJoinRequest();
    void sendJoinAccept(const std::uint8_t* address, int addressLength);
    void sendInput();
    void sendSnapshot();
    void sendDisconnect();

    bool sendRaw(const void* data,
                 std::size_t size,
                 const std::uint8_t* address,
                 int addressLength);

    void handlePacket(const std::uint8_t* data,
                      std::size_t size,
                      const std::uint8_t* sourceAddress,
                      int sourceAddressLength);

    void rememberRoom(const DiscoveryOfferPacket& offer,
                      const std::uint8_t* sourceAddress,
                      int sourceAddressLength);

    bool addressFromIpv4(const std::string& ipv4,
                         std::uint16_t port,
                         std::array<std::uint8_t, 128>& storage,
                         int& length) const;
    std::string ipv4FromAddress(const std::uint8_t* address, int addressLength) const;

    std::intptr_t socketHandle_{-1};
    std::uint16_t boundPort_{0};
    std::uint16_t discoveryPort_{kDefaultLanPort};

    NetworkRole role_{NetworkRole::Offline};
    ConnectionState state_{ConnectionState::Offline};
    bool connected_{false};
    bool discovering_{false};

    std::string roomName_{"Doodle Room"};
    std::string statusText_{"Offline"};
    std::vector<RoomInfo> rooms_;

    std::array<std::uint8_t, 128> peerAddress_{};
    int peerAddressLength_{0};
    std::array<std::uint8_t, 128> hostAddress_{};
    int hostAddressLength_{0};

    InputState localInput_{};
    InputState latestPeerInput_{};
    MatchSnapshot outgoingSnapshot_{};
    MatchSnapshot latestSnapshot_{};
    bool snapshotAvailable_{false};

    std::uint32_t packetSequence_{1};
    std::uint32_t localInputSequence_{0};
    std::uint32_t latestPeerInputSequence_{0};

    float discoveryTimer_{0.0f};
    float inputTimer_{0.0f};
    float snapshotTimer_{0.0f};
    float pingTimer_{0.0f};
    float secondsSincePacket_{0.0f};
};

} // namespace net
