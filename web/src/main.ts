import Phaser from 'phaser';
import './style.css';
import { BootScene } from './scenes/BootScene';
import { MenuScene } from './scenes/MenuScene';
import { ArenaScene } from './scenes/ArenaScene';

const config: Phaser.Types.Core.GameConfig = {
  type: Phaser.AUTO,
  parent: 'app',
  width: 1280,
  height: 720,
  backgroundColor: '#62c7ff',
  antialias: true,
  render: { pixelArt: false, roundPixels: false },
  scale: {
    mode: Phaser.Scale.FIT,
    autoCenter: Phaser.Scale.CENTER_BOTH,
    width: 1280,
    height: 720
  },
  input: {
    activePointers: 4
  },
  scene: [BootScene, MenuScene, ArenaScene]
};

new Phaser.Game(config);
