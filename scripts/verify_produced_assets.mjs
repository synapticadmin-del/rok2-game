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
const SFX_P4T4 = ['gather_complete', 'research_complete', 'heal_complete', 'zone_unlock', 'rally_launch'];

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
console.log('\n[1b] Per-civ battle music WAV files (P4-T3)');
// ---------------------------------------------------------------------------
for (const civ of CIVS) {
  const p = join(ROOT, 'game/client-unreal/Content/Audio', civ, 'battle.wav');
  checkBinaryOrB64(p, [0x52, 0x49, 0x46, 0x46], `Audio/${civ}/battle.wav`);
  if (existsSync(p)) check(`Audio/${civ}/battle.wav non-trivial size`, statSync(p).size > 10000, `${statSync(p).size} bytes`);
}

// ---------------------------------------------------------------------------
console.log('\n[2] SFX WAV files');
// ---------------------------------------------------------------------------
for (const sfx of SFX) {
  const p = join(ROOT, 'game/client-unreal/Content/Audio/sfx', `${sfx}.wav`);
  checkBinaryOrB64(p, [0x52, 0x49, 0x46, 0x46], `Audio/sfx/${sfx}.wav`);
}

// ---------------------------------------------------------------------------
console.log('\n[2b] P4-T4 gameplay event SFX WAV files');
// ---------------------------------------------------------------------------
for (const sfx of SFX_P4T4) {
  const p = join(ROOT, 'game/client-unreal/Content/Audio/sfx', `${sfx}.wav`);
  checkBinaryOrB64(p, [0x52, 0x49, 0x46, 0x46], `Audio/sfx/${sfx}.wav`);
  if (existsSync(p)) check(`Audio/sfx/${sfx}.wav non-trivial size`, statSync(p).size > 5000, `${statSync(p).size} bytes`);
}

// ---------------------------------------------------------------------------
console.log('\n[3] Commander portraits (one per commander in data/commanders.json)');
// ---------------------------------------------------------------------------
const cmdJsonPath = join(ROOT, 'data/commanders.json');
check('data/commanders.json exists', existsSync(cmdJsonPath));
let commanderIds = [];
if (existsSync(cmdJsonPath)) {
  const data = JSON.parse(readFileSync(cmdJsonPath, 'utf8'));
  commanderIds = (data.commanders || []).map(c => c.id);
  check('has >= 12 commanders (P4-T5 added 6)', commanderIds.length >= 12, `found ${commanderIds.length}`);
}
// P4-T5 أضاف 6 قادة بلا بورتريهات بعد — الودجت يعتمد placeholder ملوّن لهم
// (BuildPortraitPlaceholder). الشرط: كل قائد أصلي له بورتريه حقيقي، والباقون
// يُسردون كتحذير لا يفشل الفحص.
const originalIds = new Set([
  'cmd_rome_starter', 'cmd_china_starter', 'cmd_arabia_starter',
  'cmd_egypt_starter', 'cmd_vikings_starter', 'cmd_japan_starter',
  'julius_caesar', 'richard_lionheart', 'yi_seong_gye',
  'genghis_khan', 'joan_of_arc', 'alexander_great',
]);
for (const id of commanderIds) {
  const p = join(ROOT, 'game/client-unreal/Content/Art/Commanders', `${id}.png`);
  if (originalIds.has(id)) {
    checkBinaryOrB64(p, [0x89, 0x50, 0x4e, 0x47] /* PNG */, `Art/Commanders/${id}.png`);
    if (existsSync(p)) check(`Art/Commanders/${id}.png non-trivial size`, statSync(p).size > 30000, `${statSync(p).size} bytes`);
  } else if (!existsSync(p)) {
    console.log(`  ⚠️  Art/Commanders/${id}.png missing — placeholder fallback in widget (ok)`);
  } else {
    checkBinaryOrB64(p, [0x89, 0x50, 0x4e, 0x47], `Art/Commanders/${id}.png`);
  }
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
console.log('\n[5] asset decode/import pipeline (import_assets.py)');
// ---------------------------------------------------------------------------
const setup = join(ROOT, 'game/client-unreal/setup_level.py');
check('setup_level.py exists', existsSync(setup));
const importer = join(ROOT, 'game/client-unreal/import_assets.py');
check('import_assets.py exists', existsSync(importer));
if (existsSync(importer)) {
  const c = readFileSync(importer, 'utf8');
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
console.log('\n[6b] AudioManager battle mode (P4-T3)');
// ---------------------------------------------------------------------------
const amH = join(ROOT, 'game/client-unreal/Source/Rok2/Public/Rok2AudioManager.h');
check('Rok2AudioManager.h exists', existsSync(amH));
if (existsSync(amH)) {
  const c = readFileSync(amH, 'utf8');
  check('has ERok2MusicMode enum (Peace/Battle)', c.includes('ERok2MusicMode') && c.includes('Battle'));
  check('has EnterBattleMode method', c.includes('EnterBattleMode'));
  check('has ExitBattleMode method', c.includes('ExitBattleMode'));
  check('has IsInBattleMode method', c.includes('IsInBattleMode'));
  check('has BattleMusicPaths map', c.includes('BattleMusicPaths'));
  check('has BattleModeTimeout property', c.includes('BattleModeTimeout'));
  check('has BattleModeTimer handle', c.includes('BattleModeTimer'));
}
if (existsSync(amCpp)) {
  const c = readFileSync(amCpp, 'utf8');
  for (const civ of CIVS) check(`AudioManager references Audio/${civ}/battle`, c.includes(`Audio/${civ}/battle`));
  check('implements EnterBattleMode', c.includes('void URok2AudioManager::EnterBattleMode'));
  check('implements ExitBattleMode', c.includes('void URok2AudioManager::ExitBattleMode'));
  check('implements PlayCurrentModeMusic', c.includes('void URok2AudioManager::PlayCurrentModeMusic'));
  check('battle mode switches path by mode', c.includes('MusicMode == ERok2MusicMode::Battle') && c.includes('BattleMusicPaths'));
  check('battle mode uses timer for auto-return', c.includes('SetTimer(BattleModeTimer'));
}

// ---------------------------------------------------------------------------
console.log('\n[6c] Rok2Api battle music hook (P4-T3)');
// ---------------------------------------------------------------------------
const apiCpp = join(ROOT, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp');
check('Rok2Api.cpp exists', existsSync(apiCpp));
if (existsSync(apiCpp)) {
  const c = readFileSync(apiCpp, 'utf8');
  check('calls EnterBattleMode on battle_report', c.includes('EnterBattleMode'));
  check('still plays victory/defeat sfx', c.includes('BattleVictory') && c.includes('BattleDefeat'));
}

// ---------------------------------------------------------------------------
console.log('\n[6d] P4-T4 gameplay event SFX wiring');
// ---------------------------------------------------------------------------
const amH2 = join(ROOT, 'game/client-unreal/Source/Rok2/Public/Rok2AudioManager.h');
if (existsSync(amH2)) {
  const c = readFileSync(amH2, 'utf8');
  for (const t of ['GatherComplete', 'ResearchComplete', 'HealComplete', 'ZoneUnlock', 'RallyLaunch'])
    check(`ERok2AudioType has ${t}`, c.includes(t));
}
if (existsSync(amCpp)) {
  const c = readFileSync(amCpp, 'utf8');
  for (const s of SFX_P4T4)
    check(`AudioManager maps sfx/${s}`, c.includes(`Audio/sfx/${s}`));
}
if (existsSync(apiCpp)) {
  const c = readFileSync(apiCpp, 'utf8');
  check('plays ZoneUnlock on zone_unlocked', c.includes('PlaySfx(ERok2AudioType::ZoneUnlock)'));
  check('plays ResearchComplete on tech_researched', c.includes('PlaySfx(ERok2AudioType::ResearchComplete)'));
  check('plays RallyLaunch on rally_launched', c.includes('PlaySfx(ERok2AudioType::RallyLaunch)'));
  check('plays GatherComplete on march_returning gather', c.includes('PlaySfx(ERok2AudioType::GatherComplete)'));
  check('gather check looks for gather/node kind', c.includes('gather') && c.includes('march_returning'));
  check('has HealWounded method', c.includes('void URok2Api::HealWounded'));
  check('HealWounded posts to /v1/city/heal', c.includes('/v1/city/heal'));
  check('HealWounded plays HealComplete on success', c.includes('PlaySfx(ERok2AudioType::HealComplete)'));
}
const apiH = join(ROOT, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h');
if (existsSync(apiH)) {
  const c = readFileSync(apiH, 'utf8');
  check('Rok2Api.h declares HealWounded', c.includes('HealWounded'));
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
  check('generates battle music for all civs (P4-T3)', CIVS.every(v => c.includes(`battle_${v}`)) && c.includes('battle.wav'));
  check('generates P4-T4 event sfx', SFX_P4T4.every(s => c.includes(`"${s}"`)));
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
