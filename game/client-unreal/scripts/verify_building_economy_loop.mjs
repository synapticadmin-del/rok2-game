import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  contract: path.join(repoRoot, 'design/02-buildings/BUILD_UPGRADE_PRODUCTION_LOOP.md'),
  gameData: path.join(repoRoot, 'game/backend/src/lib/gameData.ts'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  shard: path.join(repoRoot, 'game/backend/src/do/KingdomShard.ts'),
  shop: path.join(repoRoot, 'game/backend/src/do/sim/shop.ts'),
  types: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Types.h'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  cityWidgetHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2CityWidget.h'),
  cityWidgetSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2CityWidget.cpp'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['contract', /البناء والترقية والإنتاج/, 'عقد حلقة البناء والإنتاج'],
  ['contract', /الجواهر/, 'قاعدة الإنهاء بالجواهر'],
  ['gameData', /buildingUpgradeDurationSec/, 'حساب مدة الترقية الموحد'],
  ['gameData', /resourceProductionRates/, 'حساب معدلات الإنتاج الموحد'],
  ['shop', /gemFinishCost/, 'تسعير الإنهاء من كتالوج التسريعات'],
  ['router', /getActiveQueues/, 'قراءة الطابور السلطوية'],
  ['router', /getProductionStatus/, 'حالة الإنتاج السلطوية'],
  ['router', /Another building upgrade is already running/, 'قفل ترقية مبانٍ واحدة'],
  ['router', /buildingUpgradeDurationSec/, 'مدة بناء معتمدة من الخادم'],
  ['router', /\/v1\/city\/collect/, 'مسار تحصيل الإنتاج'],
  ['router', /finishWithGems/, 'إتمام طابور بالجواهر'],
  ['router', /gemsSpent/, 'تعويض جواهر محدد عند الفشل'],
  ['router', /food=food\+\?/, 'تعويض موارد طابور البناء عند الفشل'],
  ['shard', /\/queue\/list/, 'استعلام طابور اللاعب داخل الشارد'],
  ['shard', /\$\{queueType\}_queue_busy/, 'منع طابور موازٍ من النوع نفسه'],
  ['shard', /queue\.type === queueType/, 'مطابقة قفل الطابور لنوعه'],
  ['types', /RemainingSeconds/, 'زمن الطابور المتبقي في العميل'],
  ['types', /FinishCostGems/, 'سعر الإنهاء في العميل'],
  ['types', /int32 Gems/, 'رصيد عملة التسريع في العميل'],
  ['apiHeader', /FinishQueueWithGems/, 'أمر إتمام الجواهر'],
  ['apiHeader', /UseSpeedupItem/, 'أمر عنصر التسريع'],
  ['apiHeader', /CollectCityProduction/, 'أمر تحصيل المدينة'],
  ['apiSource', /activeQueues/, 'تحليل حالة طابور الخادم'],
  ['apiSource', /ratesPerHour/, 'تحليل معدلات الإنتاج السلطوية'],
  ['cityWidgetHeader', /CollectButton/, 'زر تحصيل الإنتاج'],
  ['cityWidgetSource', /OnCollectClicked/, 'ربط زر التحصيل'],
  ['cityWidgetSource', /إنهاء %d ج/, 'إظهار سعر إنهاء الطابور بالجواهر'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد حلقة البناء والاقتصاد:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد الحلقة الاقتصادية اجتاز: بناء زمني، إنتاج محصّل، تسريع بجواهر، وتعويضات متصلة.');
