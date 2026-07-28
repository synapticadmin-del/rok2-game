#!/usr/bin/env node
/**
 * verify_fog_of_war.mjs — P5-T5 structural verification
 *
 * Checks that the fog of war + scout system is properly wired:
 * URok2FogOfWar class, Rok2WorldRenderer fog integration, Rok2Api scout support,
 * Rok2Types scout entity, and Rok2MarchPanel scout button.
 * This is a static/structural test — it does not require a running UE5 build.
 *
 * Usage: node scripts/verify_fog_of_war.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = __dirname;

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
// 1. Verify URok2FogOfWar class
// ---------------------------------------------------------------------------
console.log('\n[1] URok2FogOfWar class');

const fogH = join(ROOT, 'Rok2FogOfWar.h');
const fogCpp = join(ROOT, 'Rok2FogOfWar.cpp');

check('Rok2FogOfWar.h exists', existsSync(fogH));
check('Rok2FogOfWar.cpp exists', existsSync(fogCpp));

if (existsSync(fogH)) {
  const content = readFileSync(fogH, 'utf8');
  check('has ERok2FogState enum', content.includes('ERok2FogState'));
  check('has FRok2Scout struct', content.includes('FRok2Scout'));
  check('has Get singleton', content.includes('static URok2FogOfWar* Get()'));
  check('has Init method', content.includes('void Init('));
  check('has RevealArea method', content.includes('RevealArea'));
  check('has GetFogStateAt method', content.includes('GetFogStateAt'));
  check('has IsExplored method', content.includes('IsExplored'));
  check('has AddScout method', content.includes('AddScout'));
  check('has UpdateScouts method', content.includes('UpdateScouts'));
  check('has OnFogUpdated delegate', content.includes('OnFogUpdated'));
  check('has OnScoutArrived delegate', content.includes('OnScoutArrived'));
  check('has CityRevealRadius property', content.includes('CityRevealRadius'));
  check('has ScoutRevealRadius property', content.includes('ScoutRevealRadius'));
}

if (existsSync(fogCpp)) {
  const content = readFileSync(fogCpp, 'utf8');
  check('implements Init', content.includes('void URok2FogOfWar::Init'));
  check('implements RevealArea', content.includes('void URok2FogOfWar::RevealArea'));
  check('implements GetFogStateAt', content.includes('ERok2FogState URok2FogOfWar::GetFogStateAt'));
  check('implements IsExplored', content.includes('bool URok2FogOfWar::IsExplored'));
  check('implements AddScout', content.includes('void URok2FogOfWar::AddScout'));
  check('implements UpdateScouts', content.includes('void URok2FogOfWar::UpdateScouts'));
  check('implements RemoveScout', content.includes('void URok2FogOfWar::RemoveScout'));
  check('has WorldToGridIndex', content.includes('WorldToGridIndex'));
  check('has IsValidCell', content.includes('IsValidCell'));
  check('has RevealCell', content.includes('RevealCell'));
  check('broadcasts OnFogUpdated', content.includes('OnFogUpdated.Broadcast'));
  check('broadcasts OnScoutArrived', content.includes('OnScoutArrived.Broadcast'));
}

// ---------------------------------------------------------------------------
// 2. Verify Rok2Types.h has scout entity
// ---------------------------------------------------------------------------
console.log('\n[2] Rok2Types.h scout entity');

const typesH = join(ROOT, 'Rok2Types.h');
check('Rok2Types.h exists', existsSync(typesH));

if (existsSync(typesH)) {
  const content = readFileSync(typesH, 'utf8');
  check('has FRok2ScoutEntity struct', content.includes('FRok2ScoutEntity'));
  check('FRok2ScoutEntity has OwnerPlayerId', content.includes('OwnerPlayerId'));
  check('FRok2ScoutEntity has FromX/FromY', content.includes('FromX') && content.includes('FromY'));
  check('FRok2ScoutEntity has ToX/ToY', content.includes('ToX') && content.includes('ToY'));
  check('FRok2ScoutEntity has StartMs/EtaMs', content.includes('StartMs') && content.includes('EtaMs'));
  check('FRok2ScoutEntity has State', content.includes('State'));
  check('FRok2WorldSnapshot has Scouts array', content.includes('TArray<FRok2ScoutEntity> Scouts'));
}

// ---------------------------------------------------------------------------
// 3. Verify Rok2Api.h has scout support
// ---------------------------------------------------------------------------
console.log('\n[3] Rok2Api scout support');

const apiH = join(ROOT, 'Rok2Api.h');
check('Rok2Api.h exists', existsSync(apiH));

if (existsSync(apiH)) {
  const content = readFileSync(apiH, 'utf8');
  check('has SendScout method', content.includes('SendScout'));
  check('has ParseScoutEntity method', content.includes('ParseScoutEntity'));
}

const apiCpp = join(ROOT, 'Rok2Api.cpp');
check('Rok2Api.cpp exists', existsSync(apiCpp));

if (existsSync(apiCpp)) {
  const content = readFileSync(apiCpp, 'utf8');
  check('implements SendScout', content.includes('void URok2Api::SendScout'));
  check('implements ParseScoutEntity', content.includes('void URok2Api::ParseScoutEntity'));
  check('ParseWorld parses scouts', content.includes('World.Scouts.Empty()') && content.includes('scouts'));
  check('SendScout posts to /v1/world/scout', content.includes('/v1/world/scout'));
  check('has scout_arrived WS event', content.includes('scout_arrived'));
}

// ---------------------------------------------------------------------------
// 4. Verify Rok2WorldRenderer fog integration
// ---------------------------------------------------------------------------
console.log('\n[4] Rok2WorldRenderer fog integration');

const worldCpp = join(ROOT, 'Rok2WorldRenderer.cpp');
check('Rok2WorldRenderer.cpp exists', existsSync(worldCpp));

if (existsSync(worldCpp)) {
  const content = readFileSync(worldCpp, 'utf8');
  check('includes Rok2FogOfWar.h', content.includes('Rok2FogOfWar.h'));
  check('uses URok2FogOfWar::Get()', content.includes('URok2FogOfWar::Get()'));
  check('calls RevealArea for player city', content.includes('RevealArea'));
  check('checks IsExplored for cities', content.includes('IsExplored') && content.includes('Cities'));
  check('checks IsExplored for passes', content.includes('IsExplored') && content.includes('Passes'));
  check('checks IsExplored for nodes', content.includes('IsExplored') && content.includes('Nodes'));
  check('calls UpdateScouts in Tick', content.includes('UpdateScouts'));
}

// ---------------------------------------------------------------------------
// 5. Verify Rok2MarchPanel scout button
// ---------------------------------------------------------------------------
console.log('\n[5] Rok2MarchPanel scout button');

const marchH = join(ROOT, 'Rok2MarchPanel.h');
check('Rok2MarchPanel.h exists', existsSync(marchH));

if (existsSync(marchH)) {
  const content = readFileSync(marchH, 'utf8');
  check('has OnScoutClicked method', content.includes('OnScoutClicked'));
}

const marchCpp = join(ROOT, 'Rok2MarchPanel.cpp');
check('Rok2MarchPanel.cpp exists', existsSync(marchCpp));

if (existsSync(marchCpp)) {
  const content = readFileSync(marchCpp, 'utf8');
  check('has ScoutButton', content.includes('ScoutButton'));
  check('calls SendScout', content.includes('SendScout'));
  check('has scout button text', content.includes('🔭') || content.includes('كشافة'));
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P5-T5 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
