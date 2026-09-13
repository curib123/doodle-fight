import type { PlayerInputSample, PlayerStateNet } from '../net/Protocol';

export interface PlatformDef { x:number; y:number; width:number; }
export interface ArenaPhysics {
  minX:number; maxX:number; groundY:number; gravityScale:number; moveAcceleration:number; maxMoveSpeed:number; jetAcceleration:number; maxFallSpeed:number; fuelUsePerSecond:number; fuelRechargeGround:number; fuelRechargeAir:number; platforms:PlatformDef[];
}
export interface SimBody { x:number; y:number; vx:number; vy:number; fuel:number; alive:boolean; }

export const DEFAULT_PHYSICS: ArenaPhysics = { minX:54,maxX:1226,groundY:590,gravityScale:1,moveAcceleration:2100,maxMoveSpeed:360,jetAcceleration:2050,maxFallSpeed:780,fuelUsePerSecond:30,fuelRechargeGround:36,fuelRechargeAir:13,platforms:[] };
const clamp=(value:number,min:number,max:number)=>Math.max(min,Math.min(max,value));

export function simulateMovement(body:SimBody,input:PlayerInputSample,physics:ArenaPhysics):void{
  if(!body.alive)return;const dt=clamp(input.dt,1/240,.05);const previousY=body.y;const previousX=body.x;
  body.vx+=clamp(input.moveAxis,-1,1)*physics.moveAcceleration*dt;if(Math.abs(input.moveAxis)<.04)body.vx*=Math.pow(.0015,dt);body.vx=clamp(body.vx,-physics.maxMoveSpeed,physics.maxMoveSpeed);
  body.vy+=1600*physics.gravityScale*dt;if(input.jetpack&&body.fuel>0){body.vy-=physics.jetAcceleration*dt;body.fuel=Math.max(0,body.fuel-physics.fuelUsePerSecond*dt);}else{const onGround=Math.abs(body.y-physics.groundY)<.5;body.fuel=Math.min(100,body.fuel+(onGround?physics.fuelRechargeGround:physics.fuelRechargeAir)*dt);}
  body.vy=clamp(body.vy,-760,physics.maxFallSpeed);body.x=clamp(body.x+body.vx*dt,physics.minX,physics.maxX);body.y+=body.vy*dt;
  if(body.vy>=0){let landingY=Number.POSITIVE_INFINITY;for(const platform of physics.platforms){const withinX=body.x>=platform.x&&body.x<=platform.x+platform.width;const crossedTop=previousY<=platform.y&&body.y>=platform.y;const wasNear=previousX>=platform.x-10&&previousX<=platform.x+platform.width+10;if(withinX&&wasNear&&crossedTop&&platform.y<landingY)landingY=platform.y;}if(landingY!==Number.POSITIVE_INFINITY){body.y=landingY;body.vy=0;}}
  if(body.y>=physics.groundY){body.y=physics.groundY;if(body.vy>0)body.vy=0;}
}

export class InputHistory {
  private entries:{input:PlayerInputSample}[]=[];constructor(private readonly maxEntries=180){}
  push(input:PlayerInputSample):void{this.entries.push({input});if(this.entries.length>this.maxEntries)this.entries.splice(0,this.entries.length-this.maxEntries);}
  acknowledge(sequence:number):void{const first=this.entries.findIndex((entry)=>entry.input.seq>sequence);if(first===-1)this.entries.length=0;else if(first>0)this.entries.splice(0,first);}
  replay(state:SimBody,physics:ArenaPhysics):void{for(const entry of this.entries)simulateMovement(state,entry.input,physics);}
  pending(){return this.entries as readonly {input:PlayerInputSample}[];}
  clear():void{this.entries.length=0;}
}

interface BufferedState{receivedAt:number;state:PlayerStateNet;}
export class StateInterpolator{
  private samples:BufferedState[]=[];
  push(state:PlayerStateNet,receivedAt=performance.now()):void{this.samples.push({receivedAt,state:{...state}});if(this.samples.length>12)this.samples.splice(0,this.samples.length-12);}
  sample(delayMs=110,now=performance.now()):PlayerStateNet|undefined{if(!this.samples.length)return undefined;const target=now-delayMs;while(this.samples.length>=3&&this.samples[1].receivedAt<=target)this.samples.shift();if(this.samples.length===1)return{...this.samples[0].state};const a=this.samples[0],b=this.samples[1],span=Math.max(1,b.receivedAt-a.receivedAt),t=clamp((target-a.receivedAt)/span,0,1);return{...b.state,x:a.state.x+(b.state.x-a.state.x)*t,y:a.state.y+(b.state.y-a.state.y)*t,vx:a.state.vx+(b.state.vx-a.state.vx)*t,vy:a.state.vy+(b.state.vy-a.state.vy)*t,aimX:a.state.aimX+(b.state.aimX-a.state.aimX)*t,aimY:a.state.aimY+(b.state.aimY-a.state.aimY)*t,health:a.state.health+(b.state.health-a.state.health)*t,fuel:a.state.fuel+(b.state.fuel-a.state.fuel)*t};}
  clear():void{this.samples.length=0;}
}
