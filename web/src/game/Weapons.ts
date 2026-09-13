export interface WeaponDef {
  id: string; name: string; damage: number; speed: number; fireInterval: number; clipSize: number; reloadTime: number; color: number; spread: number; radius: number; projectileLife: number;
}

export const WEAPONS: WeaponDef[] = [
  { id:'pulse-pistol', name:'Pulse Pistol', damage:18, speed:760, fireInterval:0.22, clipSize:12, reloadTime:1.1, color:0x56efff, spread:0.01, radius:6, projectileLife:1.6 },
  { id:'burst-blaster', name:'Burst Blaster', damage:11, speed:835, fireInterval:0.105, clipSize:20, reloadTime:1.3, color:0xff6f9d, spread:0.035, radius:5, projectileLife:1.45 },
  { id:'cloud-cannon', name:'Cloud Cannon', damage:25, speed:590, fireInterval:0.44, clipSize:8, reloadTime:1.45, color:0xd98bff, spread:0.06, radius:9, projectileLife:1.75 },
  { id:'shard-rifle', name:'Shard Rifle', damage:34, speed:980, fireInterval:0.62, clipSize:6, reloadTime:1.55, color:0x64baff, spread:0.008, radius:7, projectileLife:1.35 },
  { id:'nova-launcher', name:'Nova Launcher', damage:44, speed:500, fireInterval:0.86, clipSize:4, reloadTime:1.7, color:0xffd94f, spread:0.035, radius:13, projectileLife:1.8 }
];
