import { WebSocketServer } from 'ws';

const port = Number(process.env.PORT || 8787);
const wss = new WebSocketServer({ port, maxPayload: 128 * 1024 });
const rooms = new Map();
let clientCounter = 1;

const randomCode = () => {
  const alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  let code = '';
  for (let i = 0; i < 5; i += 1) code += alphabet[Math.floor(Math.random() * alphabet.length)];
  return code;
};
const send = (socket, payload) => { if (socket.readyState === 1) socket.send(JSON.stringify(payload)); };
const cleanRoom = (socket) => {
  const meta = socket.meta;
  if (!meta?.roomCode) return;
  const room = rooms.get(meta.roomCode);
  if (!room) return;
  if (meta.role === 'host') {
    for (const [clientId, peer] of room.peers) { send(peer, { type:'error', message:'Host disconnected.' }); peer.meta = undefined; peer.close(); room.peers.delete(clientId); }
    rooms.delete(meta.roomCode);
  } else if (meta.role === 'peer' && meta.clientId) {
    room.peers.delete(meta.clientId);
    send(room.host, { type:'peer-left', roomCode:meta.roomCode, clientId:meta.clientId });
  }
};

wss.on('connection', (socket) => {
  socket.on('message', (raw) => {
    let message; try { message = JSON.parse(raw.toString()); } catch { return; }
    if (message.type === 'register-host') {
      let code = String(message.roomCode || '').trim().toUpperCase();
      if (!code) { do code = randomCode(); while (rooms.has(code)); }
      if (rooms.has(code)) return send(socket, { type:'error', message:'Room code is already in use.' });
      rooms.set(code, { host:socket, peers:new Map() }); socket.meta = { role:'host', roomCode:code }; send(socket, { type:'hosted', roomCode:code }); return;
    }
    if (message.type === 'join-room') {
      const code = String(message.roomCode || '').trim().toUpperCase(); const room = rooms.get(code);
      if (!room) return send(socket, { type:'error', message:'Room not found.' });
      if (room.peers.size >= 3) return send(socket, { type:'error', message:'Room is full.' });
      const clientId = `peer-${clientCounter++}`; room.peers.set(clientId, socket); socket.meta = { role:'peer', roomCode:code, clientId };
      send(socket, { type:'join-accepted', roomCode:code, clientId }); send(room.host, { type:'peer-joined', roomCode:code, clientId }); return;
    }
    if (message.type === 'signal') {
      const meta = socket.meta; if (!meta?.roomCode) return; const room = rooms.get(meta.roomCode); if (!room) return;
      if (meta.role === 'host') { const peer = room.peers.get(String(message.target || '')); if (peer) send(peer, { type:'signal', roomCode:meta.roomCode, from:'host', payload:message.payload }); }
      else if (meta.role === 'peer' && meta.clientId) send(room.host, { type:'signal', roomCode:meta.roomCode, from:meta.clientId, payload:message.payload });
    }
  });
  socket.on('close', () => cleanRoom(socket));
  socket.on('error', () => cleanRoom(socket));
});
console.log(`Doodle Fight signaling server listening on ws://0.0.0.0:${port}`);
