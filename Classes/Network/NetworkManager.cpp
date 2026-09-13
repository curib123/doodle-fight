#include "NetworkManager.h"

namespace net {

NetworkManager& NetworkManager::instance() {
    static NetworkManager manager;
    return manager;
}

bool NetworkManager::hostLan(std::uint16_t) {
    // Networking transport is intentionally stubbed for milestone 0.1.
    // Planned implementation: UDP host authority + LAN broadcast discovery.
    role_ = NetworkRole::Host;
    return true;
}

bool NetworkManager::joinLan(const char*, std::uint16_t) {
    role_ = NetworkRole::Peer;
    return true;
}

void NetworkManager::disconnect() {
    role_ = NetworkRole::Offline;
}

void NetworkManager::update(float) {
    // Packet receive/send loop will be implemented after the offline movement prototype.
}

} // namespace net
