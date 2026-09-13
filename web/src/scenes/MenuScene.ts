import Phaser from 'phaser';
import { SignalingClient } from '../net/SignalingClient';
import { WebRtcMesh } from '../net/WebRtcMesh';

export class MenuScene extends Phaser.Scene {
  private status?: Phaser.GameObjects.Text;
  constructor(){super('MenuScene');}
  create():void{
    const g=this.add.graphics();g.fillGradientStyle(0x63caff,0x63caff,0xf8d8ff,0xf8d8ff,1).fillRect(0,0,1280,720);g.fillStyle(0xffef85,.95).fillCircle(1120,90,54);
    for(const[x,y,s]of[[75,125,1.1],[430,150,.85],[880,130,1]]as const){g.fillStyle(0xffffff,.82);g.fillCircle(x,y,26*s);g.fillCircle(x+34*s,y-12*s,36*s);g.fillCircle(x+72*s,y,28*s);}
    this.add.text(640,84,'DOODLE FIGHT',{fontFamily:'Arial Black',fontSize:'58px',color:'#fff',stroke:'#315171',strokeThickness:10}).setOrigin(.5);
    this.add.text(640,147,'Phaser + TypeScript + WebRTC + Vite PWA',{fontFamily:'Arial',fontSize:'22px',color:'#294c69'}).setOrigin(.5);
    this.add.image(640,245,'weapons').setScale(1.05).setAlpha(.98);
    this.addButton(640,390,'PRACTICE • 3 BOTS',0x6fca7b,()=>this.scene.start('LobbyScene',{role:'practice',playerName:this.playerName()}));
    this.addButton(640,474,'HOST 4-PLAYER ROOM',0xff64ad,()=>void this.hostRoom());
    this.addButton(640,558,'JOIN ROOM CODE',0x5ea8ff,()=>void this.joinRoom());
    this.status=this.add.text(640,628,`Signaling: ${SignalingClient.defaultUrl()}`,{fontFamily:'Arial',fontSize:'17px',color:'#294c69',align:'center',wordWrap:{width:900}}).setOrigin(.5);
    this.add.text(640,684,'Install from Chrome/Edge for a fullscreen Android or Windows PWA • Touch + keyboard/mouse supported',{fontFamily:'Arial',fontSize:'16px',color:'#36556c'}).setOrigin(.5);
  }
  private addButton(x:number,y:number,label:string,color:number,onClick:()=>void):void{const c=this.add.container(x,y),p=this.add.graphics();p.fillStyle(color,1).fillRoundedRect(-205,-31,410,62,26);p.lineStyle(4,0xffffff,.95).strokeRoundedRect(-205,-31,410,62,26);c.add([p,this.add.text(0,0,label,{fontFamily:'Arial Black',fontSize:'23px',color:'#fff'}).setOrigin(.5)]);c.setSize(410,62).setInteractive({useHandCursor:true});c.on('pointerover',()=>c.setScale(1.025));c.on('pointerout',()=>c.setScale(1));c.on('pointerup',onClick);}
  private playerName():string{const stored=localStorage.getItem('doodle-fight-name');if(stored)return stored;const generated=`Explorer ${Math.floor(100+Math.random()*900)}`;localStorage.setItem('doodle-fight-name',generated);return generated;}
  private async hostRoom():Promise<void>{const name=this.playerName(),session=WebRtcMesh.createHost(name);this.setStatus('Connecting to signaling service...');try{const roomCode=await session.host();this.scene.start('LobbyScene',{role:'host',session,roomCode,playerName:name});}catch(error){session.close();this.setStatus(error instanceof Error?error.message:'Could not host room.');}}
  private async joinRoom():Promise<void>{const roomCode=prompt('Enter the 5-character Doodle Fight room code:')?.trim().toUpperCase();if(!roomCode)return;const name=this.playerName(),session=WebRtcMesh.createPeer(name);this.setStatus(`Joining ${roomCode}...`);try{await session.join(roomCode);this.scene.start('LobbyScene',{role:'peer',session,roomCode,playerName:name});}catch(error){session.close();this.setStatus(error instanceof Error?error.message:'Could not join room.');}}
  private setStatus(text:string):void{this.status?.setText(text);}
}
