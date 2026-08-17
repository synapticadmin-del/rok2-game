// Copyright ROK2. Automated batch generator for game assets using Google Gemini / Imagen.
import fs from 'node:fs';
import path from 'node:path';

const OUTPUT_DIR = path.resolve('game/client-unreal/Content/Art/GeneratedAssets');

if (!fs.existsSync(OUTPUT_DIR)) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

console.log('=== ROK2 Asset Generation Pipeline ===');
console.log(`Target directory: ${OUTPUT_DIR}`);
console.log('Registry located at: 07-game-design/ASSET_GENERATION_REGISTRY.md');
console.log('Ready to generate via Google Generative AI when credentials are provided.');
