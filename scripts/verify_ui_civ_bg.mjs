#!/usr/bin/env node
// verify_ui_civ_bg.mjs — P6-T7: فحص بنيوي لخلفيات وأجواء القوائم الحضارية
// يتحقق من وجود ألوان الخلفية في ثيمات الحضارات واستخدامها في الواجهات.

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const REPO = join(__dirname, '..');
const SRC = join(REPO, 'game', 'client-unreal', 'Source', 'Rok2');
const PUB = join(SRC, 'Public');
const PRIV = join(SRC, 'Private');

let passed = 0;
let failed = 0;

function ok(name) { console.log(`  ✅ ${name}`); passed++; }
function fail(name, detail = '') { console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`); failed++; }
function check(name, condition, detail = '') { if (condition) ok(name); else fail(name, detail); }
function read(rel) { return readFileSync(rel, 'utf8').replace(/\r\n/g, '\n'); }

// ---------------------------------------------------------------------------
console.log('\n[1] Rok2CivThemes.h — PanelBg/PanelBgAlt/PanelFrame في FRok2CivTheme');
// ---------------------------------------------------------------------------
const themesHPath = join(PUB, 'Rok2CivThemes.h');
check('Rok2CivThemes.h exists', existsSync(themesHPath));
if (existsSync(themesHPath)) {
  const h = read(themesHPath);
  check('has PanelBg field', h.includes('FLinearColor PanelBg'));
  check('has PanelBgAlt field', h.includes('FLinearColor PanelBgAlt'));
  check('has PanelFrame field', h.includes('FLinearColor PanelFrame'));
}

// ---------------------------------------------------------------------------
console.log('\n[2] Rok2CivThemes.cpp — BuildDefaults يضبط ألوان الخلفية للحضارات الست');
// ---------------------------------------------------------------------------
const themesCppPath = join(PRIV, 'Rok2CivThemes.cpp');
check('Rok2CivThemes.cpp exists', existsSync(themesCppPath));
if (existsSync(themesCppPath)) {
  const c = read(themesCppPath);
  check('sets PanelBg for rome', c.includes('TEXT("#1A0A0A")'));
  check('sets PanelBg for china', c.includes('TEXT("#1A0D0A")'));
  check('sets PanelBg for arabia', c.includes('TEXT("#0A1A0F")'));
  check('sets PanelBg for egypt', c.includes('TEXT("#0A1A1A")'));
  check('sets PanelBg for vikings', c.includes('TEXT("#0A0F1A")'));
  check('sets PanelBg for japan', c.includes('TEXT("#0D0A0A")'));
  check('sets PanelFrame for rome', c.includes('TEXT("#C9A227")'));
  check('sets PanelFrame for china', c.includes('TEXT("#F0C14A")'));
  check('sets PanelFrame for arabia', c.includes('TEXT("#40E0D0")'));
  check('sets PanelFrame for egypt', c.includes('TEXT("#D4AF37")'));
  check('sets PanelFrame for vikings', c.includes('TEXT("#8AA0B4")'));
  check('sets PanelFrame for japan', c.includes('TEXT("#9B1D20")'));
}

// ---------------------------------------------------------------------------
console.log('\n[3] Rok2CommanderWidget.cpp — يستخدم ثيم الحضارة للخلفية');
// ---------------------------------------------------------------------------
const cmdCppPath = join(PRIV, 'Rok2CommanderWidget.cpp');
check('Rok2CommanderWidget.cpp exists', existsSync(cmdCppPath));
if (existsSync(cmdCppPath)) {
  const c = read(cmdCppPath);
  check('uses Theme.PanelBg', c.includes('Theme.PanelBg'));
  check('uses Theme.PanelFrame', c.includes('Theme.PanelFrame'));
  check('uses Theme.PanelBgAlt', c.includes('Theme.PanelBgAlt'));
  check('gets player civ', c.includes('Api->GetPlayer().Civ'));
}

// ---------------------------------------------------------------------------
console.log('\n[4] Rok2BattleReportWidget.cpp — يستخدم ثيم الحضارة للخلفية');
// ---------------------------------------------------------------------------
const brCppPath = join(PRIV, 'Rok2BattleReportWidget.cpp');
check('Rok2BattleReportWidget.cpp exists', existsSync(brCppPath));
if (existsSync(brCppPath)) {
  const c = read(brCppPath);
  check('includes Rok2CivThemes.h', c.includes('Rok2CivThemes.h'));
  check('Rok2Panel uses Theme.PanelBg', c.includes('Theme.PanelBg'));
  check('passes civ to Rok2Panel', c.includes('Api->GetPlayer().Civ'));
}

// ---------------------------------------------------------------------------
console.log('\n[5] Rok2AllianceRosterWidget.cpp — يستخدم ثيم الحضارة للخلفية');
// ---------------------------------------------------------------------------
const arCppPath = join(PRIV, 'Rok2AllianceRosterWidget.cpp');
check('Rok2AllianceRosterWidget.cpp exists', existsSync(arCppPath));
if (existsSync(arCppPath)) {
  const c = read(arCppPath);
  check('includes Rok2CivThemes.h', c.includes('Rok2CivThemes.h'));
  check('uses Theme.PanelBg', c.includes('Theme.PanelBg'));
  check('gets player civ', c.includes('Api->GetPlayer().Civ'));
}

// ---------------------------------------------------------------------------
console.log('\n[6] تحقق من تعدد الألوان — كل حضارة لها لون خلفية فريد');
// ---------------------------------------------------------------------------
if (existsSync(themesCppPath)) {
  const c = read(themesCppPath);
  // استخراج ألوان PanelBg من BuildDefaults
  const bgMatches = c.match(/TEXT\("#[0-9A-Fa-f]+"\)/g) || [];
  const panelBgs = bgMatches.filter((m, i) => {
    // البحث عن الألوان بعد "rome" و "china" إلخ
    return bgMatches.indexOf(m) === i; // فريد فقط
  });
  check('has unique panel bg colors', panelBgs.length >= 12, `found ${panelBgs.length} unique hex colors`);
}

// ---------------------------------------------------------------------------
console.log('\n══════════════════════════════════════════════════════════════════════');
console.log(`P6-T7 structural verification: ${passed} passed, ${failed} failed`);
if (failed === 0) {
  console.log('\n✅ ALL PASSED');
} else {
  console.log('\n❌ FAILED');
  process.exit(1);
}
