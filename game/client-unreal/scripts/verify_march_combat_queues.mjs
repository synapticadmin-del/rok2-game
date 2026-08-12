import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/04-world-map/MARCH_COMBAT_AND_QUEUES_CONTRACT.md'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  shard: path.join(repoRoot, 'game/backend/src/do/KingdomShard.ts'),
  types: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Types.h'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  cityWidget: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2CityWidget.cpp'),
  worldHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2WorldRenderer.h'),
  worldSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp'),
  controller: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2PlayerController.cpp'),
  marchPanel: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2MarchPanel.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /المسيرات والقتال/, 'عقد المسيرات والقتال'],
  ['contract', /التدريب.*الشفاء.*البحث/s, 'تعريف الطوابير المستقلة'],
  ['contract', /قاعة المدينة/, 'ارتباط سعة المسيرات بقاعة المدينة'],
  ['shard', /queueType}_queue_busy/, 'قفل نوع الطابور داخل الشارد'],
  ['shard', /q\.type === "train" \|\| q\.type === "heal"/, 'إتمام طوابير التدريب والشفاء'],
  ['shard', /q\.type === "research"/, 'إتمام طابور البحث'],
  ['shard', /const marchCapacity = Math\.min\(5, 1 \+ Math\.floor\(\(hallLevel - 1\) \/ 5\)\)/, 'معادلة سعة المسيرات السلطوية'],
  ['shard', /activeForPlayer >= marchCapacity/, 'منع تجاوز سعة المسيرات'],
  ['router', /\/v1\/world\/march/, 'مسار إنشاء المسيرة'],
  ['router', /grantBpXp\(env, player\.id, "march"\)/, 'تسجيل حدث المسيرة'],
  ['types', /ActiveQueues/, 'نموذج طوابير المدينة في العميل'],
  ['apiHeader', /DispatchMarch/, 'أمر إرسال مسيرة من العميل'],
  ['cityWidget', /البناء|تدريب|الشفاء|البحث/, 'عرض قنوات الطوابير في واجهة المدينة'],
  ['worldHeader', /GetMarchCapacity/, 'إظهار سعة المسيرات لواجهة العالم'],
  ['worldHeader', /CanInteractWithWorldTarget/, 'عقد تفاعل الخريطة المقيد بالطبقة'],
  ['worldSource', /CurrentZoomLayer/, 'اعتماد التفاعل على طبقة التكبير'],
  ['worldSource', /TEXT\("barbarian"\)/, 'دعم أهداف البرابرة الفعلية'],
  ['controller', /CanInteractWithWorldTarget\(FoundType, false\)/, 'حجب لوحة المسيرة للأهداف غير المرئية'],
  ['marchPanel', /المسيرات: %d \/ %d/, 'عرض سعة المسيرات للاعب'],
  ['marchPanel', /CanInteractWithWorldTarget\(TargetType, true\)/, 'حجب الإرسال المحلي خارج الطبقة التكتيكية'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد المسيرات والقتال والطوابير:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد المسيرات والقتال والطوابير اجتاز: سعة سلطوية، قنوات مستقلة، وتفاعل خريطة متدرج.');
