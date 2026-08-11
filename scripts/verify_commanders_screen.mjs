#!/usr/bin/env node
/**
 * verify_commanders_screen.mjs — P5-T4 structural verification
 *
 * Checks that the commander screen is properly wired: commander data loading,
 * UI components, skill display, talent/equipment stubs, and API integration.
 * This is a static/structural test — it does not require a running UE5 build.
 *
 * Usage: node scripts/verify_commanders_screen.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = __dirname;
// جذر المستودع = scripts/.. — الملفات الحقيقية في data/ وSource/Rok2/{Public,Private}
const REPO = join(__dirname, '..');
const DATA = join(REPO, 'data');
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
// 1. Verify data/commanders.json exists and has commanders with skills
// ---------------------------------------------------------------------------
console.log('\n[1] data/commanders.json');

const cmdJsonPath = join(DATA, 'commanders.json');
check('commanders.json exists (workspace copy)', existsSync(cmdJsonPath));

if (existsSync(cmdJsonPath)) {
  const data = JSON.parse(readFileSync(cmdJsonPath, 'utf8'));
  const cmdrs = data.commanders;
  check('>= 12 commanders present (P4-T5 added 6)', cmdrs.length >= 12, `got ${cmdrs.length}`);

  // Check first 6 starters have nation matching civs
  const starters = cmdrs.filter(c => c.id.includes('_starter'));
  check('6 starter commanders', starters.length === 6, `got ${starters.length}`);

  const nations = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];
  for (const n of nations) {
    const c = starters.find(x => x.nation === n);
    check(`starter for nation "${n}"`, !!c);
    if (c) {
      check(`  has 3 skills`, c.skills?.length === 3, `got ${c.skills?.length}`);
      check(`  has attack skill`, c.skills?.some(s => s.type === 'attack'));
      check(`  has defense skill`, c.skills?.some(s => s.type === 'defense'));
      check(`  has passive skill`, c.skills?.some(s => s.type === 'passive'));
      check(`  has base_stats`, !!c.base_stats);
    }
  }

  // Check legendary commanders
  const legendaries = cmdrs.filter(c => c.rarity === 'legendary');
  check('>= 6 legendary commanders', legendaries.length >= 6, `got ${legendaries.length}`);
}

// ---------------------------------------------------------------------------
// 2. Verify Rok2CommanderWidget.h has all required components
// ---------------------------------------------------------------------------
console.log('\n[2] Rok2CommanderWidget.h structure');

const widgetH = join(PUB, 'Rok2CommanderWidget.h');
check('Rok2CommanderWidget.h exists', existsSync(widgetH));

if (existsSync(widgetH)) {
  const content = readFileSync(widgetH, 'utf8');
  check('has FRok2CommanderSkillData struct', content.includes('FRok2CommanderSkillData'));
  check('has FRok2CommanderDetailData struct', content.includes('FRok2CommanderDetailData'));
  check('has SetupWithApi method', content.includes('SetupWithApi'));
  check('has RefreshCommanderList method', content.includes('RefreshCommanderList'));
  check('has SelectCommander method', content.includes('SelectCommander'));
  check('has OnCommanderSelected delegate', content.includes('OnCommanderSelected'));
  check('has OnAssignCommander delegate', content.includes('OnAssignCommander'));
  check('has BuildPortraitPlaceholder', content.includes('BuildPortraitPlaceholder'));
  check('has BuildSkillRow', content.includes('BuildSkillRow'));
  check('has BuildTalentTreeStub', content.includes('BuildTalentTreeStub'));
  check('has BuildEquipmentSlots', content.includes('BuildEquipmentSlots'));
  check('has LoadCommanderDetailsFromJson', content.includes('LoadCommanderDetailsFromJson'));
  check('has RarityColor helper', content.includes('RarityColor'));
  check('has StarsForRarity helper', content.includes('StarsForRarity'));
}

// ---------------------------------------------------------------------------
// 3. Verify Rok2CommanderWidget.cpp implementation
// ---------------------------------------------------------------------------
console.log('\n[3] Rok2CommanderWidget.cpp implementation');

const widgetCpp = join(PRIV, 'Rok2CommanderWidget.cpp');
check('Rok2CommanderWidget.cpp exists', existsSync(widgetCpp));

if (existsSync(widgetCpp)) {
  const content = readFileSync(widgetCpp, 'utf8');
  check('includes Rok2CivThemes.h', content.includes('Rok2CivThemes.h'));
  check('includes Rok2Api.h', content.includes('Rok2Api.h'));
  check('reads commanders.json from disk', content.includes('commanders.json'));
  check('has BuildUI method', content.includes('BuildUI()'));
  check('has NativeConstruct', content.includes('NativeConstruct()'));
  check('uses COLOR_GOLD', content.includes('COLOR_GOLD'));
  check('uses COLOR_BRONZE_BG', content.includes('COLOR_BRONZE_BG'));
  check('has rarity colors defined', content.includes('COLOR_LEGENDARY') && content.includes('COLOR_EPIC'));
  check('BuildPortraitPlaceholder uses CivThemes', content.includes('URok2CivThemes::Get()'));
  check('BuildSkillRow has attack/defense/passive icons (P6-T1: procedural icons)', content.includes('TEXT("sword")') && content.includes('TEXT("shield")') && content.includes('TEXT("sparkle")'));
  check('BuildTalentTreeStub has 3 branches', content.includes('قتال') && content.includes('دعم') && content.includes('حركة'));
  check('BuildEquipmentSlots has 5 slots', content.includes('سلاح') && content.includes('خوذة') && content.includes('درع') && content.includes('حذاء') && content.includes('إكسسوار'));
  check('has OnAssignClicked', content.includes('OnAssignClicked'));
  check('has OnLevelUpClicked', content.includes('OnLevelUpClicked'));
  check('has OnSkillUpgradeClicked', content.includes('OnSkillUpgradeClicked'));
  check('has PopulateDetailPanel', content.includes('PopulateDetailPanel'));
  check('has star display (★/☆)', content.includes('★') && content.includes('☆'));
  check('has XP progress bar', content.includes('DetailXpBar'));
}

// ---------------------------------------------------------------------------
// 4. Verify integration points
// ---------------------------------------------------------------------------
console.log('\n[4] Integration with existing systems');

// Check that Rok2Types.h has FRok2Commander
const typesH = join(PUB, 'Rok2Types.h');
check('Rok2Types.h exists', existsSync(typesH));
if (existsSync(typesH)) {
  const content = readFileSync(typesH, 'utf8');
  check('has FRok2Commander struct', content.includes('FRok2Commander'));
  check('FRok2Commander has Rarity field', content.includes('Rarity'));
  check('FRok2Commander has Nation field', content.includes('Nation'));
  check('FRok2Commander has Tags field', content.includes('Tags'));
}

// Check Rok2Api.h has GetCommanders
const apiH = join(PUB, 'Rok2Api.h');
check('Rok2Api.h exists', existsSync(apiH));
if (existsSync(apiH)) {
  const content = readFileSync(apiH, 'utf8');
  check('has GetCommanders method', content.includes('GetCommanders'));
}

// Check Rok2CivThemes.h exists (from P5-T2)
const civThemesH = join(PUB, 'Rok2CivThemes.h');
check('Rok2CivThemes.h exists (P5-T2)', existsSync(civThemesH));

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P5-T4 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
