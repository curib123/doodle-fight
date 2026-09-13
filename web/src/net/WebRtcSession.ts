export type WebRtcRole = 'host' | 'peer';
export type ConnectionStatus = 'new' | 'connecting' | 'connected' | 'disconnected' | 'failed' | 'closed';

export interface P2PInputMessage {
  type: 'input';
  seq: number;
  moveAxis: number;
  aimX: number;
  aimY: number;
  jetpack: boolean;
  firing: boolean;
  weaponIndex: number;
  reloadSeq: number;
}

export interface P2PEntityState {
  x: number;
  y: number;
  vx: number;
  vy: number;
  aimX: number;
  aimY: number;
  health: number;
  fuel: number;
  alive: boolean;
  weaponIndex: number;
  ammo: number;
  koCount: number;
}

export interface P2PProjectileState {
  id: number;
  x: number;
  y: number;
  color: number;
  radius: number;
}

export interface P2PSnapshotMessage {
  type: 'snapshot';
  ackInputSeq: number;
  host: P2PEntityState;
  peer: P2PEntityState;
  projectiles: P2PProjectileState[];
}

export type P2PMessage = P2PInputMessage | P2PSnapshotMessage;

type MessageHandler = (message: P2PMessage) => void;
type StateHandler = (status: ConnectionStatus) => void;

const encodeSignal = (description: RTCSessionDescriptionInit): string => btoa(JSON.stringify(description));
const decodeSignal = (code: string): RTCSessionDescriptionInit => JSON.parse(atob(code.trim())) as RTCSessionDescriptionInit;

export class WebRtcSession {
  readonly role: WebRtcRole;
  private readonly pc: RTCPeerConnection;
  private channel?: RTCDataChannel;
  private messageHandlers = new Set<MessageHandler>();
  private stateHandlers = new Set<StateHandler>();

  constructor(role: WebRtcRole) {
    this.role = role;
    this.pc = new RTCPeerConnection({
      iceServers: [
        { urls: 'stun:stun.l.google.com:19302' },
        { urls: 'stun:stun1.l.google.com:19302' }
      ]
    });

    this.pc.addEventListener('connectionstatechange', () => {
      this.emitState(this.pc.connectionState as ConnectionStatus);
    });

    if (role === 'host') {
      this.attachChannel(this.pc.createDataChannel('doodle-fight', {
        ordered: false,
        maxRetransmits: 0
      }));
    } else {
      this.pc.addEventListener('datachannel', (event) => this.attachChannel(event.channel));
    }
  }

  get connected(): boolean {
    return this.channel?.readyState === 'open';
  }

  onMessage(handler: MessageHandler): () => void {
    this.messageHandlers.add(handler);
    return () => this.messageHandlers.delete(handler);
  }

  onState(handler: StateHandler): () => void {
    this.stateHandlers.add(handler);
    handler(this.connected ? 'connected' : 'connecting');
    return () => this.stateHandlers.delete(handler);
  }

  send(message: P2PMessage): void {
    if (!this.connected || !this.channel) return;
    this.channel.send(JSON.stringify(message));
  }

  async createOfferCode(): Promise<string> {
    if (this.role !== 'host') throw new Error('Only the host can create an offer.');
    const offer = await this.pc.createOffer();
    await this.pc.setLocalDescription(offer);
    await this.waitForIceGathering();
    if (!this.pc.localDescription) throw new Error('Could not create host offer.');
    return encodeSignal(this.pc.localDescription.toJSON());
  }

  async acceptAnswerCode(code: string): Promise<void> {
    if (this.role !== 'host') throw new Error('Only the host can accept an answer.');
    await this.pc.setRemoteDescription(decodeSignal(code));
  }

  async acceptOfferCode(code: string): Promise<string> {
    if (this.role !== 'peer') throw new Error('Only a joining peer can accept an offer.');
    await this.pc.setRemoteDescription(decodeSignal(code));
    const answer = await this.pc.createAnswer();
    await this.pc.setLocalDescription(answer);
    await this.waitForIceGathering();
    if (!this.pc.localDescription) throw new Error('Could not create join answer.');
    return encodeSignal(this.pc.localDescription.toJSON());
  }

  close(): void {
    this.channel?.close();
    this.pc.close();
    this.emitState('closed');
  }

  private attachChannel(channel: RTCDataChannel): void {
    this.channel = channel;
    channel.binaryType = 'arraybuffer';
    channel.addEventListener('open', () => this.emitState('connected'));
    channel.addEventListener('close', () => this.emitState('closed'));
    channel.addEventListener('error', () => this.emitState('failed'));
    channel.addEventListener('message', (event) => {
      if (typeof event.data !== 'string') return;
      try {
        const parsed = JSON.parse(event.data) as P2PMessage;
        for (const handler of this.messageHandlers) handler(parsed);
      } catch {
        // Ignore malformed peer packets instead of crashing the match.
      }
    });
  }

  private async waitForIceGathering(): Promise<void> {
    if (this.pc.iceGatheringState === 'complete') return;
    await new Promise<void>((resolve) => {
      const listener = () => {
        if (this.pc.iceGatheringState !== 'complete') return;
        this.pc.removeEventListener('icegatheringstatechange', listener);
        resolve();
      };
      this.pc.addEventListener('icegatheringstatechange', listener);
      window.setTimeout(() => {
        this.pc.removeEventListener('icegatheringstatechange', listener);
        resolve();
      }, 5000);
    });
  }

  private emitState(status: ConnectionStatus): void {
    for (const handler of this.stateHandlers) handler(status);
  }
}
