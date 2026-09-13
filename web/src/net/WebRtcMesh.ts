import type {
  GameMessage,
  HelloMessage,
  PlayerId,
  ReliableMessage,
  SignalingEnvelope
} from './Protocol';
import { SignalingClient } from './SignalingClient';

export type NetworkRole = 'host' | 'peer';
export type NetworkStatus = 'offline' | 'signaling' | 'connecting' | 'connected' | 'failed' | 'closed';

interface PeerLink {
  clientId: string;
  playerId: PlayerId;
  pc: RTCPeerConnection;
  reliable?: RTCDataChannel;
  game?: RTCDataChannel;
  pendingCandidates: RTCIceCandidateInit[];
}

type ReliableHandler = (from: PlayerId, message: ReliableMessage) => void;
type GameHandler = (from: PlayerId, message: GameMessage) => void;
type StatusHandler = (status: NetworkStatus, detail?: string) => void;
type PeerHandler = (playerId: PlayerId, connected: boolean) => void;

type SignalPayload = { description: RTCSessionDescriptionInit } | { candidate: RTCIceCandidateInit };

const RTC_CONFIG: RTCConfiguration = {
  iceServers: [
    { urls: ['stun:stun.l.google.com:19302', 'stun:stun1.l.google.com:19302'] },
    { urls: 'stun:stun.cloudflare.com:3478' }
  ],
  iceCandidatePoolSize: 4
};

export class WebRtcMesh {
  readonly role: NetworkRole;
  readonly displayName: string;
  roomCode = '';
  localPlayerId: PlayerId;
  status: NetworkStatus = 'offline';

  private readonly signaling: SignalingClient;
  private readonly links = new Map<string, PeerLink>();
  private peerLink?: PeerLink;
  private reliableHandlers = new Set<ReliableHandler>();
  private gameHandlers = new Set<GameHandler>();
  private statusHandlers = new Set<StatusHandler>();
  private peerHandlers = new Set<PeerHandler>();
  private offSignal?: () => void;
  private started = false;

  private constructor(role: NetworkRole, displayName: string, signalingUrl?: string) {
    this.role = role;
    this.displayName = displayName.trim().slice(0, 20) || 'Sky Explorer';
    this.localPlayerId = role === 'host' ? 1 : 2;
    this.signaling = new SignalingClient(signalingUrl);
  }

  static createHost(displayName: string, signalingUrl?: string): WebRtcMesh { return new WebRtcMesh('host', displayName, signalingUrl); }
  static createPeer(displayName: string, signalingUrl?: string): WebRtcMesh { return new WebRtcMesh('peer', displayName, signalingUrl); }

  async host(roomCode?: string): Promise<string> {
    if (this.role !== 'host') throw new Error('Only a host session can host a room.');
    await this.startSignaling();
    this.setStatus('signaling', 'Registering room...');
    return new Promise<string>((resolve, reject) => {
      const timeout = window.setTimeout(() => reject(new Error('Room registration timed out.')), 7000);
      const off = this.signaling.onMessage((message) => {
        if (message.type === 'hosted' && message.roomCode) {
          window.clearTimeout(timeout); off(); this.roomCode = message.roomCode;
          this.setStatus('connecting', `Room ${message.roomCode} ready`); resolve(message.roomCode);
        }
        if (message.type === 'error') { window.clearTimeout(timeout); off(); reject(new Error(message.message ?? 'Signaling error.')); }
      });
      this.signaling.send({ type:'register-host', roomCode:roomCode?.toUpperCase() });
    });
  }

  async join(roomCode: string): Promise<void> {
    if (this.role !== 'peer') throw new Error('Only a peer session can join a room.');
    this.roomCode = roomCode.trim().toUpperCase();
    if (!this.roomCode) throw new Error('Enter a room code.');
    await this.startSignaling(); this.setStatus('signaling', `Joining ${this.roomCode}...`);
    this.signaling.send({ type:'join-room', roomCode:this.roomCode });
  }

  onReliable(handler: ReliableHandler): () => void { this.reliableHandlers.add(handler); return () => this.reliableHandlers.delete(handler); }
  onGame(handler: GameHandler): () => void { this.gameHandlers.add(handler); return () => this.gameHandlers.delete(handler); }
  onStatus(handler: StatusHandler): () => void { this.statusHandlers.add(handler); handler(this.status); return () => this.statusHandlers.delete(handler); }
  onPeer(handler: PeerHandler): () => void { this.peerHandlers.add(handler); return () => this.peerHandlers.delete(handler); }

  connectedPlayerIds(): PlayerId[] {
    if (this.role === 'peer') return this.peerLink?.reliable?.readyState === 'open' ? [1] : [];
    return [...this.links.values()].filter((link) => link.reliable?.readyState === 'open').map((link) => link.playerId);
  }

  sendReliable(message: ReliableMessage, target?: PlayerId): void {
    if (this.role === 'peer') { this.sendChannel(this.peerLink?.reliable, message); return; }
    for (const link of this.links.values()) { if (target !== undefined && link.playerId !== target) continue; this.sendChannel(link.reliable, message); }
  }
  sendGame(message: GameMessage, target?: PlayerId): void {
    if (this.role === 'peer') { this.sendChannel(this.peerLink?.game, message); return; }
    for (const link of this.links.values()) { if (target !== undefined && link.playerId !== target) continue; this.sendChannel(link.game, message); }
  }

  close(): void {
    this.offSignal?.(); this.offSignal = undefined;
    for (const link of this.links.values()) this.closeLink(link); this.links.clear();
    if (this.peerLink) this.closeLink(this.peerLink); this.peerLink = undefined;
    this.signaling.close(); this.setStatus('closed');
  }

  private async startSignaling(): Promise<void> {
    if (this.started) return; this.started = true;
    this.offSignal = this.signaling.onMessage((message) => void this.handleSignalEnvelope(message));
    await this.signaling.connect();
  }

  private async handleSignalEnvelope(message: SignalingEnvelope): Promise<void> {
    if (message.type === 'error') { this.setStatus('failed', message.message ?? 'Signaling error'); return; }
    if (this.role === 'host') {
      if (message.type === 'peer-joined' && message.clientId) {
        if (this.links.has(message.clientId) || this.links.size >= 3) return;
        const link = this.createHostLink(message.clientId); this.links.set(message.clientId, link);
        const offer = await link.pc.createOffer(); await link.pc.setLocalDescription(offer);
        this.signalTo(message.clientId, { description:offer }); return;
      }
      if (message.type === 'peer-left' && message.clientId) {
        const link = this.links.get(message.clientId); if (link) { this.emitPeer(link.playerId,false); this.closeLink(link); this.links.delete(message.clientId); } return;
      }
      if (message.type === 'signal' && message.from && message.payload) {
        const link = this.links.get(message.from); if (link) await this.applySignal(link, message.payload as SignalPayload);
      }
      return;
    }
    if (message.type === 'join-accepted') { this.setStatus('connecting','WebRTC negotiation started'); return; }
    if (message.type === 'signal' && message.payload) {
      if (!this.peerLink) this.peerLink = this.createPeerLink(message.from ?? 'host');
      const payload = message.payload as SignalPayload;
      if ('description' in payload && payload.description.type === 'offer') {
        await this.applySignal(this.peerLink,payload); const answer = await this.peerLink.pc.createAnswer();
        await this.peerLink.pc.setLocalDescription(answer); this.signalTo('host',{ description:answer });
      } else await this.applySignal(this.peerLink,payload);
    }
  }

  private createHostLink(clientId: string): PeerLink {
    const used = new Set([...this.links.values()].map((link) => link.playerId));
    const playerId = ([2,3,4] as PlayerId[]).find((id) => !used.has(id)); if (!playerId) throw new Error('Room is full.');
    const pc = new RTCPeerConnection(RTC_CONFIG); const link: PeerLink = { clientId, playerId, pc, pendingCandidates:[] };
    this.attachReliable(link, pc.createDataChannel('reliable',{ ordered:true }));
    this.attachGame(link, pc.createDataChannel('game',{ ordered:false,maxRetransmits:0 })); this.attachPeerConnection(link); return link;
  }
  private createPeerLink(clientId: string): PeerLink {
    const pc = new RTCPeerConnection(RTC_CONFIG); const link: PeerLink = { clientId, playerId:1, pc, pendingCandidates:[] };
    pc.addEventListener('datachannel',(event)=>{ if(event.channel.label==='reliable')this.attachReliable(link,event.channel); if(event.channel.label==='game')this.attachGame(link,event.channel); });
    this.attachPeerConnection(link); return link;
  }
  private attachPeerConnection(link: PeerLink): void {
    link.pc.addEventListener('icecandidate',(event)=>{ if(event.candidate)this.signalTo(this.role==='host'?link.clientId:'host',{candidate:event.candidate.toJSON()}); });
    link.pc.addEventListener('connectionstatechange',()=>{ const state=link.pc.connectionState; if(state==='failed'||state==='closed'||state==='disconnected'){if(this.role==='host')this.emitPeer(link.playerId,false);this.setStatus(state==='failed'?'failed':'connecting',state);} });
  }
  private attachReliable(link: PeerLink, channel: RTCDataChannel): void {
    link.reliable=channel;
    channel.addEventListener('open',()=>{ if(this.role==='host'){this.sendChannel(channel,{type:'assigned',playerId:link.playerId});this.emitPeer(link.playerId,true);this.setStatus('connected',`${this.connectedPlayerIds().length} peer(s) connected`);}else this.setStatus('connected','Connected to host'); });
    channel.addEventListener('message',(event)=>{ const message=this.parseMessage<ReliableMessage>(event.data);if(!message)return;if(this.role==='peer'&&message.type==='assigned'){this.localPlayerId=message.playerId;const hello:HelloMessage={type:'hello',name:this.displayName};this.sendChannel(channel,hello);}const from=this.role==='host'?link.playerId:1;for(const handler of this.reliableHandlers)handler(from,message); });
    channel.addEventListener('close',()=>{if(this.role==='host')this.emitPeer(link.playerId,false);});
  }
  private attachGame(link: PeerLink, channel: RTCDataChannel): void {
    link.game=channel;
    channel.addEventListener('open',()=>{if(this.role==='peer'&&link.reliable?.readyState==='open')this.setStatus('connected','Gameplay channel ready');});
    channel.addEventListener('message',(event)=>{const message=this.parseMessage<GameMessage>(event.data);if(!message)return;const from=this.role==='host'?link.playerId:1;for(const handler of this.gameHandlers)handler(from,message);});
  }
  private async applySignal(link: PeerLink, payload: SignalPayload): Promise<void> {
    if ('description' in payload) {
      await link.pc.setRemoteDescription(payload.description);
      if(link.pendingCandidates.length){const queued=link.pendingCandidates.splice(0);for(const candidate of queued){try{await link.pc.addIceCandidate(candidate);}catch{}}}
    } else if ('candidate' in payload) {
      if(!link.pc.remoteDescription){link.pendingCandidates.push(payload.candidate);return;}
      try{await link.pc.addIceCandidate(payload.candidate);}catch{}
    }
  }
  private signalTo(target:string,payload:SignalPayload):void{this.signaling.send({type:'signal',roomCode:this.roomCode,target,payload});}
  private sendChannel(channel:RTCDataChannel|undefined,message:unknown):void{if(channel?.readyState==='open')channel.send(JSON.stringify(message));}
  private parseMessage<T>(payload:unknown):T|undefined{if(typeof payload!=='string')return undefined;try{return JSON.parse(payload) as T;}catch{return undefined;}}
  private closeLink(link:PeerLink):void{link.reliable?.close();link.game?.close();link.pc.close();}
  private setStatus(status:NetworkStatus,detail?:string):void{this.status=status;for(const handler of this.statusHandlers)handler(status,detail);}
  private emitPeer(playerId:PlayerId,connected:boolean):void{for(const handler of this.peerHandlers)handler(playerId,connected);}
}
