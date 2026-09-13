# Doodle Fight PWA Multiplayer Architecture

## Topology

The PWA supports up to four players with a host-star WebRTC topology:

```text
Peer 2 ─┐
Peer 3 ─┼── direct WebRTC DataChannels ── Host / Player 1
Peer 4 ─┘
```

The signaling service does not carry gameplay traffic. It only forwards room presence, SDP descriptions, and ICE candidates.

## Channels

Each host↔peer link has two channels:

- `reliable` — ordered/reliable lobby, ready state, match start/end, and control events
- `game` — unordered, zero-retransmit input/snapshot traffic for lower latency

## Host authority

The host owns:

- accepted movement state
- weapon cooldown/ammo state
- projectiles
- hit detection and damage
- KOs and scores
- respawns
- match end

Peers send input samples, never trusted final positions.

## Prediction and replay

A peer immediately applies its own input locally and stores the sample in a bounded history. Host snapshots carry the last input sequence processed for that peer. When a snapshot arrives, the peer:

1. restores the host-authoritative state,
2. removes acknowledged inputs,
3. replays every unacknowledged input through the same movement simulation.

This is stronger than simple position blending because the client keeps responsive movement while converging to host truth.

## Remote interpolation

Non-local players are rendered from a short delayed snapshot buffer (about 110 ms). The renderer interpolates between the two surrounding samples instead of snapping to the newest network position.

## Signaling and STUN

`web/signaling/server.mjs` is a tiny WebSocket service for automatic room codes. Public STUN servers are configured in the browser WebRTC layer for NAT discovery. No gameplay relay or dedicated match server is required.

A TURN relay is intentionally not required for the current direct-P2P milestone. Networks that block all direct WebRTC paths can still fail to connect; TURN can be added later as an optional fallback.

## PWA vs native LAN

The web platform does not provide unrestricted raw UDP socket APIs, so the PWA uses WebRTC for LAN and Internet play. Raw UDP broadcast discovery remains in the native Cocos2d-x implementation.
