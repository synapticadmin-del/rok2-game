import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/04-world-map/MARCH_REDIRECT_CONTRACT.md'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  shard: path.join(repoRoot, 'game/backend/src/do/KingdomShard.ts'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  panelHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2MarchPanel.h'),
  panelSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2MarchPanel.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /إعادة توجيه المسيرات/, 'عقد إعادة توجيه المسيرات'],
  ['contract', /لا يمكن تحويل مسيرة رالي موحدة/, 'حظر إعادة توجيه الرالي في العقد'],
  ['router', /redirectMatch = path\.match/ , 'مسار HTTP مصادق لإعادة التوجيه'],
  ['router', /playerId: player\.id/, 'هوية اللاعب من الجلسة لا جسم الطلب'],
  ['router', /marchId: decodeURIComponent\(redirectMatch\[1\]\)/, 'معرف المسيرة من الرابط'],
  ['shard', /private async redirectMarch/, 'قرار إعادة التوجيه داخل الشارد'],
  ['shard', /march\.ownerPlayerId !== playerId/, 'فرض ملكية المسيرة'],
  ['shard', /march\.state !== "moving"/, 'قصر التحويل على المسيرات المتحركة'],
  ['shard', /rally_march_cannot_redirect/, 'حظر مسيرة الرالي الموحدة'],
  ['shard', /const elapsedRatio = Math\.max\(0, Math\.min\(1, \(now - march\.startMs\)/, 'حساب نقطة الانعطاف من وقت الخادم'],
  ['shard', /march\.etaMs = now \+ marchDurationMs\(/, 'إعادة حساب وقت الوصول'],
  ['shard', /type: "march_redirected"/, 'بث المسيرة المعاد توجيهها'],
  ['apiHeader', /RedirectMarch\(/, 'أمر إعادة التوجيه في واجهة الشبكة'],
  ['apiSource', /TEXT\("rally"\)/, 'تعريف مسيرة الرالي في محلل العميل'],
  ['panelHeader', /RedirectMarchBox/, 'قائمة اختيار المسيرة في رأس اللوحة'],
  ['panelHeader', /OnRedirectClicked/, 'معالج إعادة التوجيه في رأس اللوحة'],
  ['panelSource', /March\.OwnerPlayerId != PlayerId/, 'عرض مسيرات اللاعب فقط'],
  ['panelSource', /March\.State != TEXT\("moving"\)/, 'عرض المسيرات المتحركة فقط'],
  ['panelSource', /March\.Kind == TEXT\("rally"\)/, 'استبعاد الراليات من قائمة التحويل'],
  ['panelSource', /Api->RedirectMarch\(\*MarchId, TargetType, TargetId, ToX, ToY\)/, 'إرسال الهدف فقط عبر عميل الشبكة'],
  ['panelSource', /CanInteractWithWorldTarget\(TargetType, true\)/, 'حظر التحويل خارج طبقة التكبير التكتيكية'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد إعادة توجيه المسيرات:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد إعادة توجيه المسيرات اجتاز: تحكم سلطوي، زمن جديد، وعرض Unreal مقيد بالملكية والطبقة.');
