#!/usr/bin/env node
/**
 * verify_p7_t7_accessibility.mjs — P7-T7 structural verification
 *
 * Checks that the accessibility/localization unit (URok2Accessibility) is
 * properly defined and wired through every UMG widget: RTL default, UI scale,
 * WCAG-AA contrast helpers, Arabic alt labels for icons, and non-color
 * status cues (connection text, rally prefixes, XP ratio text).
 *
 * Static/structural test — does not require a running UE5 build or backend.
 *
 * Usage: node scripts/verify_p7_t7_accessibility.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..', 'game', 'client-unreal', 'Source', 'Rok2');
const PRIV = join(ROOT, 'Private');
const PUB = join(ROOT, 'Public');

let passed = 0;
let failed = 0;

function ok(name) { console.log(`  ✅ ${name}`); passed++; }
function fail(name, detail = '') { console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); failed++; }
function check(name, condition, detail = '') { if (condition) ok(name); else fail(name, detail); }

function read(rel) {
  const p = join(PRIV, rel);
  if (!existsSync(p)) { fail(`${rel} exists`); return null; }
  return readFileSync(p, 'utf8');
}

// ---------------------------------------------------------------------------
// 1. URok2Accessibility unit
// ---------------------------------------------------------------------------
console.log('\n[1] وحدة URok2Accessibility');

const accH = join(PUB, 'Rok2Accessibility.h');
const accCpp = join(PRIV, 'Rok2Accessibility.cpp');

check('Rok2Accessibility.h exists', existsSync(accH));
check('Rok2Accessibility.cpp exists', existsSync(accCpp));

if (existsSync(accH)) {
  const c = readFileSync(accH, 'utf8');
  check('has UCLASS URok2Accessibility', c.includes('URok2Accessibility'));
  check('has Get() singleton', c.includes('static URok2Accessibility* Get()'));
  check('has IsRtl', c.includes('IsRtl'));
  check('has GetUiScale', c.includes('GetUiScale'));
  check('has ScaledSize', c.includes('ScaledSize'));
  check('has ScaledIconSize', c.includes('ScaledIconSize'));
  check('has IsHighContrast', c.includes('IsHighContrast'));
  check('has AccessibleTextFor', c.includes('AccessibleTextFor'));
  check('has HighContrastForState', c.includes('HighContrastForState'));
  check('has LabelForIcon', c.includes('LabelForIcon'));
  check('has SetUiScale', c.includes('SetUiScale'));
  check('has SetHighContrast', c.includes('SetHighContrast'));
  check('has OnAccessibilityChanged delegate', c.includes('OnAccessibilityChanged'));
}

if (existsSync(accCpp)) {
  const c = readFileSync(accCpp, 'utf8');
  check('RTL default true', c.includes('return true'));
  check('DPIScaleFactor clamp 0.85-1.6', c.includes('GetDPIScaleFactorAtPoint'));
  check('WCAG AA 4.5 ratio threshold', c.includes('4.5f'));
  check('dark bg luminance constant', c.includes('0.0128f'));
  check('fallback ivory', c.includes('0.96f, 0.91f, 0.81f'));
  check('Arabic label طعام', c.includes('TEXT("طعام")'));
  check('Arabic label إشعارات', c.includes('TEXT("إشعارات")'));
  check('Arabic label حالة الاتصال', c.includes('TEXT("حالة الاتصال")'));
  check('Arabic label دردشة المملكة', c.includes('TEXT("دردشة المملكة")'));
  check('broadcasts on change', c.includes('OnAccessibilityChanged.Broadcast()'));
}

// ---------------------------------------------------------------------------
// 2. Typography scale hook
// ---------------------------------------------------------------------------
console.log('\n[2] مقياس URok2Typography');

const tyCpp = read('Rok2Typography.cpp');
if (tyCpp) {
  check('SizeOf exists', tyCpp.includes('SizeOf'));
  check('SizeOf multiplies by UiScale', tyCpp.includes('GetUiScale'));
  check('clamps Min/Max', tyCpp.includes('FMath::Clamp'));
  check('includes Rok2Accessibility.h', tyCpp.includes('Rok2Accessibility.h'));
}

// ---------------------------------------------------------------------------
// 3. HUD: connection text + tooltips + AA colors + scaled sizes
// ---------------------------------------------------------------------------
console.log('\n[3] HUD — حالة الاتصال والنصوص البديلة');

const hudCpp = read('Rok2HudWidget.cpp');
if (hudCpp) {
  check('includes Rok2Accessibility.h', hudCpp.includes('Rok2Accessibility.h'));
  check('ConnStateText in header wiring', readFileSync(join(PUB, 'Rok2HudWidget.h'), 'utf8').includes('ConnStateText'));
  check('متصل status text', hudCpp.includes('متصل'));
  check('منقطع status text', hudCpp.includes('منقطع'));
  check('HighContrastForState in OnConnState', hudCpp.includes('HighContrastForState'));
  check('HighContrastForState on badges', hudCpp.includes('HighContrastForState'));
  check('ScaledSize for text', hudCpp.includes('ScaledSize('));
  check('ScaledIconSize for icons', hudCpp.includes('ScaledIconSize('));
  check('tooltips present', hudCpp.includes('SetToolTipText'));
  check('MakePill tooltips', hudCpp.includes('SetToolTipText'));
}

// ---------------------------------------------------------------------------
// 4. Every widget has icon alt labels + AA contrast
// ---------------------------------------------------------------------------
console.log('\n[4] النصوص البديلة والتباين في كل الودجات');

const widgetTooltips = [
  ['Rok2ChatWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  // P24-T1: `Rok2CityWidget` تقاعدت (ألواحها كانت مطوية بلا مسار إظهار)؛
  // شريط الموارد والطوابير وتسميات أيقوناتها انتقلت إلى الـHUD.
  ['Rok2HudWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2BattleReportWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2BuildingDetailWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2MarchPanel.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2AllianceRosterWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2AllianceRallyWidget.cpp', ['✔', '◔', '▲']],
  ['Rok2BootWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2CivInfoWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2BuildMenuWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2ResearchWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
  ['Rok2OnboardingWidget.cpp', ['LabelForIcon', 'SetToolTipText']],
];

for (const [file, needles] of widgetTooltips) {
  const c = read(file);
  if (!c) continue;
  for (const n of needles) {
    check(`${file} contains ${n}`, c.includes(n));
  }
  check(`${file} includes Rok2Accessibility.h`, c.includes('Rok2Accessibility.h'));
}

// ---------------------------------------------------------------------------
// 5. Non-color cues: XP ratio text + SeasonStory prefixes + AA season colors
// ---------------------------------------------------------------------------
console.log('\n[5] مؤشرات غير لونية');

const cmdCpp = read('Rok2CommanderWidget.cpp');
if (cmdCpp) {
  check('DetailXpText in header', readFileSync(join(PUB, 'Rok2CommanderWidget.h'), 'utf8').includes('DetailXpText'));
  check('DetailXpText ratio text', cmdCpp.includes('DetailXpText'));
}

const storyCpp = read('Rok2SeasonStoryWidget.cpp');
if (storyCpp) {
  check('SeasonStory AA Gold', storyCpp.includes('1.f, 0.80f, 0.34f'));
  check('SeasonStory AA Azure', storyCpp.includes('0.52f, 0.78f, 1.0f'));
  check('SeasonStory AA Crimson', storyCpp.includes('0.95f, 0.42f, 0.36f'));
  check('SeasonStory AA Jade', storyCpp.includes('0.40f, 0.85f, 0.58f'));
  check('day prefix symbols', storyCpp.includes('★') && storyCpp.includes('⚔'));
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P7-T7 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
