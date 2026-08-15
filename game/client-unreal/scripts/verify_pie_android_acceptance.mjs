#!/usr/bin/env node
/**
 * P7 acceptance guard: protects the reproducible Windows/UE 5.4.4 preparation
 * for the two-client PIE review and Android Development performance run.
 * It intentionally does not claim an editor, device, or real measurement exists.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const clientRoot = path.resolve(scriptDir, '..');
const gameRoot = path.resolve(clientRoot, '..');
const backendPackage = path.join(gameRoot, 'backend', 'package.json');

const files = {
  build: path.join(scriptDir, 'Build-Rok2.ps1'),
  prepare: path.join(scriptDir, 'Prepare-AndroidDevelopment.ps1'),
  acceptance: path.join(clientRoot, 'Docs', 'PIE_TWO_CLIENTS_ANDROID_ACCEPTANCE.md'),
  engine: path.join(clientRoot, 'Config', 'DefaultEngine.ini'),
  // إعدادات التحزيم تُقرأ من DefaultGame.ini وحده: ProjectPackagingSettings
  // معلَّنة UCLASS(config=Game)، فوجودها في DefaultEngine.ini لا يُقرأ (P16-T3).
  game: path.join(clientRoot, 'Config', 'DefaultGame.ini'),
  package: backendPackage,
};

const text = Object.fromEntries(Object.entries(files).map(([name, file]) => [name, fs.readFileSync(file, 'utf8')]));
const checks = [
  ['Build script accepts Android', text.build.includes("[ValidateSet('Win64', 'Android')]")],
  ['Build script rejects Android Editor target', text.build.includes("لا يدعم Android هدف Editor")],
  ['Build script passes an explicit selected-platform argument to UAT', text.build.includes('$UatPlatformArgument') && text.build.includes("'-platform=Android'")],
  ['Preparation script pins UE patch 5.4.4', text.prepare.includes("$ActualVersion -ne '5.4.4'")],
  ['Preparation script validates Android project contracts', text.prepare.includes("'TargetSDKVersion=34'") && text.prepare.includes("'MinSDKVersion=26'") && text.prepare.includes("'bSupportsVulkan=True'")],
  ['Preparation script verifies Turnkey before declaring readiness', text.prepare.includes("'-Command=VerifySdk'")],
  ['Preparation script requires explicit install opt-in', text.prepare.includes('[switch]$InstallSdk') && text.prepare.includes("'-Command=InstallSDK'")],
  ['Project Android settings target SDK 34', text.engine.includes('TargetSDKVersion=34')],
  ['Project Android settings target Vulkan', text.engine.includes('bSupportsVulkan=True')],
  ['Project packages Development builds', text.game.includes('BuildConfiguration=PPBC_Development')],
  ['Packaging settings live in the config file Unreal actually reads', !text.engine.includes('[/Script/UnrealEd.ProjectPackagingSettings]')],
  ['Acceptance guide covers two independent clients', text.acceptance.includes('T3-PIE-01') && text.acceptance.includes('T3-PIE-06')],
  ['Acceptance guide requires real Android evidence', text.acceptance.includes('stat unit') && text.acceptance.includes('stat RHI') && text.acceptance.includes('stat memory')],
  ['Acceptance guide distinguishes preparation from device measurement', text.acceptance.includes('لا يفي نجاح `Prepare-AndroidDevelopment.ps1` أو بناء APK وحده بشرط القياس')],
  ['Backend quality gate invokes this guard', text.package.includes('test:pie-android-acceptance') && text.package.includes('npm run test:pie-android-acceptance')],
];

const failures = checks.filter(([, passed]) => !passed).map(([label]) => label);
if (failures.length) {
  console.error('P7 PIE/Android acceptance guard failed:');
  for (const failure of failures) console.error(`- ${failure}`);
  process.exit(1);
}

console.log(`P7 PIE/Android acceptance guard passed (${checks.length} contracts).`);
