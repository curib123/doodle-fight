import type { SignalingEnvelope } from './Protocol';

type SignalHandler = (message: SignalingEnvelope) => void;

export class SignalingClient {
  private socket?: WebSocket;
  private handlers = new Set<SignalHandler>();
  private openPromise?: Promise<void>;

  constructor(readonly url = SignalingClient.defaultUrl()) {}

  static defaultUrl(): string {
    const configured = import.meta.env.VITE_SIGNALING_URL as string | undefined;
    if (configured) return configured;
    const scheme = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    return `${scheme}//${window.location.hostname}:8787`;
  }

  connect(): Promise<void> {
    if (this.socket?.readyState === WebSocket.OPEN) return Promise.resolve();
    if (this.openPromise) return this.openPromise;

    this.openPromise = new Promise<void>((resolve, reject) => {
      const socket = new WebSocket(this.url);
      this.socket = socket;
      const timeout = window.setTimeout(() => reject(new Error('Signaling connection timed out.')), 7000);
      socket.addEventListener('open', () => { window.clearTimeout(timeout); resolve(); }, { once: true });
      socket.addEventListener('error', () => { window.clearTimeout(timeout); reject(new Error(`Could not connect to signaling server: ${this.url}`)); }, { once: true });
      socket.addEventListener('message', (event) => {
        if (typeof event.data !== 'string') return;
        try {
          const message = JSON.parse(event.data) as SignalingEnvelope;
          for (const handler of this.handlers) handler(message);
        } catch {}
      });
      socket.addEventListener('close', () => { this.openPromise = undefined; });
    });
    return this.openPromise;
  }

  onMessage(handler: SignalHandler): () => void { this.handlers.add(handler); return () => this.handlers.delete(handler); }
  send(message: SignalingEnvelope): void {
    if (this.socket?.readyState !== WebSocket.OPEN) throw new Error('Signaling socket is not connected.');
    this.socket.send(JSON.stringify(message));
  }
  close(): void { this.socket?.close(); this.socket = undefined; this.openPromise = undefined; }
}
