# Doodle Fight PWA

Doodle Fight's browser edition is built with **Phaser 3 + TypeScript + WebRTC + Vite PWA** and uses the project's original AI-generated hero, VFX, and fantasy-tech weapon sprite sheets.

## Current feature set

- installable landscape PWA for Android and Windows
- AI-generated idle/run/jetpack/shoot/KO sprite animations and VFX
- five original fantasy-tech weapons
- three colorful arena themes with one-way floating platforms
- 2–4 player host-authoritative Free For All over direct WebRTC DataChannels
- lobby with ready states, room code, map/mode/score selection
- client-side prediction with input-history replay
- delayed interpolation for remote players
- practice mode with three bots
- desktop keyboard/mouse and Android-friendly multitouch controls
- public STUN NAT discovery and optional WebSocket signaling

## Run locally

```bash
cd web
npm install
npm run signal
```

In a second terminal:

```bash
npm run dev
```

For production, serve `dist/` over HTTPS and use `VITE_SIGNALING_URL=wss://signal.example.com`.

## Controls

Desktop: `A/D`, `Space`, mouse aim/fire, `Q/E`, `R`, `M`.

Touch: left virtual stick to move, right virtual stick to aim/fire, plus `JET`, `SWAP`, and `RLD`.
