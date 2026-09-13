# Doodle Fight PWA

Browser/PWA version of Doodle Fight built with **Phaser 3 + TypeScript + WebRTC + Vite PWA**.

## What is implemented

- installable landscape PWA with offline caching
- original AI-generated hero sprite sheet used as real Phaser animations
- idle, run, jetpack, shooting and KO animation states
- original AI-generated VFX sheet for muzzle flashes, impacts, jet exhaust and respawn effects
- original fantasy-tech weapon art sheet
- five playable weapon profiles
- desktop keyboard/mouse controls
- multitouch mobile controls
- practice arena with combat drones
- direct WebRTC DataChannel 1v1
- host-authoritative damage, health, KOs, respawns and projectiles
- peer-side movement prediction with snapshot reconciliation
- no gameplay server required
- manual WebRTC offer/answer signaling for the current prototype

## Desktop controls

- `A` / `D` — move
- `Space` — jetpack
- mouse — aim
- hold left mouse — fire
- `Q` / `E` — switch weapon
- `R` — reload
- `M` — return to menu

## Touch controls

- drag the left thumb area — move
- hold `JET` — jetpack
- hold/drag on the right side — aim and fire

## Run

```bash
cd web
npm install
npm run dev
```

Open the shown local URL. For testing on a phone, run on your LAN and open the development server from the phone using the computer's LAN IP.

## WebRTC 1v1

### Host
1. Select **HOST WEBRTC**.
2. Copy the generated HOST OFFER code and send it to the other player.
3. Paste the JOIN ANSWER returned by the other player.

### Join
1. Select **JOIN WEBRTC**.
2. Paste the host's offer.
3. Copy the generated JOIN ANSWER back to the host.

Once both descriptions are exchanged, the browser establishes a direct WebRTC DataChannel. Public STUN is used for NAT discovery; there is no dedicated gameplay server or TURN fallback in this prototype.

## Build/install

```bash
npm run build
npm run preview
```

Serve the production build over HTTPS (or localhost) for full PWA installation and service-worker behavior.
