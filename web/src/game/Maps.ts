import Phaser from 'phaser';
import type { GameModeId, MapId, MatchConfig } from '../net/Protocol';
import type { ArenaPhysics, PlatformDef } from '../sim/Simulation';

export interface ArenaTheme { id:MapId;name:string;skyTop:number;skyBottom:number;sun:number;cloud:number;grass:number;soil:number;accent:number;accent2:number;platforms:PlatformDef[];spawnXs:[number,number,number,number]; }
export const ARENAS:ArenaTheme[]=[
{id:'sky-garden',name:'Sky Garden',skyTop:0x5fc9ff,skyBottom:0xf9ddff,sun:0xffee83,cloud:0xffffff,grass:0x79dc70,soil:0x3c9a5b,accent:0x58d6ff,accent2:0xff76c2,platforms:[{x:150,y:420,width:210},{x:525,y:340,width:230},{x:900,y:420,width:210}],spawnXs:[105,425,855,1170]},
{id:'crystal-cove',name:'Crystal Cove',skyTop:0x6b79ff,skyBottom:0x90f2ff,sun:0xffb9f3,cloud:0xe9fbff,grass:0x6ee7d7,soil:0x315c87,accent:0x9d7cff,accent2:0x54f3ff,platforms:[{x:105,y:455,width:180},{x:365,y:365,width:180},{x:710,y:300,width:180},{x:995,y:430,width:175}],spawnXs:[90,410,820,1180]},
{id:'sunset-forge',name:'Sunset Forge',skyTop:0xff8f85,skyBottom:0xffd49a,sun:0xfff0a5,cloud:0xfff1da,grass:0xf4b35d,soil:0x804a52,accent:0xff5e8f,accent2:0xffd85b,platforms:[{x:205,y:390,width:190},{x:550,y:315,width:180},{x:880,y:390,width:190}],spawnXs:[105,455,825,1170]}
];
export interface ModeRules{id:GameModeId;name:string;description:string;gravityScale:number;damageScale:number;respawnMs:number;forcedWeapon?:number;}
export const MODES:ModeRules[]=[
{id:'ffa',name:'Free For All',description:'Classic balanced arena combat.',gravityScale:1,damageScale:1,respawnMs:1800},
{id:'turbo-ffa',name:'Turbo FFA',description:'Lower gravity, quicker respawns, faster chaos.',gravityScale:.78,damageScale:1,respawnMs:1000},
{id:'one-shot',name:'One Shot',description:'Shard Rifle only. Every clean hit is lethal.',gravityScale:1,damageScale:4,respawnMs:1600,forcedWeapon:3}
];
export const arenaById=(id:MapId)=>ARENAS.find((a)=>a.id===id)??ARENAS[0];
export const modeById=(id:GameModeId)=>MODES.find((m)=>m.id===id)??MODES[0];
export function physicsFor(config:MatchConfig):ArenaPhysics{const arena=arenaById(config.mapId),mode=modeById(config.modeId);return{minX:45,maxX:1235,groundY:590,gravityScale:mode.gravityScale,moveAcceleration:2100,maxMoveSpeed:config.modeId==='turbo-ffa'?400:360,jetAcceleration:config.modeId==='turbo-ffa'?2200:2050,maxFallSpeed:780,fuelUsePerSecond:30,fuelRechargeGround:36,fuelRechargeAir:13,platforms:arena.platforms};}
export function drawArena(scene:Phaser.Scene,config:MatchConfig):void{const arena=arenaById(config.mapId),g=scene.add.graphics();g.fillGradientStyle(arena.skyTop,arena.skyTop,arena.skyBottom,arena.skyBottom,1).fillRect(0,0,1280,720);g.fillStyle(arena.sun,.95).fillCircle(1120,88,48);g.fillStyle(arena.sun,.16).fillCircle(1120,88,76);for(const[x,y,s]of[[85,110,1.1],[420,160,.82],[805,115,.95]]as const){g.fillStyle(arena.cloud,.78);g.fillCircle(x,y,25*s);g.fillCircle(x+31*s,y-11*s,35*s);g.fillCircle(x+71*s,y,27*s);g.fillRoundedRect(x-15*s,y,102*s,28*s,14*s);}for(let i=0;i<9;i++){const x=35+i*160,y=190+(i%3)*24;g.fillStyle(i%2===0?arena.accent:arena.accent2,.09).fillCircle(x,y,42+(i%3)*12);}const land=scene.add.graphics();land.fillStyle(arena.grass,1).fillRoundedRect(0,558,1280,48,16);land.fillStyle(arena.soil,1).fillRect(0,596,1280,124);land.fillStyle(0xffffff,.16).fillRect(0,596,1280,7);for(const pdef of arena.platforms){const p=scene.add.graphics();p.fillStyle(0xffffff,.88).fillRoundedRect(pdef.x,pdef.y-10,pdef.width,20,10);p.fillStyle(arena.accent,.95).fillRoundedRect(pdef.x+14,pdef.y+7,pdef.width-28,10,6);p.fillStyle(arena.accent2,.26).fillTriangle(pdef.x+25,pdef.y+17,pdef.x+pdef.width-25,pdef.y+17,pdef.x+pdef.width/2,pdef.y+68);}scene.add.text(640,20,arena.name,{fontFamily:'Arial Black',fontSize:'20px',color:'#fff',stroke:'#315070',strokeThickness:5}).setOrigin(.5,0).setAlpha(.72).setDepth(2);}
