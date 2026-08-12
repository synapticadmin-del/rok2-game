import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  migration: path.join(repoRoot, 'game/backend/migrations/0009_city_layouts.sql'),
  layoutRules: path.join(repoRoot, 'game/backend/src/lib/cityLayout.ts'),
  router: path.join(repoRoot, 'game/backend/src/http/router.ts'),
  apiHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Api.h'),
  apiSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2Api.cpp'),
  types: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2Types.h'),
  layoutActor: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2CityLayoutActor.cpp'),
  contract: path.join(repoRoot, 'design/04-world-map/CITY_LAYOUT_SYNC_CONTRACT.md'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['migration', /CREATE TABLE IF NOT EXISTS city_layouts/, 'جدول تخطيط منفصل مملوك للاعب'],
  ['migration', /player_id TEXT PRIMARY KEY/, 'ملكية تخطيط واحدة لكل لاعب'],
  ['migration', /layout_json TEXT NOT NULL/, 'حمولة التخطيط المحفوظة'],
  ['layoutRules', /validateCityLayout/, 'متحقق خادمي للتخطيط'],
  ['layoutRules', /cityLayoutRadiusForHallLevel/, 'حدود المدينة حسب مستوى القاعة'],
  ['layoutRules', /layout_city_hall_fixed/, 'حماية قاعة المدينة الثابتة'],
  ['layoutRules', /layout_building_not_owned/, 'رفض مبنى غير مملوك'],
  ['layoutRules', /layout_overlap/, 'منع تراكب البصمات'],
  ['layoutRules', /layout_outside_city_wall/, 'منع تجاوز السور'],
  ['router', /path === "\/v1\/city\/layout" && request\.method === "POST"/, 'مسار حفظ التخطيط'],
  ['router', /requirePlayer\(request, env\)/, 'هوية المالك من الجلسة'],
  ['router', /validateCityLayout\(body\.placements, buildings, city\.hall_level\)/, 'التحقق قبل التخزين'],
  ['router', /INSERT INTO city_layouts/, 'الكتابة السلطوية'],
  ['router', /layout: await getCityLayout\(env, player\.id\)/, 'إرجاع التخطيط ضمن لقطة المدينة'],
  ['types', /FRok2CityLayoutPlacement/, 'تمثيل تخطيط المدينة في Unreal'],
  ['types', /LayoutPlacements/, 'تخزين مواضع الخادم في المدينة'],
  ['apiHeader', /SaveCityLayout\(const TArray<FRok2CityLayoutPlacement>& Placements, TFunction<void\(bool\)> OnCompleted/, 'واجهة طلب حفظ التخطيط مع نتيجة سلطوية'],
  ['apiSource', /Post\(TEXT\("\/v1\/city\/layout"\)/, 'إرسال طلب التخطيط'],
  ['apiSource', /SetArrayField\(TEXT\("placements"\)/, 'تسلسل جميع المواضع'],
  ['apiSource', /تم حفظ تخطيط القلعة/, 'تنبيه نجاح المزامنة'],
  ['apiSource', /تعذّر حفظ التخطيط/, 'تنبيه رفض المزامنة'],
  ['layoutActor', /AuthoritativeCity\.LayoutPlacements\.Num\(\) > 0/, 'تفضيل نسخة الخادم عند إعادة البناء'],
  ['layoutActor', /SavedPlacements = LoadLocalLayout\(\)/, 'احتياطي محلي انتقالي'],
  ['layoutActor', /Api->SaveCityLayout\(Payload, \[WeakThis, Placements\]\(bool bAccepted\)/, 'ربط محرر القلعة بالحفظ السلطوي'],
  ['layoutActor', /if \(bAccepted && WeakThis\.IsValid\(\)\)\s*\{\s*WeakThis->SaveAcceptedLayoutLocally\(Placements\);/, 'لا يحفظ المحرر محلياً إلا بعد قبول الخادم'],
  ['layoutActor', /void ARok2CityLayoutActor::SaveAcceptedLayoutLocally/, 'الحفظ المحلي معزول لمسار القبول فقط'],
  ['layoutActor', /!OwnedLevel \|\| \*OwnedLevel <= 0/, 'إخفاء المباني غير المملوكة من الحمولة'],
  ['contract', /تراكب/, 'توثيق منع التراكب'],
  ['contract', /جهاز جديد أو إعادة تسجيل دخول/, 'توثيق المزامنة بين الأجهزة'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد مزامنة تخطيط القلعة:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص مزامنة تخطيط القلعة اجتاز: الملكية والبصمات والسور والمحرر والنسخة السلطوية متصلة.');
