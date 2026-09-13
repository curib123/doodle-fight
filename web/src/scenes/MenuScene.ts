import Phaser from 'phaser';
import { WebRtcSession } from '../net/WebRtcSession';

export class MenuScene extends Phaser.Scene {
  constructor() { super('MenuScene'); }

  create(): void {
    const { width, height } = this.scale;
    const bg = this.add.graphics();
    bg.fillGradientStyle(0x62c7ff, 0x62c7ff, 0xf9d8ff, 0xf9d8ff, 1).fillRect(0, 0, width, height);
    this.drawCloud(bg, 130, 145, 1.15);
    this.drawCloud(bg, 980, 120, 0.95);

    this.add.sprite(210, 300, 'hero', 0).setScale(2.35).play('hero-idle');
    this.add.image(890, 530, 'weapons').setScale(0.42);

    this.add.text(735, 92, 'DOODLE FIGHT', {
      fontFamily: 'Arial Black', fontSize: '58px', color: '#ffffff', stroke: '#315578', strokeThickness: 10
    }).setOrigin(0.5);
    this.add.text(735, 151, 'Phaser • TypeScript • WebRTC • Installable PWA', {
      fontFamily: 'Arial', fontSize: '23px', color: '#26455f'
    }).setOrigin(0.5);

    this.add.text(735, 208,
      'Desktop: A/D • Space • Mouse • Click • Q/E • R\nTouch: left thumb move • JET button • right side aim + fire',
      { fontFamily: 'Arial', fontSize: '19px', color: '#31506a', align: 'center', lineSpacing: 8 }
    ).setOrigin(0.5);

    this.makeButton(735, 315, 'PRACTICE ARENA', 0xff5f9f, () => {
      this.scene.start('ArenaScene', { mode: 'practice' });
    });
    this.makeButton(735, 403, 'HOST WEBRTC', 0x805dff, () => void this.hostP2P());
    this.makeButton(735, 491, 'JOIN WEBRTC', 0x24c6d8, () => void this.joinP2P());

    this.add.text(735, 595,
      'P2P uses a direct WebRTC DataChannel. Offer/answer codes are exchanged manually,\nso there is no dedicated gameplay or signaling server required for this prototype.',
      { fontFamily: 'Arial', fontSize: '17px', color: '#37536a', align: 'center', lineSpacing: 6 }
    ).setOrigin(0.5);
  }

  private drawCloud(g: Phaser.GameObjects.Graphics, x: number, y: number, scale: number): void {
    g.fillStyle(0xffffff, 0.78);
    g.fillCircle(x, y, 28 * scale);
    g.fillCircle(x + 34 * scale, y - 12 * scale, 39 * scale);
    g.fillCircle(x + 76 * scale, y, 29 * scale);
  }

  private makeButton(x: number, y: number, label: string, color: number, action: () => void): void {
    const container = this.add.container(x, y);
    const plate = this.add.graphics();
    plate.fillStyle(color, 1).fillRoundedRect(-185, -34, 370, 68, 28);
    plate.lineStyle(5, 0xffffff, 0.95).strokeRoundedRect(-185, -34, 370, 68, 28);
    container.add([plate, this.add.text(0, 0, label, { fontFamily: 'Arial Black', fontSize: '25px', color: '#ffffff' }).setOrigin(0.5)]);
    container.setSize(370, 68).setInteractive({ useHandCursor: true });
    container.on('pointerover', () => container.setScale(1.035));
    container.on('pointerout', () => container.setScale(1));
    container.on('pointerup', action);
  }

  private async hostP2P(): Promise<void> {
    try {
      const session = new WebRtcSession('host');
      const offer = await session.createOfferCode();
      window.prompt('HOST OFFER — copy this entire code and send it to the joining player.', offer);
      const answer = window.prompt('Paste the JOIN ANSWER code here:');
      if (!answer) { session.close(); return; }
      await session.acceptAnswerCode(answer);
      this.scene.start('ArenaScene', { mode: 'host', session });
    } catch (error) {
      window.alert(`Could not create WebRTC host: ${String(error)}`);
    }
  }

  private async joinP2P(): Promise<void> {
    try {
      const offer = window.prompt('Paste the HOST OFFER code here:');
      if (!offer) return;
      const session = new WebRtcSession('peer');
      const answer = await session.acceptOfferCode(offer);
      window.prompt('JOIN ANSWER — copy this entire code and send it back to the host.', answer);
      this.scene.start('ArenaScene', { mode: 'peer', session });
    } catch (error) {
      window.alert(`Could not join WebRTC match: ${String(error)}`);
    }
  }
}
