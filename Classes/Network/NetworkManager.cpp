#include "Network/NetworkManager.h"

#include <algorithm>
#include <cstring>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace net {
namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;

bool ensureSocketRuntime() {
    static bool initialized = false;
    static bool succeeded = false;
    if (!initialized) {
        WSADATA data{};
        succeeded = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        initialized = true;
    }
    return succeeded;
}

void closeNativeSocket(NativeSocket socket) {
    if (socket != kInvalidSocket) {
        closesocket(socket);
    }
}

bool setNonBlocking(NativeSocket socket) {
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
}

bool wouldBlock() {
    const int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK;
}
#else
using NativeSocket = int;
constexpr NativeSocket kInvalidSocket = -1;

bool ensureSocketRuntime() {
    return true;
}

void closeNativeSocket(NativeSocket socket) {
    if (socket != kInvalidSocket) {
        close(socket);
    }
}

bool setNonBlocking(NativeSocket socket) {
    const int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool wouldBlock() {
    return errno == EWOULDBLOCK || errno == EAGAIN;
}
#endif

NativeSocket toNative(std::intptr_t handle) {
    return static_cast<NativeSocket>(handle);
}

bool sameEndpoint(const std::uint8_t* lhs, int lhsLength,
                  const std::uint8_t* rhs, int rhsLength) {
    if (lhsLength != rhsLength || lhsLength <= 0) {
        return false;
    }

    const auto* a = reinterpret_cast<const sockaddr*>(lhs);
    const auto* b = reinterpret_cast<const sockaddr*>(rhs);
    if (a->sa_family != AF_INET || b->sa_family != AF_INET) {
        return false;
    }

    const auto* av4 = reinterpret_cast<const sockaddr_in*>(a);
    const auto* bv4 = reinterpret_cast<const sockaddr_in*>(b);
    return av4->sin_port == bv4->sin_port && av4->sin_addr.s_addr == bv4->sin_addr.s_addr;
}

template <typename T>
bool packetFits(std::size_t size) {
    return size >= sizeof(T);
}

EntityState fromPacket(const EntityStatePacket& packet) {
    EntityState state{};
    state.x = packet.x;
    state.y = packet.y;
    state.velocityX = packet.velocityX;
    state.velocityY = packet.velocityY;
    state.aimX = packet.aimX;
    state.aimY = packet.aimY;
    state.health = packet.health;
    state.alive = packet.alive != 0;
    return state;
}

EntityStatePacket toPacket(const EntityState& state) {
    EntityStatePacket packet{};
    packet.x = state.x;
    packet.y = state.y;
    packet.velocityX = state.velocityX;
    packet.velocityY = state.velocityY;
    packet.aimX = state.aimX;
    packet.aimY = state.aimY;
    packet.health = state.health;
    packet.alive = state.alive ? 1 : 0;
    return packet;
}

} // namespace

NetworkManager& NetworkManager::instance() {
    static NetworkManager manager;
    return manager;
}

NetworkManager::~NetworkManager() {
    closeSocket();
}

bool NetworkManager::openSocket(std::uint16_t bindPort) {
    closeSocket();

    if (!ensureSocketRuntime()) {
        statusText_ = "Socket runtime failed";
        state_ = ConnectionState::Error;
        return false;
    }

    const NativeSocket socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket == kInvalidSocket) {
        statusText_ = "Could not create UDP socket";
        state_ = ConnectionState::Error;
        return false;
    }

    int enabled = 1;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&enabled), sizeof(enabled));
    setsockopt(socket, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<const char*>(&enabled), sizeof(enabled));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(bindPort);

    if (::bind(socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        closeNativeSocket(socket);
        statusText_ = "UDP port is unavailable";
        state_ = ConnectionState::Error;
        return false;
    }

    if (!setNonBlocking(socket)) {
        closeNativeSocket(socket);
        statusText_ = "Could not enable non-blocking UDP";
        state_ = ConnectionState::Error;
        return false;
    }

    sockaddr_in actual{};
#ifdef _WIN32
    int actualLength = sizeof(actual);
#else
    socklen_t actualLength = sizeof(actual);
#endif
    if (getsockname(socket, reinterpret_cast<sockaddr*>(&actual), &actualLength) == 0) {
        boundPort_ = ntohs(actual.sin_port);
    } else {
        boundPort_ = bindPort;
    }

    socketHandle_ = static_cast<std::intptr_t>(socket);
    return true;
}

void NetworkManager::closeSocket() {
    if (socketHandle_ == -1) {
        return;
    }

    closeNativeSocket(toNative(socketHandle_));
    socketHandle_ = -1;
    boundPort_ = 0;
}

bool NetworkManager::hostLan(std::uint16_t port, const std::string& roomName) {
    disconnect();

    if (!openSocket(port)) {
        return false;
    }

    role_ = NetworkRole::Host;
    state_ = ConnectionState::Hosting;
    connected_ = false;
    discovering_ = false;
    discoveryPort_ = port;
    roomName_ = roomName.empty() ? "Doodle Room" : roomName.substr(0, 31);
    statusText_ = "Hosting LAN room on UDP " + std::to_string(port);
    secondsSincePacket_ = 0.0f;
    return true;
}

bool NetworkManager::beginDiscovery(std::uint16_t port) {
    disconnect();

    if (!openSocket(0)) {
        return false;
    }

    role_ = NetworkRole::Offline;
    state_ = ConnectionState::Discovering;
    connected_ = false;
    discovering_ = true;
    discoveryPort_ = port;
    rooms_.clear();
    discoveryTimer_ = kDiscoveryInterval;
    statusText_ = "Searching LAN for rooms...";
    return true;
}

bool NetworkManager::joinLan(const std::string& address, std::uint16_t port) {
    if (socketHandle_ == -1 && !openSocket(0)) {
        return false;
    }

    if (!addressFromIpv4(address, port, hostAddress_, hostAddressLength_)) {
        statusText_ = "Invalid host IPv4 address";
        state_ = ConnectionState::Error;
        return false;
    }

    role_ = NetworkRole::Peer;
    state_ = ConnectionState::Connecting;
    connected_ = false;
    discovering_ = false;
    inputTimer_ = 0.0f;
    secondsSincePacket_ = 0.0f;
    statusText_ = "Connecting to " + address + ":" + std::to_string(port);
    sendJoinRequest();
    return true;
}

bool NetworkManager::joinFirstDiscoveredRoom() {
    if (rooms_.empty()) {
        statusText_ = "No LAN rooms found";
        return false;
    }
    return joinLan(rooms_.front().address, rooms_.front().port);
}

void NetworkManager::disconnect() {
    if (connected_) {
        sendDisconnect();
    }

    closeSocket();
    role_ = NetworkRole::Offline;
    state_ = ConnectionState::Offline;
    connected_ = false;
    discovering_ = false;
    peerAddressLength_ = 0;
    hostAddressLength_ = 0;
    latestPeerInput_ = {};
    outgoingSnapshot_ = {};
    latestSnapshot_ = {};
    snapshotAvailable_ = false;
    packetSequence_ = 1;
    localInputSequence_ = 0;
    latestPeerInputSequence_ = 0;
    discoveryTimer_ = 0.0f;
    inputTimer_ = 0.0f;
    snapshotTimer_ = 0.0f;
    pingTimer_ = 0.0f;
    secondsSincePacket_ = 0.0f;
    statusText_ = "Offline";
}

void NetworkManager::update(float dt) {
    for (auto& room : rooms_) {
        room.age += dt;
    }
    rooms_.erase(
        std::remove_if(rooms_.begin(), rooms_.end(), [](const RoomInfo& room) {
            return room.age > kRoomTimeout;
        }),
        rooms_.end()
    );

    if (socketHandle_ == -1) {
        return;
    }

    if (connected_) {
        secondsSincePacket_ += dt;
    }

    pollSocket();

    if (discovering_) {
        discoveryTimer_ += dt;
        if (discoveryTimer_ >= kDiscoveryInterval) {
            discoveryTimer_ = 0.0f;
            sendDiscoveryRequest();
        }
    }

    if (role_ == NetworkRole::Peer && state_ == ConnectionState::Connecting) {
        inputTimer_ += dt;
        if (inputTimer_ >= 0.5f) {
            inputTimer_ = 0.0f;
            sendJoinRequest();
        }
    }

    if (role_ == NetworkRole::Peer && connected_) {
        inputTimer_ += dt;
        if (inputTimer_ >= kInputSendInterval) {
            inputTimer_ = 0.0f;
            sendInput();
        }
    }

    if (role_ == NetworkRole::Host && connected_) {
        snapshotTimer_ += dt;
        if (snapshotTimer_ >= kSnapshotSendInterval) {
            snapshotTimer_ = 0.0f;
            sendSnapshot();
        }
    }

    if (connected_) {
        pingTimer_ += dt;
        if (pingTimer_ >= 1.0f) {
            pingTimer_ = 0.0f;
            PingPacket ping{};
            ping.header.type = PacketType::Ping;
            ping.header.sequence = packetSequence_++;

            if (role_ == NetworkRole::Host) {
                sendRaw(&ping, sizeof(ping), peerAddress_.data(), peerAddressLength_);
            } else if (role_ == NetworkRole::Peer) {
                sendRaw(&ping, sizeof(ping), hostAddress_.data(), hostAddressLength_);
            }
        }

        if (secondsSincePacket_ > kConnectionTimeout) {
            closeSocket();
            role_ = NetworkRole::Offline;
            state_ = ConnectionState::Error;
            connected_ = false;
            discovering_ = false;
            statusText_ = "Connection timed out";
        }
    }
}

void NetworkManager::setLocalInput(const InputState& input) {
    localInput_ = input;
    localInput_.sequence = ++localInputSequence_;
}

void NetworkManager::publishSnapshot(const MatchSnapshot& snapshot) {
    outgoingSnapshot_ = snapshot;
    outgoingSnapshot_.acknowledgedInputSequence = latestPeerInputSequence_;
}

bool NetworkManager::consumeLatestSnapshot(MatchSnapshot& outSnapshot) {
    if (!snapshotAvailable_) {
        return false;
    }

    outSnapshot = latestSnapshot_;
    snapshotAvailable_ = false;
    return true;
}

void NetworkManager::pollSocket() {
    std::array<std::uint8_t, 2048> buffer{};

    while (true) {
        std::array<std::uint8_t, 128> sourceStorage{};
#ifdef _WIN32
        int sourceLength = static_cast<int>(sourceStorage.size());
        const int received = recvfrom(
            toNative(socketHandle_),
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()),
            0,
            reinterpret_cast<sockaddr*>(sourceStorage.data()),
            &sourceLength
        );
        if (received == SOCKET_ERROR) {
            if (wouldBlock()) {
                break;
            }
            statusText_ = "UDP receive error";
            break;
        }
#else
        socklen_t sourceLengthNative = static_cast<socklen_t>(sourceStorage.size());
        const ssize_t received = recvfrom(
            toNative(socketHandle_),
            buffer.data(),
            buffer.size(),
            0,
            reinterpret_cast<sockaddr*>(sourceStorage.data()),
            &sourceLengthNative
        );
        const int sourceLength = static_cast<int>(sourceLengthNative);
        if (received < 0) {
            if (wouldBlock()) {
                break;
            }
            statusText_ = "UDP receive error";
            break;
        }
#endif

        if (received <= 0) {
            break;
        }

        handlePacket(
            buffer.data(),
            static_cast<std::size_t>(received),
            sourceStorage.data(),
            sourceLength
        );
    }
}

void NetworkManager::sendDiscoveryRequest() {
    DiscoveryRequestPacket packet{};
    packet.header.type = PacketType::DiscoveryRequest;
    packet.header.sequence = packetSequence_++;

    std::array<std::uint8_t, 128> broadcast{};
    int length = 0;
    if (addressFromIpv4("255.255.255.255", discoveryPort_, broadcast, length)) {
        sendRaw(&packet, sizeof(packet), broadcast.data(), length);
    }
}

void NetworkManager::sendDiscoveryOffer(const std::uint8_t* address, int addressLength) {
    DiscoveryOfferPacket packet{};
    packet.header.type = PacketType::DiscoveryOffer;
    packet.header.sequence = packetSequence_++;
    std::strncpy(packet.roomName, roomName_.c_str(), sizeof(packet.roomName) - 1);
    packet.gamePort = discoveryPort_;
    packet.playerCount = connected_ ? 2 : 1;
    packet.maxPlayers = 2;
    sendRaw(&packet, sizeof(packet), address, addressLength);
}

void NetworkManager::sendJoinRequest() {
    if (hostAddressLength_ <= 0) {
        return;
    }

    JoinRequestPacket packet{};
    packet.header.type = PacketType::JoinRequest;
    packet.header.sequence = packetSequence_++;
    std::strncpy(packet.playerName, "Sky Explorer", sizeof(packet.playerName) - 1);
    sendRaw(&packet, sizeof(packet), hostAddress_.data(), hostAddressLength_);
}

void NetworkManager::sendJoinAccept(const std::uint8_t* address, int addressLength) {
    JoinAcceptPacket packet{};
    packet.header.type = PacketType::JoinAccept;
    packet.header.sequence = packetSequence_++;
    packet.assignedPlayerId = 2;
    sendRaw(&packet, sizeof(packet), address, addressLength);
}

void NetworkManager::sendInput() {
    if (!connected_ || hostAddressLength_ <= 0) {
        return;
    }

    InputPacket packet{};
    packet.header.type = PacketType::Input;
    packet.header.sequence = packetSequence_++;
    packet.moveAxis = localInput_.moveAxis;
    packet.aimX = localInput_.aimX;
    packet.aimY = localInput_.aimY;
    packet.jetpack = localInput_.jetpack ? 1 : 0;
    packet.firing = localInput_.firing ? 1 : 0;
    packet.reload = localInput_.reload ? 1 : 0;
    packet.inputSequence = localInput_.sequence;
    sendRaw(&packet, sizeof(packet), hostAddress_.data(), hostAddressLength_);
}

void NetworkManager::sendSnapshot() {
    if (!connected_ || peerAddressLength_ <= 0) {
        return;
    }

    SnapshotPacket packet{};
    packet.header.type = PacketType::Snapshot;
    packet.header.sequence = packetSequence_++;
    packet.host = toPacket(outgoingSnapshot_.host);
    packet.peer = toPacket(outgoingSnapshot_.peer);
    packet.acknowledgedInputSequence = outgoingSnapshot_.acknowledgedInputSequence;
    packet.hostKOs = outgoingSnapshot_.hostKOs;
    packet.peerKOs = outgoingSnapshot_.peerKOs;
    packet.hostShotCounter = outgoingSnapshot_.hostShotCounter;
    packet.peerShotCounter = outgoingSnapshot_.peerShotCounter;
    sendRaw(&packet, sizeof(packet), peerAddress_.data(), peerAddressLength_);
}

void NetworkManager::sendDisconnect() {
    DisconnectPacket packet{};
    packet.header.type = PacketType::Disconnect;
    packet.header.sequence = packetSequence_++;

    if (role_ == NetworkRole::Host && peerAddressLength_ > 0) {
        sendRaw(&packet, sizeof(packet), peerAddress_.data(), peerAddressLength_);
    } else if (role_ == NetworkRole::Peer && hostAddressLength_ > 0) {
        sendRaw(&packet, sizeof(packet), hostAddress_.data(), hostAddressLength_);
    }
}

bool NetworkManager::sendRaw(const void* data,
                             std::size_t size,
                             const std::uint8_t* address,
                             int addressLength) {
    if (socketHandle_ == -1 || !data || !address || addressLength <= 0) {
        return false;
    }

#ifdef _WIN32
    const int sent = sendto(
        toNative(socketHandle_),
        reinterpret_cast<const char*>(data),
        static_cast<int>(size),
        0,
        reinterpret_cast<const sockaddr*>(address),
        addressLength
    );
    return sent == static_cast<int>(size);
#else
    const ssize_t sent = sendto(
        toNative(socketHandle_),
        data,
        size,
        0,
        reinterpret_cast<const sockaddr*>(address),
        static_cast<socklen_t>(addressLength)
    );
    return sent == static_cast<ssize_t>(size);
#endif
}

void NetworkManager::handlePacket(const std::uint8_t* data,
                                  std::size_t size,
                                  const std::uint8_t* sourceAddress,
                                  int sourceAddressLength) {
    if (!packetFits<PacketHeader>(size)) {
        return;
    }

    const auto* header = reinterpret_cast<const PacketHeader*>(data);
    if (header->magic != kProtocolMagic || header->version != kProtocolVersion) {
        return;
    }

    switch (header->type) {
        case PacketType::DiscoveryRequest: {
            if (role_ == NetworkRole::Host && packetFits<DiscoveryRequestPacket>(size)) {
                sendDiscoveryOffer(sourceAddress, sourceAddressLength);
            }
            break;
        }

        case PacketType::DiscoveryOffer: {
            if (discovering_ && packetFits<DiscoveryOfferPacket>(size)) {
                rememberRoom(
                    *reinterpret_cast<const DiscoveryOfferPacket*>(data),
                    sourceAddress,
                    sourceAddressLength
                );
            }
            break;
        }

        case PacketType::JoinRequest: {
            if (role_ != NetworkRole::Host || !packetFits<JoinRequestPacket>(size)) {
                break;
            }

            if (!connected_ || sameEndpoint(
                    peerAddress_.data(), peerAddressLength_, sourceAddress, sourceAddressLength)) {
                std::memset(peerAddress_.data(), 0, peerAddress_.size());
                std::memcpy(peerAddress_.data(), sourceAddress,
                            std::min<int>(sourceAddressLength, static_cast<int>(peerAddress_.size())));
                peerAddressLength_ = std::min<int>(sourceAddressLength, static_cast<int>(peerAddress_.size()));
                connected_ = true;
                state_ = ConnectionState::Connected;
                secondsSincePacket_ = 0.0f;
                statusText_ = "Peer joined: " + ipv4FromAddress(sourceAddress, sourceAddressLength);
                sendJoinAccept(sourceAddress, sourceAddressLength);
            }
            break;
        }

        case PacketType::JoinAccept: {
            if (role_ == NetworkRole::Peer && packetFits<JoinAcceptPacket>(size) &&
                sameEndpoint(hostAddress_.data(), hostAddressLength_, sourceAddress, sourceAddressLength)) {
                connected_ = true;
                state_ = ConnectionState::Connected;
                secondsSincePacket_ = 0.0f;
                statusText_ = "Connected to host";
            }
            break;
        }

        case PacketType::Input: {
            if (role_ != NetworkRole::Host || !connected_ ||
                !packetFits<InputPacket>(size) ||
                !sameEndpoint(peerAddress_.data(), peerAddressLength_, sourceAddress, sourceAddressLength)) {
                break;
            }

            const auto& packet = *reinterpret_cast<const InputPacket*>(data);
            if (packet.inputSequence >= latestPeerInputSequence_) {
                latestPeerInput_.moveAxis = packet.moveAxis;
                latestPeerInput_.aimX = packet.aimX;
                latestPeerInput_.aimY = packet.aimY;
                latestPeerInput_.jetpack = packet.jetpack != 0;
                latestPeerInput_.firing = packet.firing != 0;
                latestPeerInput_.reload = packet.reload != 0;
                latestPeerInput_.sequence = packet.inputSequence;
                latestPeerInputSequence_ = packet.inputSequence;
            }
            secondsSincePacket_ = 0.0f;
            break;
        }

        case PacketType::Snapshot: {
            if (role_ != NetworkRole::Peer || !connected_ ||
                !packetFits<SnapshotPacket>(size) ||
                !sameEndpoint(hostAddress_.data(), hostAddressLength_, sourceAddress, sourceAddressLength)) {
                break;
            }

            const auto& packet = *reinterpret_cast<const SnapshotPacket*>(data);
            latestSnapshot_.host = fromPacket(packet.host);
            latestSnapshot_.peer = fromPacket(packet.peer);
            latestSnapshot_.acknowledgedInputSequence = packet.acknowledgedInputSequence;
            latestSnapshot_.hostKOs = packet.hostKOs;
            latestSnapshot_.peerKOs = packet.peerKOs;
            latestSnapshot_.hostShotCounter = packet.hostShotCounter;
            latestSnapshot_.peerShotCounter = packet.peerShotCounter;
            snapshotAvailable_ = true;
            secondsSincePacket_ = 0.0f;
            break;
        }

        case PacketType::Disconnect: {
            const bool fromPeer = role_ == NetworkRole::Host &&
                sameEndpoint(peerAddress_.data(), peerAddressLength_, sourceAddress, sourceAddressLength);
            const bool fromHost = role_ == NetworkRole::Peer &&
                sameEndpoint(hostAddress_.data(), hostAddressLength_, sourceAddress, sourceAddressLength);

            if (fromPeer || fromHost) {
                connected_ = false;
                if (role_ == NetworkRole::Host) {
                    state_ = ConnectionState::Hosting;
                    peerAddressLength_ = 0;
                    statusText_ = "Peer left - hosting room";
                } else {
                    state_ = ConnectionState::Error;
                    statusText_ = "Host ended the session";
                }
            }
            break;
        }

        case PacketType::Ping: {
            PingPacket pong{};
            pong.header.type = PacketType::Pong;
            pong.header.sequence = packetSequence_++;
            sendRaw(&pong, sizeof(pong), sourceAddress, sourceAddressLength);

            const bool counterpart =
                (role_ == NetworkRole::Host && connected_ &&
                 sameEndpoint(peerAddress_.data(), peerAddressLength_, sourceAddress, sourceAddressLength)) ||
                (role_ == NetworkRole::Peer && connected_ &&
                 sameEndpoint(hostAddress_.data(), hostAddressLength_, sourceAddress, sourceAddressLength));
            if (counterpart) {
                secondsSincePacket_ = 0.0f;
            }
            break;
        }

        case PacketType::Pong: {
            const bool counterpart =
                (role_ == NetworkRole::Host && connected_ &&
                 sameEndpoint(peerAddress_.data(), peerAddressLength_, sourceAddress, sourceAddressLength)) ||
                (role_ == NetworkRole::Peer && connected_ &&
                 sameEndpoint(hostAddress_.data(), hostAddressLength_, sourceAddress, sourceAddressLength));
            if (counterpart) {
                secondsSincePacket_ = 0.0f;
            }
            break;
        }
    }
}

void NetworkManager::rememberRoom(const DiscoveryOfferPacket& offer,
                                  const std::uint8_t* sourceAddress,
                                  int sourceAddressLength) {
    const std::string address = ipv4FromAddress(sourceAddress, sourceAddressLength);
    if (address.empty()) {
        return;
    }

    auto existing = std::find_if(rooms_.begin(), rooms_.end(), [&](const RoomInfo& room) {
        return room.address == address && room.port == offer.gamePort;
    });

    const std::string safeName(offer.roomName,
        std::find(offer.roomName, offer.roomName + sizeof(offer.roomName), '\0'));

    if (existing != rooms_.end()) {
        existing->name = safeName.empty() ? "Doodle Room" : safeName;
        existing->playerCount = offer.playerCount;
        existing->maxPlayers = offer.maxPlayers;
        existing->age = 0.0f;
    } else {
        RoomInfo room{};
        room.name = safeName.empty() ? "Doodle Room" : safeName;
        room.address = address;
        room.port = offer.gamePort;
        room.playerCount = offer.playerCount;
        room.maxPlayers = offer.maxPlayers;
        room.age = 0.0f;
        rooms_.push_back(room);
    }

    statusText_ = std::to_string(rooms_.size()) + " LAN room(s) found - press J to join";
}

bool NetworkManager::addressFromIpv4(const std::string& ipv4,
                                     std::uint16_t port,
                                     std::array<std::uint8_t, 128>& storage,
                                     int& length) const {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, ipv4.c_str(), &address.sin_addr) != 1) {
        return false;
    }

    storage.fill(0);
    std::memcpy(storage.data(), &address, sizeof(address));
    length = sizeof(address);
    return true;
}

std::string NetworkManager::ipv4FromAddress(const std::uint8_t* address, int addressLength) const {
    if (!address || addressLength < static_cast<int>(sizeof(sockaddr_in))) {
        return {};
    }

    const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(address);
    if (ipv4->sin_family != AF_INET) {
        return {};
    }

    char text[INET_ADDRSTRLEN]{};
    if (!inet_ntop(AF_INET, &ipv4->sin_addr, text, sizeof(text))) {
        return {};
    }
    return text;
}

} // namespace net
