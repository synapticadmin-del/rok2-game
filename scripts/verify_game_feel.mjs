#!/usr/bin/env node
/**
 * verify_game_feel.mjs — P5-T6 structural verification
 *
 * Checks that animations + audio system are properly wired:
 * URok2AudioManager, Rok2BuildingActor animations, Rok2WorldRenderer reveal/march sounds,
 * Rok2Api music/SFX integration, and Rok2CityLayoutActor build animations.
 * This is a static/structural test — it does not require a running UE5 build.
 *
 * Usage: node scripts/verify_game_feel.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = __dirname;
// جذر المستودع = scripts/.. — الملفات الحقيقية في Source/Rok2/{Public,Private}
const REPO = join(__dirname, '..');
const PUB = join(REPO, 'game', 'client-unreal', 'Source', 'Rok2', 'Public');
const PRIV = join(REPO, 'game', 'client-unreal', 'Source', 'Rok2', 'Private');

let passed = 0;
let failed = 0;

function ok(name) {
  console.log(`  ✅ ${name}`);
  passed++;
}
function fail(name, detail = '') {
  console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`);
  failed++;
}
function check(name, condition, detail = '') {
  if (condition) ok(name);
  else fail(name, detail);
}

// ---------------------------------------------------------------------------
// 1. Verify URok2AudioManager class
// ---------------------------------------------------------------------------
console.log('\n[1] URok2AudioManager class');

const audioH = join(PUB, 'Rok2AudioManager.h');
const audioCpp = join(PRIV, 'Rok2AudioManager.cpp');

check('Rok2AudioManager.h exists', existsSync(audioH));
check('Rok2AudioManager.cpp exists', existsSync(audioCpp));

if (existsSync(audioH)) {
  const content = readFileSync(audioH, 'utf8');
  check('has ERok2AudioType enum', content.includes('ERok2AudioType'));
  check('has ERok2MusicState enum', content.includes('ERok2MusicState'));
  check('has Get singleton', content.includes('static URok2AudioManager* Get()'));
  check('has InitForCiv method', content.includes('InitForCiv'));
  check('has PlayMusic method', content.includes('PlayMusic'));
  check('has StopMusic method', content.includes('StopMusic'));
  check('has PlaySfx method', content.includes('PlaySfx'));
  check('has PlaySfxWithVolume method', content.includes('PlaySfxWithVolume'));
  check('has IsMusicPlaying method', content.includes('IsMusicPlaying'));
  check('has OnMusicStateChanged delegate', content.includes('OnMusicStateChanged'));
  check('has MasterVolume property', content.includes('MasterVolume'));
  check('has bAudioEnabled property', content.includes('bAudioEnabled'));
  check('has MusicPaths map', content.includes('MusicPaths'));
  check('has SfxPaths map', content.includes('SfxPaths'));
}

if (existsSync(audioCpp)) {
  const content = readFileSync(audioCpp, 'utf8');
  check('implements InitForCiv', content.includes('void URok2AudioManager::InitForCiv'));
  check('implements PlayMusic', content.includes('void URok2AudioManager::PlayMusic'));
  check('implements StopMusic', content.includes('void URok2AudioManager::StopMusic'));
  check('implements PlaySfx', content.includes('void URok2AudioManager::PlaySfx'));
  check('implements BuildAudioPaths', content.includes('void URok2AudioManager::BuildAudioPaths'));
  check('has 6 civ music paths', content.includes('rome') && content.includes('china') && content.includes('arabia') && content.includes('egypt') && content.includes('vikings') && content.includes('japan'));
  check('has SFX paths for all types', content.includes('BuildComplete') && content.includes('Upgrade') && content.includes('BattleVictory') && content.includes('BattleDefeat') && content.includes('MarchStart'));
  check('uses UGameplayStatics::PlaySound2D', content.includes('PlaySound2D'));
  check('broadcasts OnMusicStateChanged', content.includes('OnMusicStateChanged.Broadcast'));
}

// ---------------------------------------------------------------------------
// 2. Verify Rok2BuildingActor animations
// ---------------------------------------------------------------------------
console.log('\n[2] Rok2BuildingActor animations');

const buildingH = join(PUB, 'Rok2BuildingActor.h');
check('Rok2BuildingActor.h exists', existsSync(buildingH));

if (existsSync(buildingH)) {
  const content = readFileSync(buildingH, 'utf8');
  check('has PlayBuildAnimation method', content.includes('PlayBuildAnimation'));
  check('has PlayUpgradeAnimation method', content.includes('PlayUpgradeAnimation'));
  check('has PlayRevealAnimation method', content.includes('PlayRevealAnimation'));
  check('has bIsAnimating property', content.includes('bIsAnimating'));
  check('has ActiveAnimType property', content.includes('ActiveAnimType'));
  check('has AnimTimer property', content.includes('AnimTimer'));
  check('has UpdateAnimation method', content.includes('UpdateAnimation'));
  check('has ComputeAnimatedScale method', content.includes('ComputeAnimatedScale'));
  check('has BuildAnimDuration constant', content.includes('BuildAnimDuration'));
  check('has UpgradeAnimDuration constant', content.includes('UpgradeAnimDuration'));
  check('has RevealAnimDuration constant', content.includes('RevealAnimDuration'));
}

const buildingCpp = join(PRIV, 'Rok2BuildingActor.cpp');
check('Rok2BuildingActor.cpp exists', existsSync(buildingCpp));

if (existsSync(buildingCpp)) {
  const content = readFileSync(buildingCpp, 'utf8');
  check('implements PlayBuildAnimation', content.includes('void ARok2BuildingActor::PlayBuildAnimation'));
  check('implements PlayUpgradeAnimation', content.includes('void ARok2BuildingActor::PlayUpgradeAnimation'));
  check('implements PlayRevealAnimation', content.includes('void ARok2BuildingActor::PlayRevealAnimation'));
  check('implements UpdateAnimation', content.includes('void ARok2BuildingActor::UpdateAnimation'));
  check('implements ComputeAnimatedScale', content.includes('FVector ARok2BuildingActor::ComputeAnimatedScale'));
  check('has Tick override', content.includes('void ARok2BuildingActor::Tick'));
  check('build anim scales from 0.1', content.includes('FMath::Lerp(0.1f, 1.0f'));
  check('upgrade anim uses sin pulse', content.includes('FMath::Sin'));
  check('reveal anim scales from 0.01', content.includes('FMath::Lerp(0.01f, 1.0f'));
}

// ---------------------------------------------------------------------------
// 3. Verify Rok2CityLayoutActor build animation call
// ---------------------------------------------------------------------------
console.log('\n[3] Rok2CityLayoutActor build animation');

const layoutCpp = join(PRIV, 'Rok2CityLayoutActor.cpp');
check('Rok2CityLayoutActor.cpp exists', existsSync(layoutCpp));

if (existsSync(layoutCpp)) {
  const content = readFileSync(layoutCpp, 'utf8');
  check('calls PlayBuildAnimation on spawn', content.includes('PlayBuildAnimation'));
}

// ---------------------------------------------------------------------------
// 4. Verify Rok2WorldRenderer audio + reveal
// ---------------------------------------------------------------------------
console.log('\n[4] Rok2WorldRenderer audio + reveal');

const worldCpp = join(PRIV, 'Rok2WorldRenderer.cpp');
check('Rok2WorldRenderer.cpp exists', existsSync(worldCpp));

if (existsSync(worldCpp)) {
  const content = readFileSync(worldCpp, 'utf8');
  check('includes Rok2AudioManager.h', content.includes('Rok2AudioManager.h'));
  check('calls PlaySfx for march start', content.includes('PlaySfx') && content.includes('MarchStart'));
  check('has reveal animation for cities', content.includes('PlayRevealAnimation'));
}

// ---------------------------------------------------------------------------
// 5. Verify Rok2Api audio integration
// ---------------------------------------------------------------------------
console.log('\n[5] Rok2Api audio integration');

const apiCpp = join(PRIV, 'Rok2Api.cpp');
check('Rok2Api.cpp exists', existsSync(apiCpp));

if (existsSync(apiCpp)) {
  const content = readFileSync(apiCpp, 'utf8');
  check('includes Rok2AudioManager.h', content.includes('Rok2AudioManager.h'));
  check('calls InitForCiv on InitCity', content.includes('InitForCiv'));
  check('calls PlayMusic on InitCity', content.includes('PlayMusic'));
  check('calls PlaySfx(Upgrade) on upgrade', content.includes('PlaySfx(ERok2AudioType::Upgrade)'));
  check('calls PlaySfx(BattleVictory/Defeat) on battle report', content.includes('BattleVictory') && content.includes('BattleDefeat'));
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P5-T6 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
