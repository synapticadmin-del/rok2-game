#!/usr/bin/env node
/**
 * verify_ui_icons.mjs — P6-T1 structural verification
 *
 * Checks that the unified procedural UI icon system is properly defined and
 * wired through the client code: URok2IconLibrary (procedural UTexture2D
 * renderer), URok2ArtAssets icon facade, and zero emoji left in any widget.
 *
 * Static/structural test — does not require a running UE5 build or backend.
 *
 * Usage: node scripts/verify_ui_icons.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..', 'game', 'client-unreal', 'Source', 'Rok2');

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

// Emoji regex — أي رمز تعبيري حقيقي (لا نجوم ★☆ ولا أسهم نصية)
const EMOJI_RE = /[\u{1F000}-\u{1FAFF}\u{2B00}-\u{2BFF}\u{FE0F}]/u;

// ---------------------------------------------------------------------------
// 1. URok2IconLibrary class exists with the procedural renderer
// ---------------------------------------------------------------------------
console.log('\n[1] URok2IconLibrary — مكتبة الرسم الإجرائي');

const iconH = join(ROOT, 'Public', 'Rok2IconLibrary.h');
const iconCpp = join(ROOT, 'Private', 'Rok2IconLibrary.cpp');

check('Rok2IconLibrary.h exists', existsSync(iconH));
check('Rok2IconLibrary.cpp exists', existsSync(iconCpp));

if (existsSync(iconH)) {
  const c = readFileSync(iconH, 'utf8');
  check('has URok2IconLibrary UCLASS', c.includes('URok2IconLibrary'));
  check('has Get singleton', c.includes('static URok2IconLibrary* Get()'));
  check('has HasIcon', c.includes('HasIcon'));
  check('has GetIconIds', c.includes('GetIconIds'));
  check('has GetBrush returning FSlateBrush', c.includes('FSlateBrush GetBrush'));
  check('has MakeIconImage helper', c.includes('MakeIconImage'));
  check('has MakeIconLabel helper', c.includes('MakeIconLabel'));
  check('has ERok2IconSize with 24/32/48', c.includes('Small  = 24') && c.includes('Medium = 32') && c.includes('Large  = 48'));
  check('has BrushFromArtAssets bridge', c.includes('BrushFromArtAssets'));
}

if (existsSync(iconCpp)) {
  const c = readFileSync(iconCpp, 'utf8');
  check('renders UTexture2D transient', c.includes('UTexture2D::CreateTransient'));
  check('icon cache TMap', c.includes('Cache'));
  check('ivory palette from ui-ux doc', c.includes('245, 233, 208'));
  check('has DrawFood (resource icon)', c.includes('DrawFood'));
  check('has DrawBuild (hammer)', c.includes('DrawBuild'));
  check('has DrawSword', c.includes('DrawSword'));
  check('has DrawShield', c.includes('DrawShield'));
  check('has DrawBell', c.includes('DrawBell'));
  check('has DrawScroll', c.includes('DrawScroll'));
  check('has DrawCross (hospital)', c.includes('DrawCross'));
  check('has DrawScout', c.includes('DrawScout'));
  check('has DrawCrown', c.includes('DrawCrown'));
  check('has DrawStar5 (star shape)', c.includes('Star5'));
  check('canvas has Disc primitive', c.includes('void Disc('));
  check('canvas has Ring primitive', c.includes('void Ring('));
  check('canvas has Line primitive', c.includes('void Line('));
  check('canvas has Triangle primitive', c.includes('void Triangle('));
}

// ---------------------------------------------------------------------------
// 2. Icon catalog coverage — every icon id used by widgets must resolve
// ---------------------------------------------------------------------------
console.log('\n[2] تغطية فهرس الأيقونات');

const REQUIRED_IDS = [
  'food', 'wood', 'stone', 'gold', 'gems', 'ap',
  'build', 'sword', 'shield', 'helmet', 'bag', 'banner',
  'scroll', 'map', 'edit', 'bell', 'conn', 'hourglass',
  'flask', 'cross', 'scout', 'close', 'star', 'skull',
  'trophy', 'handshake', 'refresh', 'gift', 'wheat', 'box',
  'mail', 'cart', 'horse', 'bow', 'tent', 'tower', 'castle',
  'bricks', 'rock', 'beer', 'scale', 'crown', 'builder',
  'speedup', 'governor', 'move', 'sparkle', 'ring', 'boots',
  'pickaxe', 'clock', 'art', 'monument', 'wrench', 'skillup',
];

if (existsSync(iconCpp)) {
  const c = readFileSync(iconCpp, 'utf8');
  for (const id of REQUIRED_IDS) {
    check(`icon id "${id}" registered in draw map`, c.includes(`TEXT("${id}")`));
  }
}

// ---------------------------------------------------------------------------
// 3. URok2ArtAssets icon facade (P6-T1: icons served from ArtAssets)
// ---------------------------------------------------------------------------
console.log('\n[3] واجهة URok2ArtAssets للأيقونات');

const artH = join(ROOT, 'Public', 'Rok2ArtAssets.h');
const artCpp = join(ROOT, 'Private', 'Rok2ArtAssets.cpp');

check('Rok2ArtAssets.h exists', existsSync(artH));
check('Rok2ArtAssets.cpp exists', existsSync(artCpp));

if (existsSync(artH)) {
  const c = readFileSync(artH, 'utf8');
  check('has GetIconBrush static', c.includes('static FSlateBrush GetIconBrush'));
  check('has HasIcon static', c.includes('static bool HasIcon'));
  check('includes SlateBrush', c.includes('Styling/SlateBrush.h'));
  check('default tint ivory', c.includes('0.96f, 0.91f, 0.81f'));
}

if (existsSync(artCpp)) {
  const c = readFileSync(artCpp, 'utf8');
  check('includes Rok2IconLibrary.h', c.includes('Rok2IconLibrary.h'));
  check('GetIconBrush delegates to IconLibrary', c.includes('URok2IconLibrary::BrushFromArtAssets'));
  check('HasIcon delegates', c.includes('URok2IconLibrary::Get()->HasIcon'));
}

// ---------------------------------------------------------------------------
// 4. Widgets use the icon system
// ---------------------------------------------------------------------------
console.log('\n[4] ربط الـ Widgets بنظام الأيقونات');

const widgetChecks = [
  // P24-T1: `Rok2CityWidget` تقاعدت — ألواحها الثلاثة كانت تُبنى ثم تُخفى
  // بـ`ESlateVisibility::Collapsed` بلا مسار يعيد إظهارها، فأيقوناتها لم
  // تُعرض قط. التحصيل والتدريب وتسريع الطوابير انتقلت إلى الـHUD.
  ['Rok2HudWidget.cpp', ['GetIconBrush', 'Rok2ArtAssets.h', 'TEXT("build")', 'TEXT("food")', 'TEXT("bell")', 'TEXT("conn")', 'TEXT("helmet")', 'TEXT("hourglass")', 'TEXT("speedup")']],
  ['Rok2BuildMenuWidget.cpp', ['GetIconBrush', 'TEXT("wheat")', 'TEXT("sword")', 'TEXT("art")', 'TEXT("castle")']],
  ['Rok2BuildingDetailWidget.cpp', ['GetIconBrush', 'ActionIconForBuilding', 'HeaderIcon', 'CostFoodIcon', 'TimeIcon']],
  ['Rok2CommanderWidget.cpp', ['GetIconBrush', 'TEXT("sparkle")', 'TEXT("boots")', 'TEXT("ring")', 'TEXT("skillup")']],
  ['Rok2MarchPanel.cpp', ['GetIconBrush', 'TEXT("scout")', 'TEXT("sword")']],
  ['Rok2BattleReportWidget.cpp', ['GetIconBrush', 'TEXT("trophy")', 'TEXT("handshake")', 'TEXT("scroll")']],
  ['Rok2BootWidget.cpp', ['GetIconBrush', 'TEXT("crown")']],
  ['Rok2AllianceRosterWidget.cpp', ['GetIconBrush', 'TEXT("handshake")']],
  ['Rok2ResearchWidget.cpp', ['GetIconBrush', 'TEXT("flask")']],
];

for (const [file, needles] of widgetChecks) {
  const p = join(ROOT, 'Private', file);
  check(`${file} exists`, existsSync(p));
  if (!existsSync(p)) continue;
  const c = readFileSync(p, 'utf8');
  for (const n of needles) {
    check(`  ${file} contains ${n}`, c.includes(n));
  }
}

// ---------------------------------------------------------------------------
// 5. Zero emoji left in any client source (UI requirement from the task)
// ---------------------------------------------------------------------------
console.log('\n[5] لا إيموجي متبقٍ في كود العميل');

const clientFiles = [
  'Private/Rok2HudWidget.cpp', 'Private/Rok2BuildMenuWidget.cpp',
  'Private/Rok2BuildingDetailWidget.cpp', 'Private/Rok2CommanderWidget.cpp',
  'Private/Rok2MarchPanel.cpp', 'Private/Rok2BattleReportWidget.cpp',
  'Private/Rok2BootWidget.cpp',
  'Private/Rok2AllianceRosterWidget.cpp', 'Private/Rok2ResearchWidget.cpp',
  'Private/Rok2Api.cpp',
];

for (const rel of clientFiles) {
  const p = join(ROOT, rel);
  if (!existsSync(p)) {
    fail(`${rel} readable for emoji scan`);
    continue;
  }
  const lines = readFileSync(p, 'utf8').split('\n');
  const emojiLines = [];
  lines.forEach((line, idx) => {
    // تعليقات توثق الاستبدال مسموح لها بذكر الإيموجي القديم
    if (line.trim().startsWith('//')) return;
    if (EMOJI_RE.test(line)) emojiLines.push(idx + 1);
  });
  check(`${rel} has zero emoji in code`, emojiLines.length === 0, `lines: ${emojiLines.slice(0, 5).join(',')}`);
}

// ---------------------------------------------------------------------------
// 6. Widget headers declare the new icon image members
// ---------------------------------------------------------------------------
console.log('\n[6] تصريحات الـ headers الجديدة');

const headerChecks = [
  // شريط الموارد الوحيد صار في الـHUD بعد تقاعد `Rok2CityWidget` (P24-T1)،
  // فأسماء نصوص الموارد هناك هي ResFoodText وأخواتها.
  ['Rok2HudWidget.h', ['UImage* ConnIcon', 'UImage* BellIcon', 'ResFoodText', 'ResWoodText', 'ResStoneText', 'ResGoldText']],
  ['Rok2BuildingDetailWidget.h', ['UImage* HeaderIcon', 'UImage* CostFoodIcon', 'UImage* ActionBtnIcon', 'ActionIconForBuilding']],
];

for (const [file, needles] of headerChecks) {
  const p = join(ROOT, 'Public', file);
  check(`${file} exists`, existsSync(p));
  if (!existsSync(p)) continue;
  const c = readFileSync(p, 'utf8');
  for (const n of needles) {
    check(`  ${file} declares ${n}`, c.includes(n));
  }
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P6-T1 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
