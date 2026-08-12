import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/04-world-map/ALLIANCE_RALLY_CONTRACT.md'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  types: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Types.h'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  marchPanel: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2MarchPanel.cpp'),
  rosterHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2AllianceRosterWidget.h'),
  rosterSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2AllianceRosterWidget.cpp'),
  rallyWidget: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2AllianceRallyWidget.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /رالي|Rally/, 'عقد الراليات'],
  ['contract', /forming.*launched|قيد التجميع/s, 'حالات دورة حياة الرالي'],
  ['router', /path === "\/v1\/alliance\/rallies" && request\.method === "GET"/, 'استعلام الراليات السلطوي'],
  ['router', /r\.alliance_id = \? AND r\.status IN \('forming', 'launched'\)/, 'عزل راليات التحالف وحالاتها'],
  ['router', /isJoined/, 'حالة عضوية اللاعب المعادة للعميل'],
  ['router', /path === "\/v1\/alliance\/rally" && request\.method === "POST"/, 'إنشاء رالي سلطوي'],
  ['router', /path === "\/v1\/alliance\/rally\/join" && request\.method === "POST"/, 'انضمام سلطوي للرالي'],
  ['types', /FRok2AllianceRally/, 'نموذج رالي واجهة العميل'],
  ['apiHeader', /LaunchAllianceRally/, 'أمر إطلاق الرالي في الواجهة'],
  ['apiHeader', /JoinAllianceRally/, 'أمر انضمام الرالي في الواجهة'],
  ['apiHeader', /FetchAllianceRallies/, 'استعلام الراليات في الواجهة'],
  ['apiHeader', /OnAllianceRalliesUpdated/, 'إشارة تحديث قائمة الراليات'],
  ['apiSource', /\/v1\/alliance\/rallies/, 'ربط عميل الاستعلام بالراليات'],
  ['apiSource', /primaryCommanderId/, 'توافق حقل قائد الرالي مع الخادم'],
  ['apiSource', /ParseAllianceRally/, 'تحويل الرالي من JSON'],
  ['marchPanel', /TargetType == TEXT\("pass"\) \|\| TargetType == TEXT\("throne"\)/, 'حصر إطلاق الرالي في الممر والعرش'],
  ['marchPanel', /LaunchAllianceRally\(TargetType, TargetId/, 'ربط لوحة المسيرة بأمر الرالي'],
  ['rosterHeader', /RallyVBox/, 'حاوية راليات في واجهة التحالف'],
  ['rosterSource', /OnAllianceRalliesUpdated/, 'ربط نافذة التحالف بالتحديث الحي'],
  ['rallyWidget', /JoinAllianceRally/, 'زر الانضمام في بطاقة الرالي'],
  ['rallyWidget', /LaunchMs/, 'عدّاد تجهيز الرالي'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد الراليات:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد الراليات اجتاز: استعلام سلطوي، إطلاق/انضمام، حالة عضوية، وعدّاد واجهة التحالف.');
