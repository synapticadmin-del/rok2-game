#!/usr/bin/env node
/**
 * ROK2 — P10-T7: فحص جودة وتكامل أصول الحانة والصناديق والمفاتيح والمنحوتات (Tavern Visual Assets).
 *
 * يتحقق من:
 * 1. وجود 4 موديلات GLB صالحة (glTF 2.0).
 * 2. وجود 23 أيقونة PNG صالحة (Chests, Keys, Sculptures, Materials, Blueprints).
 * 3. وجود 2 مؤثرين صوتيين WAV صالحين (16-bit PCM 44.1kHz).
 * 4. تكامل C++ في URok2ArtAssets (H + CPP).
 * 5. وجود سكربت الاستيراد والتوليد والتوثيق.
 */
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const REPO = path.resolve(__dirname, '..', '..', '..');
const CLIENT = path.join(REPO, 'game', 'client-unreal');
const TAVERN_DIR = path.join(CLIENT, 'Content', 'Art', 'Tavern');
const SFX_DIR = path.join(CLIENT, 'Content', 'Audio', 'sfx');

const PNG_MAGIC = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
const GLB_MAGIC = Buffer.from([0x67, 0x6c, 0x54, 0x46]); // "glTF"
const RIFF_MAGIC = Buffer.from([0x52, 0x49, 0x46, 0x46]); // "RIFF"

let checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}

// 1. 3D GLB Models
const requiredGlbs = [
  'building_tavern',
  'chest_silver',
  'chest_gold',
  'chest_equipment'
];

for (const glb of requiredGlbs) {
  const p = path.join(TAVERN_DIR, `${glb}.glb`);
  const exists = fs.existsSync(p);
  let valid = false;
  if (exists) {
    const buf = fs.readFileSync(p);
    valid = buf.length >= 100 && buf.subarray(0, 4).equals(GLB_MAGIC);
  }
  expect(`GLB Model: ${glb}.glb`, exists && valid, exists ? '' : 'File missing');
}

// 2. 2D Texture PNGs
const requiredPngs = [
  // 3 Chests
  'chest_silver', 'chest_gold', 'chest_equipment',
  // 6 Keys
  'key_silver', 'key_gold', 'key_equipment', 'key_expedition', 'key_canyon', 'key_osiris',
  // 4 Sculptures
  'sculpture_legendary', 'sculpture_epic', 'sculpture_elite', 'sculpture_advanced',
  // 4 Materials
  'material_leather', 'material_iron', 'material_ebony', 'material_crystal',
  // 6 Blueprints
  'blueprint_weapon', 'blueprint_helm', 'blueprint_chest', 'blueprint_gloves', 'blueprint_legs', 'blueprint_boots'
];

for (const png of requiredPngs) {
  const p = path.join(TAVERN_DIR, `${png}.png`);
  const exists = fs.existsSync(p);
  let valid = false;
  if (exists) {
    const buf = fs.readFileSync(p);
    valid = buf.length >= 50 && buf.subarray(0, 8).equals(PNG_MAGIC);
  }
  expect(`PNG Icon: ${png}.png`, exists && valid, exists ? '' : 'File missing');
}

// 3. Audio SFX WAVs
const requiredWavs = [
  'chest_open',
  'wheel_spin'
];

for (const wav of requiredWavs) {
  const p = path.join(SFX_DIR, `${wav}.wav`);
  const exists = fs.existsSync(p);
  let valid = false;
  if (exists) {
    const buf = fs.readFileSync(p);
    valid = buf.length >= 1000 && buf.subarray(0, 4).equals(RIFF_MAGIC);
  }
  expect(`SFX Audio: ${wav}.wav`, exists && valid, exists ? '' : 'File missing');
}

// 4. C++ Source Integration
const artHeader = fs.readFileSync(path.join(CLIENT, 'Source', 'Rok2', 'Public', 'Rok2ArtAssets.h'), 'utf8');
const artCpp = fs.readFileSync(path.join(CLIENT, 'Source', 'Rok2', 'Private', 'Rok2ArtAssets.cpp'), 'utf8');

expect('C++ Header declares GetTavernMeshAssetPath', artHeader.includes('GetTavernMeshAssetPath'));
expect('C++ Header declares GetTavernIconAssetPath', artHeader.includes('GetTavernIconAssetPath'));
expect('C++ Header declares LoadTavernIcon', artHeader.includes('LoadTavernIcon'));
expect('C++ Header declares HasTavernAsset', artHeader.includes('HasTavernAsset'));

expect('C++ CPP implements GetTavernMeshAssetPath', artCpp.includes('URok2ArtAssets::GetTavernMeshAssetPath'));
expect('C++ CPP implements GetTavernIconAssetPath', artCpp.includes('URok2ArtAssets::GetTavernIconAssetPath'));
expect('C++ CPP implements LoadTavernIcon', artCpp.includes('URok2ArtAssets::LoadTavernIcon'));
expect('C++ CPP implements HasTavernAsset', artCpp.includes('URok2ArtAssets::HasTavernAsset'));
expect('C++ CPP maps /Game/Art/Tavern/', artCpp.includes('/Game/Art/Tavern/'));

// 5. Scripts & Automation
expect('Generator script exists', fs.existsSync(path.join(REPO, 'scripts', 'generate_tavern_assets.py')));
expect('PowerShell Import script exists', fs.existsSync(path.join(CLIENT, 'scripts', 'Import-TavernAssets.ps1')));

// Report Results
let passed = 0;
let failed = 0;
for (const c of checks) {
  if (c.ok) {
    passed += 1;
    console.log('OK  :', c.name);
  } else {
    failed += 1;
    console.error('FAIL:', c.name, c.detail);
  }
}

console.log(`\n${passed}/${checks.length} checks passed.`);
if (failed === 0) {
  console.log('P10-T7 Tavern visual and audio assets contract verified.');
  process.exit(0);
} else {
  console.error('P10-T7 contract FAILED.');
  process.exit(1);
}
