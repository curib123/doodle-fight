export type PlayerId = 1 | 2 | 3 | 4;
export type GameModeId = 'ffa' | 'turbo-ffa' | 'one-shot';
export type MapId = 'sky-garden' | 'crystal-cove' | 'sunset-forge';

export interface PlayerInputSample {
  seq: number;
  dt: number;
  moveAxis: number;
  aimX: number;
  aimY: number;
  jetpack: boolean;
  firing: boolean;
  weaponIndex: number;
  reloadSeq: number;
}

export interface InputBatchMessage {
  type: 'input-batch';
  playerId: PlayerId;
  samples: PlayerInputSample[];
}

export interface PlayerStateNet {
  id: PlayerId;
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
  score: number;
  respawnMs: number;
}

export interface ProjectileStateNet {
  id: number;
  x: number;
  y: number;
  color: number;
  radius: number;
  ownerId: PlayerId;
}

export interface SnapshotMessage {
  type: 'snapshot';
  serverTick: number;
  sentAt: number;
  ackInputSeq: Partial<Record<PlayerId, number>>;
  players: PlayerStateNet[];
  projectiles: ProjectileStateNet[];
}

export interface LobbyPlayer {
  id: PlayerId;
  name: string;
  ready: boolean;
  connected: boolean;
  isHost: boolean;
}

export interface MatchConfig {
  mapId: MapId;
  modeId: GameModeId;
  scoreLimit: number;
}

export interface LobbyStateMessage {
  type: 'lobby-state';
  players: LobbyPlayer[];
  config: MatchConfig;
  roomCode: string;
}

export interface HelloMessage { type: 'hello'; name: string; }
export interface AssignedMessage { type: 'assigned'; playerId: PlayerId; }
export interface ReadyMessage { type: 'ready'; ready: boolean; }
export interface ConfigMessage { type: 'config'; config: MatchConfig; }
export interface StartMatchMessage { type: 'start-match'; players: LobbyPlayer[]; config: MatchConfig; }
export interface MatchEndMessage { type: 'match-end'; winnerId: PlayerId; scores: Partial<Record<PlayerId, number>>; }
export interface PingMessage { type: 'ping'; nonce: number; }
export interface PongMessage { type: 'pong'; nonce: number; }

export type ReliableMessage = HelloMessage | AssignedMessage | ReadyMessage | ConfigMessage | LobbyStateMessage | StartMatchMessage | MatchEndMessage | PingMessage | PongMessage;
export type GameMessage = InputBatchMessage | SnapshotMessage;

export interface SignalingEnvelope {
  type: 'register-host' | 'hosted' | 'join-room' | 'join-accepted' | 'peer-joined' | 'peer-left' | 'signal' | 'error';
  roomCode?: string;
  clientId?: string;
  target?: string;
  from?: string;
  payload?: unknown;
  message?: string;
}
