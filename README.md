# Doodle Fight

Doodle Fight is an original, cute and colorful 2D jetpack arena shooter built from scratch. The project now has two editions that share the same gameplay direction but use the transport best suited to each platform.

## Editions

### `web/` — Phaser + TypeScript + WebRTC + Vite PWA

The PWA is the primary cross-platform browser edition and uses the original AI-generated Doodle Fight sprite/VFX/weapon sheets.

Implemented on the enhanced PWA branch:

- five distinct original fantasy-tech weapons
- three colorful floating-island arena themes with one-way platforms
- 2–4 player host-authoritative Free For All
- WebRTC host-star topology with direct gameplay DataChannels
- WebSocket room signaling + public STUN NAT discovery
- lobby ready states, room codes, map/mode/score selection
- client prediction with input-history replay after authoritative snapshots
- delayed interpolation for remote players
- practice mode with three bots
- desktop keyboard/mouse controls
- Android-friendly multitouch dual-stick controls
- installable Android and Windows PWA build via Vite PWA
- CI build artifact for `web/dist`

Run it:

```bash
cd web
npm install
npm run signal
```

Then in another terminal:

```bash
npm run dev
```

See `web/README.md` and `docs/PWA_MULTIPLAYER.md`.

### Native — Cocos2d-x + C++

The native prototype remains the raw-socket edition for Windows/Android work. It currently includes Windows desktop startup, movement/jetpack/combat, and 2-player IPv4 UDP LAN discovery/host-authoritative networking.

Native LAN controls on Windows:

- `H` — host a LAN room on UDP `42042`
- `F` — search for LAN rooms
- `J` — join the first discovered room
- `L` — leave the network session

Build on Windows:

```powershell
git clone https://github.com/curib123/doodle-fight.git
cd doodle-fight
.\scripts\setup_cocos.ps1
.\scripts\build_windows.ps1
.\build\Release\doodle_fight.exe
```

## Design direction

Doodle Fight does not copy Mini Militia source code, characters, maps, UI, sprites, names, or assets. Its identity is a cheerful fantasy-tech sky world with chibi explorers, glowing jetpacks, oversized toy-like energy weapons, colorful VFX, and short competitive arena matches.

## Platform networking note

Browsers and PWAs do not expose unrestricted raw UDP sockets. Therefore:

- **PWA LAN/Internet multiplayer** → WebRTC DataChannels + STUN + signaling
- **Native LAN multiplayer** → raw UDP discovery/game transport

This keeps the PWA installable on Android/Windows while preserving a native path for socket-level LAN features.
