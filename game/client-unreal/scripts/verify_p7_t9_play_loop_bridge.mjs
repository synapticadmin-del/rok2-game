#!/usr/bin/env node
/**
 * P7-T9 bridge-acceptance guard.
 *
 * Ensures the full play-loop acceptance document (registration → civ
 * carousel → city → world exploration → rally/pass combat → reconnect →
 * performance measurement) exists and references every established backend
 * E2E and asset contract, while keeping explicit reservations that the
 * engine-side acceptance must be executed on Windows/UE 5.4.4.
 */

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const clientRoot = path.resolve(scriptDir, '..');
const repoRoot = path.resolve(clientRoot, '..');

const read = (relative) => fs.readFileSync(path.join(repoRoot, relative), 'utf8');
const failures = [];
function requireText(relativePath, tokens, label) {
  const content = read(relativePath);
  for (const token of tokens) {
    if (!content.includes(token)) failures.push(`${label}: missing "${token}"`);
  }
}

// ---- 1. The bridge document itself ----
requireText('docs/P7_T9_FULL_PLAY_LOOP_ACCEPTANCE.md',
  [
    'كاروسيلًا بصريًا',                      // civ carousel in PIE step
    'e2e_p7_t1_guest_onboarding.mjs',        // registration proof
    'e2e_p7_t3_player_journey.mjs',          // social loop proof
    'test:reconnect-restore',                // reconnect proof
    'P7_T6_WORLD_PERFORMANCE_BUDGET.md',     // benchmark scene proof
    '30 FPS',                                // budget floor
    'Kenney Castle Kit',                     // license traceability
    'Windows/UE 5.4.4',                      // engine-side reservation
    'مراجعة قانونية',                         // legal review of generated assets
  ],
  'bridge acceptance doc');

// ---- 2. The visual civ-selection path in BootWidget stays intact ----
requireText('client-unreal/Source/Rok2/Private/Rok2BootWidget.cpp',
  [
    'OnPreviousCivClicked',                  // visual carousel navigation
    'OnNextCivClicked',                      // visual carousel navigation
    'URok2CivLoreRegistry::Get()',           // unified civ data source
    'URok2ArtAssets::GetIconBrush',          // runtime-safe art resolution
  ],
  'Rok2BootWidget.cpp');

// ---- 3. Asset pipelines still wired for runtime-safe imports ----
requireText('client-unreal/Source/Rok2/Private/Rok2CityWidget.cpp',
  ['ApplyButtonSkin', 'ResolveCityBuildingArtId'],
  'Rok2CityWidget.cpp');

// ---- 4. No placeholder promises in the docs ----
requireText('docs/P7_T9_FULL_PLAY_LOOP_ACCEPTANCE.md',
    ['## 1. الحالة التشغيلية المثبتة', '## 3. ما لا يتحقق بنويًا'],
  'bridge doc honesty');

if (failures.length) {
  console.error('P7-T9 play-loop bridge verification failed:');
  for (const f of failures) console.error(`- ${f}`);
  process.exit(1);
}

console.log('P7-T9 play-loop bridge verification passed (registration → civ carousel → city → world → rally/pass combat → reconnect → performance scene, with engine-side reservation documented).');
