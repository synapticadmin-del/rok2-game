#!/usr/bin/env node
/**
 * P7-T9 civilization showcase contract guard.
 *
 * This is intentionally a static, engine-independent test. It ensures that the
 * art sources needed by Unreal are real PNG bytes and that BootWidget, import,
 * build, and PIE documentation retain the complete user path.
 */
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';

const scriptDir = path.dirname(new URL(import.meta.url).pathname);
const clientRoot = path.resolve(scriptDir, '..');
const repoRoot = path.resolve(clientRoot, '..', '..', '..');
const civs = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];
const pngMagic = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
let failures = 0;

function requireText(relativePath, needles) {
  const absolutePath = path.join(clientRoot, relativePath);
  const text = fs.readFileSync(absolutePath, 'utf8');
  for (const needle of needles) {
    if (!text.includes(needle)) {
      console.error(`FAIL ${relativePath}: missing ${needle}`);
      failures += 1;
    }
  }
}

function requirePng(relativePath) {
  const absolutePath = path.join(clientRoot, relativePath);
  if (!fs.existsSync(absolutePath)) {
    console.error(`FAIL missing asset: ${relativePath}`);
    failures += 1;
    return;
  }
  const bytes = fs.readFileSync(absolutePath);
  if (bytes.length < pngMagic.length || !bytes.subarray(0, pngMagic.length).equals(pngMagic)) {
    console.error(`FAIL non-PNG binary asset: ${relativePath}`);
    failures += 1;
  }
}

requireText('Source/Rok2/Public/Rok2BootWidget.h', [
  'BuildCivShowcase', 'ShowCivVisuals', 'SelectCivIndex',
  'PreviousCivButton', 'NextCivButton',
]);
requireText('Source/Rok2/Private/Rok2BootWidget.cpp', [
  'BuildCivShowcase', 'ShowCivVisuals', 'CivBackgrounds', 'CivIcons',
  'icon_%s_runtime', 'Commanders', 'OnPreviousCivClicked', 'OnNextCivClicked',
  'SetSelectedIndex', 'URok2CivLoreRegistry::Get()',
]);
requireText('scripts/Import-CivVisuals.ps1', [
  "'-run=ImportAssets'", 'CivBackgrounds', 'CivIcons', 'CivCommanders',
  'CivCommanders2', 'TextureFactory', 'icon_$($_)_runtime.png',
]);
requireText('scripts/Build-Rok2.ps1', [
  '$ImportCivVisuals', 'Import-CivVisuals.ps1', '-ReplaceExisting',
]);
requireText('Docs/BUILD_AND_PIE.md', [
  'Import-CivVisuals.ps1', '-ImportCivVisuals', 'PIE-09', 'PIE-10', 'PIE-11',
]);
requireText('../docs/P7_T9_VISUAL_ASSET_AUDIT.md', [
  'P7-T9', 'وجهة Unreal بعد الاستيراد', 'مراجعة قانونية',
]);

for (const civ of civs) {
  requirePng(`Content/Art/CivBackgrounds/bg_${civ}.png`);
  requirePng(`Content/Art/CivIcons/icon_${civ}_runtime.png`);
  requirePng(`Content/Art/Commanders/cmd_${civ}_starter.png`);
  requirePng(`Content/Art/Commanders/cmd_${civ}_2.png`);
}

const roster2 = [
  'cmd_rome_2', 'cmd_china_2', 'cmd_arabia_2',
  'cmd_egypt_2', 'cmd_vikings_2', 'cmd_japan_2',
];
for (const id of roster2) {
  requirePng(`Content/Art/Commanders/${id}.png`);
}

if (failures > 0) {
  console.error(`\nP7-T9 civilization showcase verification failed (${failures} issue(s)).`);
  process.exit(1);
}
console.log('P7-T9 civilization showcase verification passed: 24 Unreal-importable PNGs and complete UI/build contracts found.');
