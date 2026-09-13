# Doodle Fight PWA Multiplayer

This document is reserved for the enhanced 4-player WebRTC architecture being integrated on the feature branch. The browser edition uses WebRTC DataChannels rather than raw UDP sockets: peers connect directly to the host for gameplay, while the signaling service only exchanges room presence, SDP, and ICE candidates.
