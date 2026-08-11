#!/usr/bin/env node
/**
 * verify_civ_themes.mjs — P5-T2 structural verification
 *
 * Checks that the six civilization themes are properly defined and wired
 * through the client code. This is a static/structural test — it does not
 * require a running UE5 build or the live backend.
 *
 * Usage: node scripts/verify_civ_themes.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
// جذر المستودع = scripts/.. — الملفات الحقيقية في data/ وSource/Rok2/{Public,Private}
const REPO = join(__dirname, '..');
const DATA = join(REPO, 'data');
const SRC = join(REPO, 'game', 'client-unreal', 'Source', 'Rok2');
const PUB = join(SRC, 'Public');
const PRIV = join(SRC, 'Private');

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
// 1. Verify data/civilizations.json exists and has 6 civs with theme palettes
// ---------------------------------------------------------------------------
console.log('\n[1] data/civilizations.json');

// نتحقق من الملفات في مواقعها الحقيقية داخل الشجرة
const civJsonPath = join(DATA, 'civilizations.json');
check('civilizations.json exists (repo data/)', existsSync(civJsonPath));

if (existsSync(civJsonPath)) {
  const civs = JSON.parse(readFileSync(civJsonPath, 'utf8')).civilizations;
  check('6 civilizations present', civs.length === 6, `got ${civs.length}`);

  const expectedIds = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];
  for (const id of expectedIds) {
    const c = civs.find((x) => x.id === id);
    check(`civ "${id}" exists`, !!c);
    if (c) {
      check(`  has theme.primary hex`, /^#[0-9A-Fa-f]{6}$/.test(c.theme?.primary ?? ''));
      check(`  has theme.secondary hex`, /^#[0-9A-Fa-f]{6}$/.test(c.theme?.secondary ?? ''));
      check(`  has theme.architecture`, typeof c.theme?.architecture === 'string');
    }
  }
}

// ---------------------------------------------------------------------------
// 2. Verify URok2CivThemes files exist and contain the 6 civ IDs
// ---------------------------------------------------------------------------
console.log('\n[2] Rok2CivThemes class');

const civThemesH = join(PUB, 'Rok2CivThemes.h');
const civThemesCpp = join(PRIV, 'Rok2CivThemes.cpp');

check('Rok2CivThemes.h exists', existsSync(civThemesH));
check('Rok2CivThemes.cpp exists', existsSync(civThemesCpp));

if (existsSync(civThemesH)) {
  const content = readFileSync(civThemesH, 'utf8');
  check('has ERok2ArchStyle enum', content.includes('ERok2ArchStyle'));
  check('has FRok2CivTheme struct', content.includes('FRok2CivTheme'));
  check('has GetTheme method', content.includes('GetTheme'));
  check('has Get singleton', content.includes('static URok2CivThemes* Get()'));
}

if (existsSync(civThemesCpp)) {
  const content = readFileSync(civThemesCpp, 'utf8');
  const civIds = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];
  for (const id of civIds) {
    check(`civ "${id}" in BuildDefaults`, content.includes(`TEXT("${id}")`));
  }
  check('has ArchStyleFromString', content.includes('ArchStyleFromString'));
  check('has ParseThemeFromJson', content.includes('ParseThemeFromJson'));
  check('reads civilizations.json from disk', content.includes('civilizations.json'));
}

// ---------------------------------------------------------------------------
// 3. Verify Rok2BuildingActor is theme-aware
// ---------------------------------------------------------------------------
console.log('\n[3] Rok2BuildingActor theme integration');

const buildingH = join(PUB, 'Rok2BuildingActor.h');
const buildingCpp = join(PRIV, 'Rok2BuildingActor.cpp');

check('Rok2BuildingActor.h exists', existsSync(buildingH));
check('Rok2BuildingActor.cpp exists', existsSync(buildingCpp));

if (existsSync(buildingH)) {
  const content = readFileSync(buildingH, 'utf8');
  check('has CivId property', content.includes('FString CivId'));
  check('has SetupWithCiv method', content.includes('SetupWithCiv'));
  check('has RoofMesh component', content.includes('RoofMesh'));
  check('has TrimMesh component', content.includes('TrimMesh'));
  check('has AccentMesh component', content.includes('AccentMesh'));
  check('has ApplyCivTheme method', content.includes('ApplyCivTheme'));
}

if (existsSync(buildingCpp)) {
  const content = readFileSync(buildingCpp, 'utf8');
  check('includes Rok2CivThemes.h', content.includes('Rok2CivThemes.h'));
  check('calls ApplyCivTheme in SetupWithCiv', content.includes('ApplyCivTheme()'));
  check('has ApplyArchStyleToRoof', content.includes('ApplyArchStyleToRoof'));
  check('uses ERok2ArchStyle', content.includes('ERok2ArchStyle::'));
}

// ---------------------------------------------------------------------------
// 4. Verify Rok2CityLayoutActor passes civ to buildings
// ---------------------------------------------------------------------------
console.log('\n[4] Rok2CityLayoutActor civ propagation');

const layoutCpp = join(PRIV, 'Rok2CityLayoutActor.cpp');
check('Rok2CityLayoutActor.cpp exists', existsSync(layoutCpp));

if (existsSync(layoutCpp)) {
  const content = readFileSync(layoutCpp, 'utf8');
  check('includes Rok2CivThemes.h', content.includes('Rok2CivThemes.h'));
  check('reads Player.Civ', content.includes('GetPlayer().Civ'));
  check('calls SetupWithCiv', content.includes('SetupWithCiv'));
  check('passes CivId to Wall', content.includes('Wall->CivId'));
}

// ---------------------------------------------------------------------------
// 5. Verify Rok2HexWallActor is theme-aware
// ---------------------------------------------------------------------------
console.log('\n[5] Rok2HexWallActor theme integration');

const wallH = join(PUB, 'Rok2HexWallActor.h');
const wallCpp = join(PRIV, 'Rok2HexWallActor.cpp');

check('Rok2HexWallActor.h exists', existsSync(wallH));
check('Rok2HexWallActor.cpp exists', existsSync(wallCpp));

if (existsSync(wallH)) {
  const content = readFileSync(wallH, 'utf8');
  check('has CivId property', content.includes('FString CivId'));
  check('has ApplyCivTheme method', content.includes('ApplyCivTheme'));
}

if (existsSync(wallCpp)) {
  const content = readFileSync(wallCpp, 'utf8');
  check('includes Rok2CivThemes.h', content.includes('Rok2CivThemes.h'));
  check('calls ApplyCivTheme in RebuildWall', content.includes('ApplyCivTheme()'));
}

// ---------------------------------------------------------------------------
// 6. Verify Rok2WorldRenderer uses civ theme for own city
// ---------------------------------------------------------------------------
console.log('\n[6] Rok2WorldRenderer theme integration');

const worldCpp = join(PRIV, 'Rok2WorldRenderer.cpp');
check('Rok2WorldRenderer.cpp exists', existsSync(worldCpp));

if (existsSync(worldCpp)) {
  const content = readFileSync(worldCpp, 'utf8');
  check('includes Rok2CivThemes.h', content.includes('Rok2CivThemes.h'));
  check('reads Player.Civ', content.includes('GetPlayer().Civ'));
  check('uses MyTheme.Primary for own city', content.includes('MyTheme.Primary'));
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P5-T2 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
