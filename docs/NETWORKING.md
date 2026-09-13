# Networking Architecture

## Current milestone

Doodle Fight now has a working code path for a **2-player LAN host-authoritative P2P match**.

```text
                 HOST
        authoritative simulation
        /                    \
 local host input       peer input packets
        |                    |
 host character        peer character
        \                    /
         damage / KO / respawn
                  |
          snapshots @ 20 Hz
                  |
                 PEER
       prediction + reconciliation
```

There is no dedicated gameplay server for LAN play. The host player's machine temporarily becomes the match authority.

## Transport

Protocol v1 uses UDP over IPv4.

- Default gameplay/discovery port: `42042/UDP`
- LAN discovery: broadcast to `255.255.255.255:42042`
- Peer input rate: target `30 Hz`
- Host snapshot rate: target `20 Hz`
- Keepalive: ping/pong approximately once per second
- Connection timeout: 6 seconds without counterpart traffic
- Packet magic: `DFN1`
- Protocol version: `1`

Windows uses WinSock2. POSIX/Android-compatible builds use BSD sockets.

## Discovery and joining

1. A host binds UDP port `42042`.
2. A searching peer opens an ephemeral UDP port.
3. The peer broadcasts `DiscoveryRequest` packets.
4. The host replies directly with a `DiscoveryOffer` containing room name, game port and occupancy.
5. The peer sends `JoinRequest` to the host.
6. The host stores the first peer endpoint and replies with `JoinAccept`.
7. Match input/snapshots then travel directly between those two devices.

The current MVP intentionally supports one host and one peer. Stabilize this path before increasing room capacity.

## Host authority

The peer does **not** decide its own damage, KO result or respawn result.

The peer sends input intent:

- horizontal movement axis
- aim vector
- fire held/not held
- jetpack held/not held
- reload request

The host simulates:

- host movement
- peer movement from received input
- both weapons
- both projectile paths
- projectile/player collision
- damage
- health
- KOs
- respawn lifecycle

The host then publishes snapshots containing both character states and the current score.

## Client prediction and reconciliation

The peer simulates its own movement immediately so controls do not wait for a network round trip.

When a host snapshot arrives:

- the remote host character is pulled strongly toward authoritative state;
- the peer's predicted local character is blended toward its authoritative host position/velocity;
- health/alive state always comes from the host;
- host shot counters drive remote projectile visuals on the peer.

This is an MVP reconciliation pass. A later milestone should retain an input-history buffer and replay unacknowledged inputs using `acknowledgedInputSequence` for tighter high-latency correction.

## Current packet types

```text
DiscoveryRequest
DiscoveryOffer
JoinRequest
JoinAccept
Input
Snapshot
Disconnect
Ping
Pong
```

See `Classes/Network/Protocol.h` for the packet layout.

## Windows LAN test

Build the game on two Windows machines connected to the same router/Wi-Fi, or launch two copies where the OS/network configuration permits it.

On machine A:

```text
H = host Doodle Fight Room
```

On machine B:

```text
F = search for LAN rooms
J = join the first discovered room
```

During the match:

```text
A/D        move
Space      jetpack
Mouse      aim
Left Mouse fire
R          reload
L          leave network session
```

If Windows Defender Firewall prompts for network access, allow the game on the intended private network. LAN discovery can fail when client isolation, guest Wi-Fi isolation, VPN routing, or a firewall blocks UDP broadcast/port `42042`.

## Reliability notes

Movement and aim are intentionally transient UDP state. Later packets supersede lost older packets.

Join requests are retried while connecting. Ping/pong detects stale sessions. Snapshot/input sequence values are present for ordering and reconciliation.

Critical match events are currently represented through authoritative repeated snapshot state (health/alive/score) rather than relying on a single fragile event packet. A future protocol revision can add a small reliable-UDP event channel for pickups, match start/end, map selection and other one-shot commands.

## Scaling plan

After the 2-player path is tested under packet loss and realistic Wi-Fi latency:

1. add input-history replay and better interpolation;
2. support 4 total players;
3. add lobby ready state and map/mode selection;
4. add host migration only after normal disconnect behavior is stable;
5. measure bandwidth and host CPU before considering 8-12 players.

## Internet P2P

Internet P2P remains a later milestone. NAT/CGNAT may prevent direct connections. STUN plus a tiny signaling service can help peers establish a route while keeping gameplay traffic direct P2P. A strict direct-only mode can simply report when NAT traversal fails.
