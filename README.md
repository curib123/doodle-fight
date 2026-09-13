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
- A shootable Training Buddy for combat testing
- Brighter sky/island prototype presentation and combat HUD
- Initial networking scaffold for the upcoming LAN P2P milestone
- Windows desktop entry point and PowerShell setup/build helpers

No external character or weapon art is required for the current prototype.

## Controls (Windows prototype)

- `A` / `Left Arrow` — move left
- `D` / `Right Arrow` — move right
- `Space` — jetpack
- `Mouse` — aim in 360 degrees
- `Left Mouse` — fire / hold to fire
- `R` — reload

## Run on Windows

Prerequisites: Git, CMake, and Visual Studio 2022 with the Desktop development with C++ workload.

```powershell
git clone https://github.com/curib123/doodle-fight.git
cd doodle-fight
.\scripts\setup_cocos.ps1
.\scripts\build_windows.ps1
.\build\Release\doodle_fight.exe
```

`setup_cocos.ps1` installs Cocos2d-x v4 into the ignored local `cocos2d/` directory, so the engine source is not copied into this repository.

## MVP roadmap

- [x] Movement + jetpack
- [x] 360-degree aiming
- [x] Shooting/projectiles
- [x] Reusable weapon foundation
- [x] Health, KO and respawn
- [ ] Four distinct original fantasy-tech weapons
- [ ] One production-quality colorful arena
- [ ] LAN room discovery
- [ ] Host-authoritative UDP gameplay
- [ ] 2–4 player Free For All
- [ ] Prediction/interpolation/reconciliation
- [ ] Android touch controls
- [ ] Android + Windows distributable builds

See `docs/GAME_DESIGN.md` and `docs/NETWORKING.md` for the locked direction.
