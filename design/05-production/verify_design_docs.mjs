import { access, readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../..');
const designRoot = path.join(repoRoot, 'design');

const requiredDocs = [
  'README.md',
  '00-governance/RESEARCH_NOTES.md',
  '01-visual/VISUAL_IDENTITY.md',
  '02-civilizations/CIVILIZATIONS_AND_COMMANDERS.md',
  '02-civilizations/profiles/README.md',
  '03-systems/POWER_MODEL.md',
  '04-world-map/BUILDING_CATALOG.md',
  '04-world-map/MAP_AND_ALLIANCE_INTERACTION.md',
  '04-world-map/ALLIANCE_DEFENSE_CONTRACT.md',
];
const legacyDocs = [
  '01-map/05-map-objects.md',
  '02-civilizations/civilizations.md',
  '02-civilizations/commanders-reference.md',
  '07-game-design/power-balance-map.md',
  '07-game-design/ui-ux-design-system.md',
];

const failures = [];
const docContent = new Map();
for (const relative of requiredDocs) {
  const filename = path.join(designRoot, relative);
  try {
    docContent.set(filename, await readFile(filename, 'utf8'));
  } catch {
    failures.push(`الوثيقة المطلوبة مفقودة: design/${relative}`);
  }
}

for (const [filename, content] of docContent) {
  const links = [...content.matchAll(/\[[^\]]*\]\(([^)]+)\)/g)].map((match) => match[1]);
  for (const href of links) {
    if (/^(https?:|mailto:|#)/.test(href)) continue;
    const target = href.split('#', 1)[0];
    if (!target) continue;
    const targetPath = path.resolve(path.dirname(filename), target);
    try {
      await access(targetPath);
    } catch {
      failures.push(`رابط داخلي مكسور: ${path.relative(repoRoot, filename)} → ${href}`);
    }
  }
}

const civilizations = JSON.parse(await readFile(path.join(repoRoot, 'data/civilizations.json'), 'utf8'));
const civilizationRows = Array.isArray(civilizations) ? civilizations : (civilizations.civilizations ?? []);
for (const civilization of civilizationRows) {
  const id = civilization.id;
  if (!id) {
    failures.push('حضارة بلا معرّف في data/civilizations.json');
    continue;
  }
  const profile = path.join(designRoot, '02-civilizations/profiles', `${id}.md`);
  try {
    const profileContent = await readFile(profile, 'utf8');
    if (!profileContent.includes(id)) failures.push(`ملف حضارة لا يذكر معرّفه: ${path.relative(repoRoot, profile)}`);
  } catch {
    failures.push(`ملف الحضارة المولّد مفقود: ${path.relative(repoRoot, profile)}`);
  }
}

for (const relative of legacyDocs) {
  const filename = path.join(repoRoot, relative);
  try {
    if (!(await readFile(filename, 'utf8')).includes('[مرجعي قديم]')) {
      failures.push(`وثيقة قديمة بلا إحالة حاكمة: ${relative}`);
    }
  } catch {
    failures.push(`وثيقة قديمة مطلوبة للمسار غير موجودة: ${relative}`);
  }
}

const visual = docContent.get(path.join(designRoot, '01-visual/VISUAL_IDENTITY.md')) ?? '';
const defenses = docContent.get(path.join(designRoot, '04-world-map/ALLIANCE_DEFENSE_CONTRACT.md')) ?? '';
const research = docContent.get(path.join(designRoot, '00-governance/RESEARCH_NOTES.md')) ?? '';
if (!visual.includes('Rok2VisualTheme')) failures.push('دليل الهوية لا يربط الرموز بعميل Unreal.');
if (!defenses.includes('TransitGuard') || !defenses.includes('alliance_catapult')) failures.push('عقد دفاع التحالف لا يغطي حماية المرور والمنجنيق معاً.');
for (const requiredSource of ['w3.org', 'procedural-content-generation', 'world-partition']) {
  if (!research.includes(requiredSource)) failures.push(`سجل البحث لا يذكر المصدر المطلوب: ${requiredSource}`);
}

if (failures.length) {
  console.error('فشل فحص وثائق التصميم:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log(`✓ فحص وثائق التصميم اجتاز: ${requiredDocs.length} وثائق أساسية، ${civilizationRows.length} ملفات حضارات، و${legacyDocs.length} إحالات قديمة.`);
