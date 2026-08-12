import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/04-world-map/RALLY_COMBAT_REPORTS_CONTRACT.md'),
  shard: path.join(repoRoot, 'game/backend/src/do/KingdomShard.ts'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  types: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Types.h'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  reportWidget: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2BattleReportWidget.cpp'),
  allianceRoster: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2AllianceRosterWidget.cpp'),
  gameMode: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2GameMode.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /تقارير.*رالي|Rally Combat Reports/s, 'عقد تقارير الراليات'],
  ['contract', /يرى عضو التحالف تقرير رالي تحالفه.*دون كشف رالي تحالف آخر/s, 'قواعد عزل تقارير التحالف'],
  ['router', /path === "\/v1\/combat\/reports" && request\.method === "GET"/, 'استعلام HTTP المصادق للتقارير'],
  ['router', /x-rok2-player/, 'تمرير هوية اللاعب من الخادم إلى الشارد'],
  ['router', /x-rok2-alliance/, 'تمرير هوية التحالف من الخادم إلى الشارد'],
  ['shard', /reportVisibleTo/, 'فلتر خصوصية التقارير في الشارد'],
  ['shard', /visibleReportsFor/, 'قائمة تقارير مصفاة لكل لاعب'],
  ['shard', /rally\.participants.*playerId === playerId/s, 'إتاحة تقرير الرالي للمشارك نفسه'],
  ['shard', /rally\.allianceId.*=== allianceId/s, 'عزل تقرير الرالي داخل التحالف'],
  ['shard', /broadcastReport/, 'بث تقرير مقيد بالرؤية'],
  ['shard', /settleRallyCombat/, 'تسوية خسائر ومشاركة الرالي سلطوياً'],
  ['shard', /distributeRallyTroops/, 'توزيع حتمي لنتيجة الرالي بين المشاركين'],
  ['types', /FRok2RallyReportParticipant/, 'نموذج مشارك تقرير الرالي'],
  ['types', /FRok2BattleReward/, 'نموذج المكافآت السلطوية'],
  ['types', /RallyParticipants/, 'حقول المشاركين في تقرير العميل'],
  ['types', /Rewards/, 'حقول المكافآت في تقرير العميل'],
  ['apiHeader', /FetchBattleReports/, 'استعلام التقارير في واجهة العميل'],
  ['apiSource', /\/v1\/combat\/reports/, 'ربط عميل Unreal بمسار التقارير'],
  ['apiSource', /ParseBattleReport/, 'تحويل التقرير الموسع من JSON'],
  ['reportWidget', /FetchBattleReports\(\)/, 'تحديث السجل عند فتح نافذة التقارير'],
  ['reportWidget', /نتيجة رالي التحالف/, 'قسم نتيجة الرالي في الواجهة'],
  ['reportWidget', /RallyParticipants/, 'عرض المشاركين في الواجهة'],
  ['reportWidget', /المكافآت السلطوية/, 'عرض مكافآت الخادم في الواجهة'],
  ['allianceRoster', /OnRallyReportsClicked/, 'نقطة فتح التقارير من نافذة التحالف'],
  ['allianceRoster', /تقارير الراليات والقتال/, 'زر تقارير الراليات في التحالف'],
  ['gameMode', /Api->FetchBattleReports\(\)/, 'تحديث السجل عند فتح تقارير الخريطة'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد تقارير الراليات:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص تقارير الراليات اجتاز: عزل سلطوي، تسوية مشاركين، مكافآت، وربط الواجهات.');
