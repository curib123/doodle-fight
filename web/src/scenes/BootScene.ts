import Phaser from 'phaser';
import { HERO_SHEET_DATA_URI } from '../heroAsset';
import { EFFECTS_SHEET_DATA_URI } from '../effectsAsset';
import { WEAPONS_SHEET_DATA_URI } from '../weaponsAsset';

export class BootScene extends Phaser.Scene {
  constructor() { super('BootScene'); }

  preload(): void {
    this.load.spritesheet('hero', HERO_SHEET_DATA_URI, {
      frameWidth: 64,
      frameHeight: 96
    });
    this.load.spritesheet('effects', EFFECTS_SHEET_DATA_URI, {
      frameWidth: 96,
      frameHeight: 96
    });
    this.load.image('weapons', WEAPONS_SHEET_DATA_URI);

    const { width, height } = this.scale;
    this.add.text(width / 2, height / 2 - 42, 'Loading Doodle Fight', {
      fontFamily: 'Arial Black', fontSize: '30px', color: '#24496e'
    }).setOrigin(0.5);
    const bar = this.add.graphics();
    this.load.on('progress', (progress: number) => {
      bar.clear();
      bar.fillStyle(0xffffff, 0.35).fillRoundedRect(width * 0.27, height / 2 + 10, width * 0.46, 20, 10);
      bar.fillStyle(0xff66ad, 1).fillRoundedRect(width * 0.27, height / 2 + 10, width * 0.46 * progress, 20, 10);
    });
  }

  create(): void {
    const createAnim = (key: string, start: number, end: number, frameRate: number, repeat: number) => {
      if (this.anims.exists(key)) return;
      this.anims.create({ key, frames: this.anims.generateFrameNumbers('hero', { start, end }), frameRate, repeat });
    };
    createAnim('hero-idle', 0, 7, 9, -1);
    createAnim('hero-run', 8, 15, 15, -1);
    createAnim('hero-fly', 16, 23, 13, -1);
    createAnim('hero-shoot', 24, 27, 16, 0);
    createAnim('hero-ko', 28, 31, 9, 0);

    const effectAnim = (key: string, start: number, end: number, frameRate: number) => {
      if (this.anims.exists(key)) return;
      this.anims.create({ key, frames: this.anims.generateFrameNumbers('effects', { start, end }), frameRate, repeat: 0 });
    };
    effectAnim('fx-muzzle', 0, 3, 22);
    effectAnim('fx-impact', 4, 7, 22);
    effectAnim('fx-jet', 8, 11, 18);
    effectAnim('fx-respawn', 12, 15, 18);

    this.scene.start('MenuScene');
  }
}
