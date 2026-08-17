#!/usr/bin/env node
/**
 * P7-T3 — Two-player journey E2E contract.
 *
 * Structural verification (no Unreal tools required; no server is started):
 * - The E2E script exists with full-path and isolated modes documented.
 * - It covers FTUE foundation, alliance membership, rally formation,
 *   ownership of participants, session rehydration, launch, combat reports
 *   for leader and participant, season-story milestone, and march settle.
 * - Privacy evidence: anonymous world snapshot rejection.
 * - The package.json job runs the full path (E2E_FULL=1) and is in the check chain.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { chainRuns } from '../../../scripts/lib/npm_script_chain.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(here, '..', '..', '..');
const BACKEND = path.join(REPO, 'game', 'backend');

function read(relativePath, base = BACKEND) {
  return fs.readFileSync(path.join(base, relativePath), 'utf8');
}

const checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}

const script = read('scripts/e2e_p7_t3_player_journey.mjs');
const pkg = JSON.parse(read('package.json'));

// 1. Script presence and mode documentation
expect('e2e script exists', fs.existsSync(path.join(BACKEND, 'scripts', 'e2e_p7_t3_player_journey.mjs')));
expect('isolated-server mode documented', script.includes('E2E_LIVE=1'));
expect('isolated mode described', script.includes('بيئة معزولة مؤقتة'));
expect('full-path env flag E2E_FULL', script.includes('E2E_FULL'));
expect('full-path env flag WAIT_FOR_RALLY', script.includes('WAIT_FOR_RALLY'));

// 2. Journey coverage — two players from foundation to pass
expect('guest authentication for two players', script.includes('/v1/auth/guest'));
expect('FTUE city foundation', script.includes('/v1/city/init'));
expect('two different civilizations', script.includes('"rome"') && script.includes('"china"'));
expect('shared world snapshot check', script.includes('both newly founded cities share the same world snapshot'));
expect('alliance create and join', script.includes('/v1/alliance/create') && script.includes('/v1/alliance/join'));
expect('rally on pass target', script.includes('targetType: "pass"'));
expect('rally join with separate contribution', script.includes('/v1/alliance/rally/join'));
expect('participant identity preservation', script.includes('rally detail preserves both participant identities'));
expect('session rehydration check', script.includes('reloaded session restores joined-rally state'));

// 3. Full-path coverage — launch, combat report, season story, return
expect('server launch to launched march', script.includes('server-launched march'));
expect('combat report for rally leader', script.includes('rally leader receives the resulting battle report'));
expect('combat report for rally participant', script.includes('rally participant receives the resulting battle report'));
expect('both loss sides in report', script.includes('attackerLosses') && script.includes('defenderLosses'));
expect('season story milestone', script.includes('first_pass_capture') || script.includes('pass_conquered'));
expect('march settle without FK error', script.includes('rally return settles'));

// 4. Privacy and authority
expect('anonymous snapshot rejection', script.includes('rejects unauthenticated access'));
expect('admin grant fixture only', script.includes('x-admin-key'));

// 5. E2E-only zones.json shortening — backup before modify, restore in teardown
expect('zones.json shortened for E2E only (8s)', script.includes('zones.alliance.rally.prep_seconds = 8'));
expect('prep_seconds restored in teardown', script.includes('z2.alliance.rally.prep_seconds = zonesBackup'));

// 6. Package job: full path and part of check chain
const job = pkg.scripts['test:e2e-p7-t3'];
expect('test:e2e-p7-t3 runs the full path (E2E_FULL=1)', Boolean(job) && job.includes('E2E_FULL=1'));
// البوابة صارت مركّبة (check → check:fast/check:e2e/check:ue-contracts)، فالبحث
// الحرفي في سطر check وحده يبلّغ غياباً وهمياً — chainRuns يوسّع المراجع تعدياً.
expect('test:e2e-p7-t3 in check chain', chainRuns(pkg.scripts, 'test:e2e-p7-t3'));

// 7. Docs reference
const docs = path.join(REPO, 'game', 'docs', 'P7_T3_TWO_PLAYERS_E2E.md');
expect('P7-T3 E2E document exists', fs.existsSync(docs));
if (fs.existsSync(docs)) {
  const doc = fs.readFileSync(docs, 'utf8');
  expect('doc covers the two-player flow table', doc.includes('FTUE'));
  expect('doc documents run steps', doc.includes('E2E_FULL'));
}

// Report
let passed = 0;
let failed = 0;
for (const c of checks) {
  if (c.ok) { passed += 1; console.log('OK  :', c.name); }
  else { failed += 1; console.error('FAIL:', c.name, c.detail); }
}
console.log(`\n${passed}/${checks.length} checks passed.`);
if (failed === 0) {
  console.log('P7-T3 two-player journey contract verified.');
  process.exit(0);
}
console.error('P7-T3 contract FAILED.');
process.exit(1);
