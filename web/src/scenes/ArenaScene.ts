import Phaser from 'phaser';
import { drawArena, modeById, physicsFor } from '../game/Maps';
import { WEAPONS } from '../game/Weapons';
import type {
  InputBatchMessage,
  LobbyPlayer,
  MatchConfig,
  PlayerId,
  PlayerInputSample,
  PlayerStateNet,
  SnapshotMessage
} from '../net/Protocol';
import { WebRtcMesh } from '../net/WebRtcMesh';
import { InputHistory, simulateMovement, StateInterpolator } from '../sim/Simulation';

type Role = 'practice' | 'host' | 'peer';
interface InitData {
  role?: Role;
  session?: WebRtcMesh;
  config?: MatchConfig;
  lobbyPlayers?: LobbyPlayer[];
  localPlayerId?: PlayerId;
}
interface Fighter {
  id: PlayerId;
  name: string;
  sprite: Phaser.GameObjects.Sprite;
  x: number; y: number; vx: number; vy: number;
  aimX: number; aimY: number;
  health: number; fuel: number; alive: boolean;
  respawnMs: number; weaponIndex: number; ammo: number; score: number;
  fireCd: number; reloadCd: number; appliedReloadSeq: number;
  spawnX: number; botTimer: number; botMove: number;
}
interface Shot {
  id: number; ownerId: PlayerId;
  x: number; y: number; vx: number; vy: number;
  life: number; damage: number; color: number; radius: number;
  body: Phaser.GameObjects.Arc; glow: Phaser.GameObjects.Arc;
}

const TINTS: Record<PlayerId, number> = { 1: 0xffffff, 2: 0xcff7ff, 3: 0xffd1ec, 4: 0xddffd4 };

export class ArenaScene extends Phaser.Scene {
  private role: Role = 'practice';
  private session?: WebRtcMesh;
  private config: MatchConfig = { mapId: 'sky-garden', modeId: 'ffa', scoreLimit: 10 };
  private players: LobbyPlayer[] = [];
  private localId: PlayerId = 1;
  private fighters = new Map<PlayerId, Fighter>();
  private shots: Shot[] = [];
  private nextShotId = 1;
  private latestPeerInput = new Map<PlayerId, PlayerInputSample>();
  private acknowledgements: Partial<Record<PlayerId, number>> = {};
  private history = new InputHistory();
  private interpolators = new Map<PlayerId, StateInterpolator>();
  private outbound: PlayerInputSample[] = [];
  private seq = 0;
  private reloadSeq = 0;
  private inputTimer = 0;
  private snapshotTimer = 0;
  private serverTick = 0;
  private ended = false;
  private cleanup: Array<() => void> = [];

  private physics = physicsFor(this.config);
  private rules = modeById(this.config.modeId);
  private keys!: Record<'left'|'right'|'jet'|'reload'|'prev'|'next'|'menu', Phaser.Input.Keyboard.Key>;
  private aim = new Phaser.Math.Vector2(980, 300);
  private mouseFire = false;
  private touchMove = 0;
  private touchJet = false;
  private touchFire = false;
  private touchAim = new Phaser.Math.Vector2(1, 0);
  private movePointer: number | null = null;
  private aimPointer: number | null = null;
  private moveOrigin = new Phaser.Math.Vector2();
  private aimOrigin = new Phaser.Math.Vector2();
  private moveBase?: Phaser.GameObjects.Arc;
  private moveKnob?: Phaser.GameObjects.Arc;
  private aimBase?: Phaser.GameObjects.Arc;
  private aimKnob?: Phaser.GameObjects.Arc;
  private reticle?: Phaser.GameObjects.Container;
  private hud?: Phaser.GameObjects.Text;
  private scoreboard?: Phaser.GameObjects.Text;
  private status?: Phaser.GameObjects.Text;

  constructor() { super('ArenaScene'); }

  init(data: InitData): void {
    this.role = data.role ?? 'practice';
    this.session = data.session;
    this.config = data.config ?? { mapId: 'sky-garden', modeId: 'ffa', scoreLimit: 10 };
    this.players = data.lobbyPlayers ?? [{ id: 1, name: 'Sky Explorer', ready: true, connected: true, isHost: true }];
    this.localId = data.localPlayerId ?? 1;
    this.physics = physicsFor(this.config);
    this.rules = modeById(this.config.modeId);
    this.fighters.clear(); this.shots = []; this.latestPeerInput.clear();
    this.interpolators.clear(); this.history.clear(); this.outbound = [];
    this.seq = 0; this.reloadSeq = 0; this.ended = false;
  }

  create(): void {
    drawArena(this, this.config);
    this.createFighters();
    this.createHud();
    this.createControls();
    this.bindNetwork();
    this.events.once(Phaser.Scenes.Events.SHUTDOWN, () => {
      this.cleanup.forEach((off) => off());
      this.cleanup = [];
    });
  }

  update(_: number, ms: number): void {
    if (this.ended) return;
    const dt = Math.min(ms / 1000, 0.05);
    this.updateCooldowns(dt);
    const input = this.captureInput(dt);

    if (this.role === 'practice') {
      this.applyInput(this.local(), input, true);
      this.updateBots(dt);
      this.updateShots(dt);
      this.updateRespawns(dt);
      this.checkWinner();
    } else if (this.role === 'host') {
      this.applyInput(this.local(), input, true);
      this.applyPeerInputs(dt);
      this.updateShots(dt);
      this.updateRespawns(dt);
      this.publishSnapshots(dt);
      this.checkWinner();
    } else {
      this.predictLocal(input);
      this.sendInputs(dt);
      this.renderInterpolatedRemotes();
    }

    this.renderFighters();
    this.renderHud();
    this.reticle?.setPosition(this.aim.x, this.aim.y);
  }

  private createFighters(): void {
    const source = this.role === 'practice'
      ? [
          { id:1, name:'Sky Explorer', ready:true, connected:true, isHost:true },
          { id:2, name:'Mochi Bot', ready:true, connected:true, isHost:false },
          { id:3, name:'Nova Bot', ready:true, connected:true, isHost:false },
          { id:4, name:'Pip Bot', ready:true, connected:true, isHost:false }
        ] as LobbyPlayer[]
      : this.players;

    const spawns = [105, 425, 855, 1170];
    for (const p of source) {
      const weaponIndex = this.rules.forcedWeapon ?? ((p.id - 1) % WEAPONS.length);
      const sprite = this.add.sprite(spawns[p.id - 1], this.physics.groundY - 64, 'hero', 0)
        .setScale(1.25).setTint(TINTS[p.id]).play('hero-idle');
      this.fighters.set(p.id, {
        id:p.id, name:p.name, sprite, x:spawns[p.id-1], y:this.physics.groundY,
        vx:0, vy:0, aimX:p.id===1?1:-1, aimY:0, health:100, fuel:100, alive:true,
        respawnMs:0, weaponIndex, ammo:WEAPONS[weaponIndex].clipSize, score:0,
        fireCd:0, reloadCd:0, appliedReloadSeq:0, spawnX:spawns[p.id-1],
        botTimer:0.5 + p.id * 0.15, botMove:p.id % 2 === 0 ? -1 : 1
      });
      this.interpolators.set(p.id, new StateInterpolator());
    }

    const g = this.add.graphics();
    g.lineStyle(3, 0xff5ca8, .95).strokeCircle(0, 0, 13);
    g.lineBetween(-22,0,-8,0); g.lineBetween(22,0,8,0); g.lineBetween(0,-22,0,-8); g.lineBetween(0,22,0,8);
    this.reticle = this.add.container(this.aim.x, this.aim.y, [g]).setDepth(120);
  }

  private createHud(): void {
    this.hud = this.add.text(22, 58, '', { fontFamily:'Arial Black', fontSize:'18px', color:'#fff', stroke:'#2e506d', strokeThickness:5 }).setDepth(120);
    this.scoreboard = this.add.text(1258, 58, '', { fontFamily:'Arial', fontSize:'18px', color:'#29465f', align:'right', lineSpacing:6 }).setOrigin(1,0).setDepth(120);
    this.status = this.add.text(640, 52, '', { fontFamily:'Arial Black', fontSize:'15px', color:'#fff', stroke:'#5c4a7a', strokeThickness:4 }).setOrigin(.5,0).setDepth(120);
  }

  private createControls(): void {
    this.keys = this.input.keyboard!.addKeys({
      left:Phaser.Input.Keyboard.KeyCodes.A, right:Phaser.Input.Keyboard.KeyCodes.D,
      jet:Phaser.Input.Keyboard.KeyCodes.SPACE, reload:Phaser.Input.Keyboard.KeyCodes.R,
      prev:Phaser.Input.Keyboard.KeyCodes.Q, next:Phaser.Input.Keyboard.KeyCodes.E,
      menu:Phaser.Input.Keyboard.KeyCodes.M
    }) as typeof this.keys;

    this.moveBase = this.add.circle(112, 615, 66, 0xffffff, .16).setDepth(120);
    this.moveKnob = this.add.circle(112, 615, 31, 0xffffff, .34).setDepth(121);
    this.aimBase = this.add.circle(1115, 615, 66, 0xffffff, .16).setDepth(120);
    this.aimKnob = this.add.circle(1115, 615, 31, 0xff83c5, .34).setDepth(121);

    const jet = this.add.text(1090, 475, 'JET', { fontFamily:'Arial Black', fontSize:'22px', color:'#fff', backgroundColor:'#45b9d8', padding:{x:18,y:12} }).setOrigin(.5).setDepth(125).setInteractive();
    jet.on('pointerdown', () => { this.touchJet = true; });
    jet.on('pointerup', () => { this.touchJet = false; });
    jet.on('pointerout', () => { this.touchJet = false; });

    this.input.on('pointerdown', (p: Phaser.Input.Pointer) => {
      if (p.wasTouch && p.x < 360 && p.y > 420 && this.movePointer === null) {
        this.movePointer = p.id; this.moveOrigin.set(p.x,p.y);
        this.moveBase?.setPosition(p.x,p.y); this.moveKnob?.setPosition(p.x,p.y); return;
      }
      if (p.wasTouch && p.x > 700 && p.y > 360 && this.aimPointer === null) {
        this.aimPointer = p.id; this.aimOrigin.set(p.x,p.y); this.touchFire = true; return;
      }
      if (!p.wasTouch) this.mouseFire = true;
    });
    this.input.on('pointermove', (p: Phaser.Input.Pointer) => {
      if (p.id === this.movePointer) {
        const dx = Phaser.Math.Clamp(p.x - this.moveOrigin.x, -55, 55);
        this.touchMove = dx / 55; this.moveKnob?.setPosition(this.moveOrigin.x + dx, this.moveOrigin.y);
      }
      if (p.id === this.aimPointer) {
        const v = new Phaser.Math.Vector2(p.x - this.aimOrigin.x, p.y - this.aimOrigin.y);
        if (v.length() > 8) this.touchAim.copy(v.normalize());
        this.aimKnob?.setPosition(this.aimOrigin.x + this.touchAim.x * 45, this.aimOrigin.y + this.touchAim.y * 45);
      }
      if (!p.wasTouch) this.aim.set(p.x,p.y);
    });
    this.input.on('pointerup', (p: Phaser.Input.Pointer) => {
      if (p.id === this.movePointer) {
        this.movePointer = null; this.touchMove = 0;
        this.moveBase?.setPosition(112,615); this.moveKnob?.setPosition(112,615);
      }
      if (p.id === this.aimPointer) {
        this.aimPointer = null; this.touchFire = false;
        this.aimBase?.setPosition(1115,615); this.aimKnob?.setPosition(1115,615);
      }
      if (!p.wasTouch) this.mouseFire = false;
    });
  }

  private bindNetwork(): void {
    if (!this.session) return;
    this.cleanup.push(this.session.onStatus((state, detail) => this.status?.setText(`${state.toUpperCase()}${detail ? ` • ${detail}` : ''}`)));
    this.cleanup.push(this.session.onGame((from, message) => {
      if (this.role === 'host' && message.type === 'input-batch') {
        const batch = message as InputBatchMessage;
        const newest = batch.samples[batch.samples.length - 1];
        if (newest) this.latestPeerInput.set(from, newest);
      }
      if (this.role === 'peer' && message.type === 'snapshot') this.consumeSnapshot(message as SnapshotMessage);
    }));
    this.cleanup.push(this.session.onReliable((_from, message) => {
      if (message.type === 'match-end') this.endMatch(message.winnerId);
    }));
  }

  private captureInput(dt: number): PlayerInputSample {
    if (Phaser.Input.Keyboard.JustDown(this.keys.prev)) this.switchWeapon(-1);
    if (Phaser.Input.Keyboard.JustDown(this.keys.next)) this.switchWeapon(1);
    if (Phaser.Input.Keyboard.JustDown(this.keys.reload)) this.reloadSeq++;
    if (Phaser.Input.Keyboard.JustDown(this.keys.menu)) { this.session?.close(); this.scene.start('MenuScene'); }

    let moveAxis = (this.keys.left.isDown ? -1 : 0) + (this.keys.right.isDown ? 1 : 0);
    if (Math.abs(this.touchMove) > Math.abs(moveAxis)) moveAxis = this.touchMove;
    const local = this.local();
    let aimX = this.touchAim.x, aimY = this.touchAim.y;
    if (this.aimPointer === null) {
      const v = new Phaser.Math.Vector2(this.aim.x - local.x, this.aim.y - (local.y - 64));
      if (v.lengthSq() > 1) { v.normalize(); aimX = v.x; aimY = v.y; }
    } else {
      this.aim.set(local.x + aimX * 180, local.y - 64 + aimY * 180);
    }
    return {
      seq: ++this.seq, dt, moveAxis: Phaser.Math.Clamp(moveAxis,-1,1), aimX, aimY,
      jetpack: this.keys.jet.isDown || this.touchJet,
      firing: this.mouseFire || this.touchFire || this.input.activePointer.leftButtonDown(),
      weaponIndex: local.weaponIndex, reloadSeq: this.reloadSeq
    };
  }

  private applyInput(f: Fighter, input: PlayerInputSample, authoritative: boolean): void {
    f.aimX = input.aimX; f.aimY = input.aimY;
    f.weaponIndex = this.rules.forcedWeapon ?? Phaser.Math.Clamp(input.weaponIndex, 0, WEAPONS.length - 1);
    simulateMovement(f, input, this.physics);
    if (!authoritative) return;
    if (input.reloadSeq > f.appliedReloadSeq) { f.appliedReloadSeq = input.reloadSeq; this.beginReload(f); }
    if (input.firing) this.fire(f);
  }

  private predictLocal(input: PlayerInputSample): void {
    this.history.push(input); this.outbound.push(input);
    if (this.outbound.length > 12) this.outbound.splice(0, this.outbound.length - 12);
    this.applyInput(this.local(), input, false);
  }

  private sendInputs(dt: number): void {
    if (!this.session) return;
    this.inputTimer += dt;
    if (this.inputTimer < 1 / 30) return;
    this.inputTimer = 0;
    if (!this.outbound.length) return;
    this.session.sendGame({ type:'input-batch', playerId:this.localId, samples:[...this.outbound] });
    this.outbound = [];
  }

  private applyPeerInputs(dt: number): void {
    for (const [id, input] of this.latestPeerInput) {
      const f = this.fighters.get(id); if (!f) continue;
      const sample = { ...input, dt: Math.min(dt, .05) };
      this.applyInput(f, sample, true); this.acknowledgements[id] = input.seq;
    }
  }

  private publishSnapshots(dt: number): void {
    if (!this.session) return;
    this.snapshotTimer += dt;
    if (this.snapshotTimer < 1 / 20) return;
    this.snapshotTimer = 0;
    const snapshot: SnapshotMessage = {
      type:'snapshot', serverTick:++this.serverTick, sentAt:performance.now(),
      ackInputSeq:{...this.acknowledgements},
      players:[...this.fighters.values()].map((f) => this.toNet(f)),
      projectiles:this.shots.map((s) => ({ id:s.id, x:s.x, y:s.y, color:s.color, radius:s.radius, ownerId:s.ownerId }))
    };
    this.session.sendGame(snapshot);
  }

  private consumeSnapshot(snapshot: SnapshotMessage): void {
    const ack = snapshot.ackInputSeq[this.localId] ?? 0;
    const localState = snapshot.players.find((p) => p.id === this.localId);
    if (localState) {
      const local = this.local(); this.applyNet(local, localState);
      this.history.acknowledge(ack); this.history.replay(local, this.physics);
    }
    for (const state of snapshot.players) {
      if (state.id === this.localId) continue;
      this.interpolators.get(state.id)?.push(state);
    }
  }

  private renderInterpolatedRemotes(): void {
    for (const [id, interpolator] of this.interpolators) {
      if (id === this.localId) continue;
      const state = interpolator.sample(110); const f = this.fighters.get(id);
      if (state && f) this.applyNet(f, state);
    }
  }

  private updateBots(dt: number): void {
    const target = this.local();
    for (const f of this.fighters.values()) {
      if (f.id === this.localId || !f.alive) continue;
      f.botTimer -= dt;
      if (f.botTimer <= 0) { f.botTimer = Phaser.Math.FloatBetween(.45, 1.1); f.botMove = Phaser.Math.Between(0,1) ? 1 : -1; }
      const v = new Phaser.Math.Vector2(target.x - f.x, target.y - f.y); if (v.lengthSq() > 1) v.normalize();
      const sample: PlayerInputSample = { seq:0, dt, moveAxis:f.botMove, aimX:v.x, aimY:v.y, jetpack:f.y > target.y + 45, firing:true, weaponIndex:f.weaponIndex, reloadSeq:f.appliedReloadSeq };
      this.applyInput(f, sample, true);
    }
  }

  private updateCooldowns(dt: number): void {
    for (const f of this.fighters.values()) {
      f.fireCd = Math.max(0, f.fireCd - dt);
      if (f.reloadCd > 0) {
        f.reloadCd = Math.max(0, f.reloadCd - dt);
        if (f.reloadCd === 0) f.ammo = WEAPONS[f.weaponIndex].clipSize;
      }
    }
  }

  private fire(f: Fighter): void {
    if (!f.alive || f.fireCd > 0 || f.reloadCd > 0) return;
    const w = WEAPONS[f.weaponIndex];
    if (f.ammo <= 0) { this.beginReload(f); return; }
    f.fireCd = w.fireInterval; f.ammo--;
    const angle = Math.atan2(f.aimY, f.aimX) + Phaser.Math.FloatBetween(-w.spread, w.spread);
    const x = f.x + Math.cos(angle) * 42, y = f.y - 64 + Math.sin(angle) * 18;
    const glow = this.add.circle(x,y,w.radius+6,w.color,.28).setDepth(40);
    const body = this.add.circle(x,y,w.radius,0xffffff,1).setStrokeStyle(3,w.color).setDepth(41);
    this.shots.push({ id:this.nextShotId++, ownerId:f.id, x,y, vx:Math.cos(angle)*w.speed, vy:Math.sin(angle)*w.speed, life:w.projectileLife, damage:w.damage*this.rules.damageScale, color:w.color, radius:w.radius, body, glow });
    if (f.ammo <= 0) this.beginReload(f);
  }

  private updateShots(dt: number): void {
    for (let i = this.shots.length - 1; i >= 0; i--) {
      const s = this.shots[i]; s.life -= dt; s.x += s.vx * dt; s.y += s.vy * dt; s.body.setPosition(s.x,s.y); s.glow.setPosition(s.x,s.y);
      let remove = s.life <= 0 || s.x < -40 || s.x > 1320 || s.y < -40 || s.y > 760;
      if (!remove) for (const f of this.fighters.values()) {
        if (f.id === s.ownerId || !f.alive) continue;
        if (Phaser.Math.Distance.Between(s.x,s.y,f.x,f.y-60) < 34 + s.radius) { this.damage(f,s.damage,s.ownerId); remove = true; break; }
      }
      if (remove) { s.body.destroy(); s.glow.destroy(); this.shots.splice(i,1); }
    }
  }

  private damage(target: Fighter, amount: number, attackerId: PlayerId): void {
    target.health = Math.max(0, target.health - amount); if (target.health > 0) return;
    target.alive = false; target.respawnMs = this.rules.respawnMs; target.sprite.setVisible(false);
    const attacker = this.fighters.get(attackerId); if (attacker && attacker.id !== target.id) attacker.score++;
  }

  private updateRespawns(dt: number): void {
    for (const f of this.fighters.values()) {
      if (f.alive) continue; f.respawnMs -= dt * 1000; if (f.respawnMs > 0) continue;
      f.alive = true; f.health = 100; f.fuel = 100; f.vx = 0; f.vy = 0; f.x = f.spawnX; f.y = this.physics.groundY; f.ammo = WEAPONS[f.weaponIndex].clipSize; f.sprite.setVisible(true);
    }
  }

  private switchWeapon(direction: number): void {
    const f = this.local(); if (this.rules.forcedWeapon !== undefined) return;
    f.weaponIndex = (f.weaponIndex + direction + WEAPONS.length) % WEAPONS.length; f.ammo = WEAPONS[f.weaponIndex].clipSize;
  }
  private beginReload(f: Fighter): void { if (f.reloadCd <= 0) f.reloadCd = WEAPONS[f.weaponIndex].reloadTime; }

  private renderFighters(): void {
    for (const f of this.fighters.values()) {
      f.sprite.setPosition(f.x, f.y - 64).setFlipX(f.aimX < 0).setVisible(f.alive);
      if (!f.alive) continue;
      const animation = Math.abs(f.vy) > 20 || f.y < this.physics.groundY - 5 ? 'hero-fly' : Math.abs(f.vx) > 35 ? 'hero-run' : 'hero-idle';
      if (f.sprite.anims.currentAnim?.key !== animation) f.sprite.play(animation, true);
    }
  }

  private renderHud(): void {
    const l = this.local(), w = WEAPONS[l.weaponIndex];
    this.hud?.setText([`${w.name}  ${l.ammo}/${w.clipSize}`,`HP ${Math.ceil(l.health)}   Fuel ${Math.ceil(l.fuel)}`,`KOs ${l.score}/${this.config.scoreLimit}`]);
    this.scoreboard?.setText([...this.fighters.values()].sort((a,b)=>b.score-a.score).map((f)=>`P${f.id} ${f.name}: ${f.score}`).join('\n'));
    if (this.role === 'practice') this.status?.setText('PRACTICE • 4-player FFA simulation');
  }

  private checkWinner(): void {
    const winner = [...this.fighters.values()].find((f) => f.score >= this.config.scoreLimit); if (!winner) return;
    if (this.role === 'host') {
      const scores: Partial<Record<PlayerId, number>> = {};
      for (const f of this.fighters.values()) scores[f.id] = f.score;
      this.session?.sendReliable({ type:'match-end', winnerId:winner.id, scores });
    }
    this.endMatch(winner.id);
  }
  private endMatch(winnerId: PlayerId): void {
    if (this.ended) return; this.ended = true;
    const winner = this.fighters.get(winnerId);
    this.add.rectangle(640,360,760,210,0x29465f,.88).setDepth(200);
    this.add.text(640,325,`${winner?.name ?? `Player ${winnerId}`} WINS!`,{fontFamily:'Arial Black',fontSize:'40px',color:'#fff'}).setOrigin(.5).setDepth(201);
    this.add.text(640,385,'Press M to return to menu',{fontFamily:'Arial',fontSize:'22px',color:'#fff'}).setOrigin(.5).setDepth(201);
    this.input.keyboard?.once('keydown-M',()=>{this.session?.close();this.scene.start('MenuScene');});
  }

  private local(): Fighter { const f = this.fighters.get(this.localId); if (!f) throw new Error(`Local player ${this.localId} is missing.`); return f; }
  private toNet(f: Fighter): PlayerStateNet { return { id:f.id,x:f.x,y:f.y,vx:f.vx,vy:f.vy,aimX:f.aimX,aimY:f.aimY,health:f.health,fuel:f.fuel,alive:f.alive,weaponIndex:f.weaponIndex,ammo:f.ammo,score:f.score,respawnMs:f.respawnMs }; }
  private applyNet(f: Fighter, state: PlayerStateNet): void { f.x=state.x;f.y=state.y;f.vx=state.vx;f.vy=state.vy;f.aimX=state.aimX;f.aimY=state.aimY;f.health=state.health;f.fuel=state.fuel;f.alive=state.alive;f.weaponIndex=state.weaponIndex;f.ammo=state.ammo;f.score=state.score;f.respawnMs=state.respawnMs; }
}
