import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/04-world-map/MARCH_REDIRECT_NOTIFICATIONS_AND_COMBAT_CONTRACT.md'),
  shard: path.join(repoRoot, 'game/backend/src/do/KingdomShard.ts'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  panelSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2MarchPanel.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /MRN-01/, 'معيار نجاح إعادة التوجيه المرئي'],
  ['contract', /MRN-05/, 'قاعدة استقلال مسار العدو العائد'],
  ['contract', /لا توجد نافذة «تحويل أثناء القتال»/, 'قاعدة حسم سباق القتال'],
  ['shard', /type: "march_arrived", march: m/, 'بث وصول سلطوي قبل التسوية'],
  ['shard', /if \(now >= march\.etaMs\) \{\s*await this\.resolveMarchArrival\(march, now\);\s*throw new Error\("march_already_arrived"\);/s, 'حسم الوصول قبل قبول التحويل المتأخر'],
  ['shard', /march\.state !== "moving"/, 'رفض التحويل خارج الحركة'],
  ['shard', /rally_march_cannot_redirect/, 'حظر تحويل الرالي الموحد'],
  ['apiSource', /PushNotification\(TEXT\("toast"\), TEXT\("تم اعتماد إعادة التوجيه"\)/, 'تنبيه قبول إعادة التوجيه'],
  ['apiSource', /\[this\]\(const FString& Err\)/, 'معالج خطأ HTTP للتحويل'],
  ['apiSource', /Err\.Contains\(TEXT\("march_not_moving"\)\)/, 'شرح رفض مسيرة لم تعد متحركة'],
  ['apiSource', /Type == TEXT\("march_arrived"\)/, 'استقبال حدث الوصول'],
  ['apiSource', /Self->PushNotification\(TEXT\("toast"\), TEXT\("وصلت المسيرة"\)/, 'تنبيه الوصول المرئي'],
  ['apiSource', /Type == TEXT\("march_arrived"\)[\s\S]{0,900}E\.OwnerPlayerId == Self->Player\.Id/, 'قصر تنبيه الوصول على مالك المسيرة'],
  ['apiSource', /Type == TEXT\("march_returning"\) && E\.OwnerPlayerId == Self->Player\.Id/, 'تنبيه العودة للمالك فقط'],
  ['panelSource', /March\.State != TEXT\("moving"\)/, 'إخفاء المسيرات العائدة أو المشتبكة من واجهة التحويل'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

// محاكاة قرار الشارد المتسلسل: ليست بديلاً عن الشارد، بل تغطي السيناريوهات
// التي يجب أن يحافظ عليها عقد التنفيذ وقواعد المصدر أعلاه.
function decideRedirect({ state, now, etaMs, targetValid = true }) {
  if (state !== 'moving') return 'march_not_moving';
  if (now >= etaMs) return 'march_already_arrived';
  if (!targetValid) return 'invalid_redirect_target';
  return 'redirected';
}

const scenarios = [
  [{ state: 'moving', now: 999, etaMs: 1000 }, 'redirected', 'تحويل قبل الوصول يقبل'],
  [{ state: 'moving', now: 1000, etaMs: 1000 }, 'march_already_arrived', 'الوصول يحسم السباق عند التساوي'],
  [{ state: 'engaging', now: 900, etaMs: 1000 }, 'march_not_moving', 'التحويل أثناء الاشتباك مرفوض'],
  [{ state: 'returning', now: 900, etaMs: 1000 }, 'march_not_moving', 'تحويل مسيرة عائدة مرفوض'],
  [{ state: 'moving', now: 900, etaMs: 1000, targetValid: false }, 'invalid_redirect_target', 'هدف أصبح مقفلاً لا يبدل المسار'],
];
for (const [input, expected, label] of scenarios) {
  if (decideRedirect(input) !== expected) failures.push(`فشل سيناريو: ${label}`);
}

// تراجع العدو لا يدخل في مدخلات قرار تحويل المسيرة الشخصية؛ تقاطع المسارات بصري فقط.
const withEnemyRetreat = decideRedirect({ state: 'moving', now: 900, etaMs: 1000 });
if (withEnemyRetreat !== 'redirected') failures.push('فشل سيناريو: تراجع العدو غيّر قرار تحويل غير مرتبط به');

if (failures.length) {
  console.error('فشل فحص تنبيهات وتحكيم إعادة توجيه المسيرات:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ اجتاز عقد التنبيهات: قبول مرئي، وصول مملوك، رفض سلطوي عند الوصول/القتال/العودة، واستقلال تراجع العدو.');
