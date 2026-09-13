# Networking Architecture

## Model

Doodle Fight uses host-authoritative P2P matches.

```text
              HOST
       authoritative state
          /     |     \
       Peer   Peer   Peer
```

There is no dedicated gameplay server for LAN matches. The host device validates gameplay state for the duration of the match.

## Transport plan

- UDP gameplay traffic
- LAN broadcast discovery
- Sequence numbers
- Snapshot interpolation
- Client-side prediction
- Host reconciliation
- Reliable handling for critical events such as joins, KOs, respawns and match state

## Host authority

The host owns decisions for:

- damage and hits
- health
- KOs
- respawns
- pickups
- scores
- match timer
- win condition

Peers send intent/input such as:

- movement
- aim
- fire
- jetpack
- weapon switch
- gadget/grenade

## Initial limits

Start with 2 players, stabilize synchronization, then increase to 4. Do not target 8–12 players until packet loss, latency, interpolation and host CPU usage are measured and stable.

## Internet P2P

Internet P2P is a later milestone. NAT/CGNAT may prevent direct connections, so STUN/signaling can be added without turning the signaling service into a gameplay server. Strict direct-only mode can report when a route cannot be established.
