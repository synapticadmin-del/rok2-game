#!/usr/bin/env node
/**
 * verify_produced_assets.mjs — P4-T2 structural verification
 *
 * Checks that real audio assets (per-civ music + SFX) and commander portraits
 * are present, binary-valid, and wired into the client code:
 *   Content/Audio/<civ>/music.wav  (6 civs)  + Content/Audio/sfx/*.wav (7)
 *   Content/Art/Commanders/<id>.png (12 commanders from data/commanders.json)
 *   Rok2CommanderWidget real-portrait loading with placeholder fallback
 *   setup_level.py WAV/PNG decode + import step
 *
 * Note: binary assets may be stored base64-encoded in git (P2-T7 convention).
 * This verifier accepts either raw binary (magic bytes) or valid base64 of it.
 *
 * Usage: node scripts/verify_produced_assets.mjs   (from repo root)
 */

import { readFileSync, existsSync, statSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const ROOT = join(fileURLToPath(new URL('.', import.meta.url)), '..');

let passed = 0;
let failed = 0;

function ok(name) { console.log(`  ✅ ${name}`); passed++; }
function fail(name, detail = '') { console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); failed++; }
function check(name, condition, detail = '') { condition ? ok(name) : fail(name, detail); }

const CIVS = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];
const SFX = ['build_complete', 'upgrade', 'victory', 'defeat', 'march_start', 'button_click', 'notification'];

function checkBinaryOrB64(path, magic, label) {
  if (!existsSync(path)) { fail(`${label} exists`, path); return; }
  const buf = readFileSync(path);
  if (buf.subarray(0, magic.length).equals(Buffer.from(magic))) { ok(`${label} is raw binary`); return; }
  try {
    const dec = Buffer.from(buf.toString('utf8').trim(), 'base64');
    check(`${label} is valid base64 of binary`, dec.subarray(0, magic.length).equals(Buffer.from(magic)));
  } catch { fail(`${label} binary/base64 validity`); }
}

// ---------------------------------------------------------------------------
console.log('\n[1] Per-civ music WAV files');
// ---------------------------------------------------------------------------
for (const civ of CIVS) {
  const p = join(ROOT, 'game/client-unreal/Content/Audio', civ, 'music.wav');
  checkBinaryOrB64(p, [0x52, 0x49, 0x46, 0x46] /* RIFF */, `Audio/${civ}/music.wav`);
  if (existsSync(p)) check(`Audio/${civ}/music.wav non-trivial size`, statSync(p).size > 10000, `${statSync(p).size} bytes`);
}

// ---------------------------------------------------------------------------
console.log('\n[2] SFX WAV files');
// ---------------------------------------------------------------------------
for (const sfx of SFX) {
  const p = join(ROOT, 'game/client-unreal/Content/Audio/sfx', `${sfx}.wav`);
  checkBinaryOrB64(p, [0x52, 0x49, 0x46, 0x46], `Audio/sfx/${sfx}.wav`);
}

// ---------------------------------------------------------------------------
console.log('\n[3] Commander portraits (12 from data/commanders.json)');
// ---------------------------------------------------------------------------
const cmdJsonPath = join(ROOT, 'data/commanders.json');
check('data/commanders.json exists', existsSync(cmdJsonPath));
let commanderIds = [];
if (existsSync(cmdJsonPath)) {
  const data = JSON.parse(readFileSync(cmdJsonPath, 'utf8'));
  commanderIds = (data.commanders || []).map(c => c.id);
  check('has exactly 12 commanders', commanderIds.length === 12, `found ${commanderIds.length}`);
}
for (const id of commanderIds) {
  const p = join(ROOT, 'game/client-unreal/Content/Art/Commanders', `${id}.png`);
  checkBinaryOrB64(p, [0x89, 0x50, 0x4e, 0x47] /* PNG */, `Art/Commanders/${id}.png`);
  if (existsSync(p)) check(`Art/Commanders/${id}.png non-trivial size`, statSync(p).size > 30000, `${statSync(p).size} bytes`);
}

// ---------------------------------------------------------------------------
console.log('\n[4] Rok2CommanderWidget real-portrait wiring');
// ---------------------------------------------------------------------------
const wCpp = join(ROOT, 'game/client-unreal/Source/Rok2/Private/Rok2CommanderWidget.cpp');
check('Rok2CommanderWidget.cpp exists', existsSync(wCpp));
if (existsSync(wCpp)) {
  const c = readFileSync(wCpp, 'utf8');
  check('has LoadCommanderPortrait helper', c.includes('LoadCommanderPortrait'));
  check('loads from /Game/Art/Commanders/', c.includes('/Game/Art/Commanders/'));
  check('includes Engine/Texture2D.h', c.includes('Engine/Texture2D.h'));
  check('card uses real portrait with fallback', c.includes('MakePortraitImage') && c.includes('BuildPortraitPlaceholder'));
  check('detail panel sets portrait texture', c.includes('DetailPortraitImage->SetBrushFromTexture'));
  check('keeps placeholder fallback (no breakage)', c.includes('placeholder stays'));
}

// ---------------------------------------------------------------------------
console.log('\n[5] setup_level.py WAV/PNG import step');
// ---------------------------------------------------------------------------
const setup = join(ROOT, 'game/client-unreal/setup_level.py');
check('setup_level.py exists', existsSync(setup));
if (existsSync(setup)) {
  const c = readFileSync(setup, 'utf8');
  check('has P4-T2 import step', c.includes('P4-T2'));
  check('decodes WAV (RIFF magic)', c.includes('b"RIFF"'));
  check('decodes PNG (magic)', c.includes('b"\\x89PNG"'));
  check('imports Audio tree to /Game/Audio', c.includes('/Game/Audio'));
  check('imports Commanders to /Game/Art/Commanders', c.includes('/Game/Art/Commanders'));
}

// ---------------------------------------------------------------------------
console.log('\n[6] AudioManager music path consistency (paths ↔ files)');
// ---------------------------------------------------------------------------
const amCpp = join(ROOT, 'game/client-unreal/Source/Rok2/Private/Rok2AudioManager.cpp');
if (existsSync(amCpp)) {
  const c = readFileSync(amCpp, 'utf8');
  for (const civ of CIVS) check(`AudioManager references Audio/${civ}/music`, c.includes(`Audio/${civ}/music`));
  for (const sfx of ['build_complete', 'upgrade', 'victory', 'defeat', 'march_start', 'button_click', 'notification'])
    check(`AudioManager references sfx/${sfx}`, c.includes(`Audio/sfx/${sfx}`));
}

// ---------------------------------------------------------------------------
console.log('\n[7] Generation script reproducibility');
// ---------------------------------------------------------------------------
const gen = join(ROOT, 'scripts/generate_audio.py');
check('scripts/generate_audio.py exists', existsSync(gen));
if (existsSync(gen)) {
  const c = readFileSync(gen, 'utf8');
  check('generates all 6 civs', CIVS.every(v => c.includes(`"${v}"`)));
  check('generates all 7 sfx', SFX.every(s => c.includes(s)));
  check('writes 16-bit PCM WAV', c.includes('setsampwidth(2)'));
}
const decode = join(ROOT, 'scripts/decode_binary_assets.py');
check('scripts/decode_binary_assets.py exists', existsSync(decode));
if (existsSync(decode)) {
  const c = readFileSync(decode, 'utf8');
  check('decoder covers WAV+PNG+GLB', c.includes('.wav') && c.includes('.png') && c.includes('.glb'));
  check('decoder scans Audio + Commanders dirs', c.includes('Audio') && c.includes('Commanders'));
}

// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P4-T2 structural verification: ${passed} passed, ${failed} failed`);
if (failed > 0) { console.log('\n❌ FAILED'); process.exit(1); }
console.log('\n✅ ALL PASSED');
process.exit(0);
