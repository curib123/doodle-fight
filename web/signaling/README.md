# Doodle Fight Signaling Service

This is a tiny WebSocket signaling service for Doodle Fight's WebRTC rooms.

It forwards only room registration/join events, SDP offers/answers, and ICE candidates. It does **not** relay gameplay. Once WebRTC is connected, inputs and snapshots move directly between browsers.

## Run

```bash
cd web
npm install
npm run signal
```

`PORT` defaults to `8787`. For Internet play, deploy behind TLS/WebSocket support and build with `VITE_SIGNALING_URL=wss://...`.
