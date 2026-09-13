import { defineConfig } from 'vite';
import { VitePWA } from 'vite-plugin-pwa';

export default defineConfig({
  plugins: [
    VitePWA({
      registerType: 'autoUpdate',
      includeAssets: ['icon.svg'],
      manifest: {
        name: 'Doodle Fight',
        short_name: 'DoodleFight',
        description: 'Cute colorful WebRTC jetpack arena shooter.',
        theme_color: '#62c7ff',
        background_color: '#62c7ff',
        display: 'standalone',
        orientation: 'landscape',
        start_url: './',
        icons: [
          { src: 'icon.svg', sizes: 'any', type: 'image/svg+xml', purpose: 'any maskable' }
        ]
      },
      workbox: {
        globPatterns: ['**/*.{js,css,html,webp,png,svg,woff2}']
      }
    })
  ],
  build: {
    target: 'es2020'
  }
});
