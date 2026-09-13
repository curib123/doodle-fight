# Doodle Fight

Doodle Fight is an original, cute and colorful 2D jetpack arena shooter built from scratch with Cocos2d-x and C++. It is inspired by the *genre* of fast side-view arena shooters, but it does not copy Mini Militia's source code, characters, maps, UI, sprites, names, or other assets.

## Direction

- Android first, Windows for development/testing
- Cocos2d-x + C++17
- Host-authoritative P2P multiplayer
- LAN discovery first; Internet P2P/NAT traversal later
- Cute fantasy-tech characters, rounded shapes, bright VFX and colorful floating-island arenas
- No required accounts, backend, or dedicated match server for LAN play

## Current milestone

The repository now contains the first playable foundation: an original vector-drawn placeholder hero, left/right movement, gravity, jetpack thrust and a simple ground arena. No external art assets are needed for this prototype.

## Controls (prototype)

- `A` / `Left Arrow` — move left
- `D` / `Right Arrow` — move right
- `Space` — jetpack

## Planned MVP

1. Movement + jetpack
2. 360-degree aiming
3. Shooting and four original fantasy-tech weapons
4. Health, KO and respawn
5. One colorful arena
6. LAN host/join discovery
7. Host-authoritative UDP networking
8. 2–4 player Free For All
9. Prediction/interpolation/reconciliation
10. Android + Windows builds

See `docs/GAME_DESIGN.md` and `docs/NETWORKING.md` for the locked direction.
