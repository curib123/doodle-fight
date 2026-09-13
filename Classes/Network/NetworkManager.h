#pragma once

#include <cstdint>

namespace net {

enum class NetworkRole : std::uint8_t {
    Offline,
    Host,
    Peer
};

class NetworkManager final {
public:
    static NetworkManager& instance();

    bool hostLan(std::uint16_t port);
    bool joinLan(const char* address, std::uint16_t port);
    void disconnect();
    void update(float dt);

    NetworkRole role() const { return role_; }

private:
    NetworkManager() = default;
    NetworkRole role_{NetworkRole::Offline};
};

} // namespace net
