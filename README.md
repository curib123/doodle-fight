# Doodle Fight

Doodle Fight is an original, cute and colorful 2D jetpack arena shooter built from scratch with Cocos2d-x and C++. It is inspired by the *genre* of fast side-view arena shooters, but it does not copy Mini Militia's source code, characters, maps, UI, sprites, names, or other assets.

## Direction

- Android first, Windows for development/testing
- Cocos2d-x v4-style APIs + C++17
- Host-authoritative P2P multiplayer
- LAN discovery first; Internet P2P/NAT traversal later
- Cute fantasy-tech characters, rounded shapes, bright VFX and colorful floating-island arenas
- No required accounts, backend, or dedicated match server for LAN play

## Current playable foundation

The prototype now includes:

- Original vector-drawn sky-explorer mascot
- Left/right movement, gravity and jetpack thrust
- 360-degree mouse aiming with an on-screen reticle
- Reusable `Weapon` definition/state system
- Pulse Pistol with fire rate, magazine, reload timing, damage and projectile speed
- Reusable projectile entity with lifetime and hit radius
- Damage, health, KO counting and automatic respawn lifecycle
- A shootable Training Buddy for offline combat testing
- Colorful sky/floating-island prototype arena and HUD
- **2-player LAN P2P host/join flow**
- **UDP LAN room discovery**
- **Host-authoritative peer movement, shooting, damage, KOs and respawns**
- **Peer-side movement prediction and snapshot reconciliation**
- Ping/pong keepalive, disconnect handling and connection timeout
- Windows desktop entry point and PowerShell setup/build helpers

No external character or weapon art is required for the current prototype.

## Controls (Windows prototype)

Gameplay:

- `A` / `Left Arrow` — move left
- `D` / `Right Arrow` — move right
- `Space` — jetpack
- `Mouse` — aim in 360 degrees
- `Left Mouse` — fire / hold to fire
- `R` — reload

LAN:

- `H` — host a LAN room on UDP `42042`
- `F` — search for LAN rooms
- `J` — join the first discovered room
- `L` — leave the current network session

## Run on Windows

Prerequisites: Git, CMake, and Visual Studio 2022 with the **Desktop development with C++** workload.

```powershell
git clone https://github.com/curib123/doodle-fight.git
cd doodle-fight
.\scripts\setup_cocos.ps1
.\scripts\build_windows.ps1
.\build\Release\doodle_fight.exe
```

`setup_cocos.ps1` installs Cocos2d-x v4 into the ignored local `cocos2d/` directory, so the engine source is not copied into this repository.

## Test LAN multiplayer

Use two Windows machines on the same local network.

Machine A:

```text
Launch game
Press H
```

Machine B:

```text
Launch game
Press F
Wait for the room to appear
Press J
```

The host simulates both players and owns damage, KO and respawn state. The peer sends movement/aim/fire/jetpack input and predicts its own movement locally before reconciling to host snapshots.

If Windows Defender Firewall asks for access, allow the game on the intended **Private network**. Guest Wi-Fi/client isolation, VPN routing, or blocked UDP broadcast can prevent room discovery.

## Current networking limits

- Current room size: **2 players** (`1 host + 1 peer`)
- LAN IPv4 only
- Direct P2P only
- No account/login/backend required
- No Internet NAT traversal yet
- No host migration yet
- Reconciliation is an MVP blend; full input-history replay is a later optimization

See `docs/NETWORKING.md` for protocol and authority details.

## MVP roadmap

- [x] Movement + jetpack
- [x] 360-degree aiming
- [x] Shooting/projectiles
- [x] Reusable weapon foundation
- [x] Health, KO and respawn
- [x] LAN room discovery
- [x] Host-authoritative UDP gameplay foundation
- [x] 2-player LAN Free For All foundation
- [x] Basic prediction/reconciliation
- [ ] Four distinct original fantasy-tech weapons
- [ ] One production-quality colorful arena
- [ ] 4-player Free For All
- [ ] Input-history replay + stronger interpolation
- [ ] Lobby ready/map/mode selection
- [ ] Android touch controls
- [ ] Android LAN socket integration/testing
- [ ] Android + Windows distributable builds
- [ ] Internet P2P/STUN signaling

See `docs/GAME_DESIGN.md` and `docs/NETWORKING.md` for the locked direction.
