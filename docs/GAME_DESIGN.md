# Doodle Fight — Game Design Direction

## Identity

Doodle Fight is an original 2D aerial arena shooter with a cheerful fantasy-tech identity. The visual goal is cute, colorful and readable on small screens: rounded silhouettes, expressive faces, oversized gadgets, soft gradients, bright particles and whimsical floating-island environments.

This project must not copy Mini Militia characters, sprites, maps, menus, weapon art, names, audio, source code or level layouts.

## Core loop

Move → jetpack → aim → fire → dodge → collect pickups → KO opponents → respawn → score.

## MVP

- 2–4 players
- Free For All
- One original arena
- One original hero base
- Four fantasy-tech weapons
- Grenade/gadget
- Health, KO, respawn and scoreboard
- LAN P2P host/join

## Art direction

Working style: **Cozy Action Cartoon**.

- Big head / small body proportions
- Friendly silhouettes
- Fantasy-tech backpacks rather than military equipment
- Colorful energy projectiles instead of realistic weapon presentation
- Defeats use pops, sparks, portals and knockback rather than gore
- Maps use floating villages, crystal caves, gardens, ruins and whimsical workshops

## Controls

### Android
- Left virtual stick: movement
- Right virtual stick: 360° aim
- Jetpack button
- Fire / aim-release option
- Gadget/grenade
- Weapon switch

### Windows
- A/D: movement
- Space: jetpack
- Mouse: aim
- Left click: fire
- Q/E: weapon switch
- F: melee/gadget

## Development order

1. Offline movement + jetpack
2. Aim + projectiles
3. Damage + KO + respawn
4. Weapon framework
5. One complete map
6. LAN discovery
7. Two-player synchronization
8. Prediction/interpolation/reconciliation
9. Four-player match
10. Touch controls and Android packaging
