import { existsSync, readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const root = resolve(__dirname, '../../..');

console.log('=== P12-T4 License Audit Guard ===');

let pass = 0;
let fail = 0;

function check(desc, condition) {
  if (condition) {
    console.log(`  PASS: ${desc}`);
    pass++;
  } else {
    console.error(`  FAIL: ${desc}`);
    fail++;
  }
}

// 1. Audit Document
const auditDocPath = resolve(root, 'game/docs/P12_T4_LICENSE_AUDIT.md');
check('P12_T4_LICENSE_AUDIT.md exists', existsSync(auditDocPath));
if (existsSync(auditDocPath)) {
  const content = readFileSync(auditDocPath, 'utf8');
  check('Audit doc mentions CC0 1.0 Universal', content.includes('CC0 1.0 Universal'));
  check('Audit doc mentions Kenney Assets', content.includes('Kenney Assets'));
  check('Audit doc mentions Procedural GLB 2.0', content.includes('Procedural GLB 2.0') || content.includes('GLB 2.0'));
  check('Audit doc covers audio SFX', content.includes('Audio') || content.includes('SFX'));
}

// 2. Kenney Castle Kit License File
const kenneyLicPath = resolve(root, 'game/client-unreal/Content/Art/KenneyCastleKit/LICENSE_KENNEY_CASTLE_KIT.txt');
check('Kenney Castle Kit LICENSE_KENNEY_CASTLE_KIT.txt exists', existsSync(kenneyLicPath));

// 3. Procedural Asset Generators
const humanGenPath = resolve(root, 'scripts/generate_human_units_glb.py');
const tavernGenPath = resolve(root, 'scripts/generate_tavern_assets.py');
const audioGenPath = resolve(root, 'scripts/generate_audio.py');
check('generate_human_units_glb.py exists', existsSync(humanGenPath));
check('generate_tavern_assets.py exists', existsSync(tavernGenPath));
check('generate_audio.py exists', existsSync(audioGenPath));

console.log(`\nResults: ${pass} passed, ${fail} failed.`);
if (fail > 0) {
  process.exit(1);
} else {
  console.log('ALL PASSED: P12-T4 License and IP audit verified.\n');
}
