#!/usr/bin/env node
/**
 * P17-T1 — عقد نظام التصميم: رموز موحّدة، أسطح مستديرة، وأزرار كاملة الحالات.
 *
 * يمنع انحدار ما أُصلح في هذه الدفعة:
 *
 *   1. لا لوحة ألوان محلية تعرّف قيماً خاماً — كانت ست لوحات متوازية بستة
 *      «ذهب» مختلفة، فتعديل الهوية يتطلب تعديل ستة ملفات.
 *   2. لا `FLinearColor(...)` خام في ملفات الودجات — كانت 130 قيمة مكتوبة.
 *   3. لا `SetBrushColor` على الألواح والبطاقات — الشكل يأتي من `Rok2Surface`
 *      بفرشاة مستديرة بحافة، وكانت الوحدة كلها بلا `FSlateRoundedBoxBrush`.
 *   4. لا زر بحالة `Normal` وحدها — كان 12 زراً بلا رد فعل لمس، وثلاثة بلا
 *      حالة معطّلة.
 *   5. رموز الفراغ والاستدارة موجودة ومستعملة (شبكة 8pt).
 *   6. اتجاه التدفّق العربي مطبَّق فعلاً — كانت `IsRtl()` ترجع true ولا يقرأها
 *      أحد.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const CLIENT = path.resolve(here, '..');
const PRIVATE_DIR = path.join(CLIENT, 'Source', 'Rok2', 'Private');

const checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}
function read(rel) {
  return fs.readFileSync(path.join(CLIENT, rel), 'utf8');
}
/** الكود بلا تعليقات — الفحوص تسأل عن سلوك لا عن شرحٍ يذكر الاسم القديم. */
function readCode(rel) {
  return read(rel)
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/^[ \t]*\/\/.*$/gm, '')
    .replace(/[ \t]\/\/.*$/gm, '');
}

// ملفات طبقة العرض: كل الودجات + لوحة المسيرة.
const WIDGET_FILES = fs
  .readdirSync(PRIVATE_DIR)
  .filter((f) => /^Rok2.*(Widget|Panel)\.cpp$/.test(f))
  .map((f) => `Source/Rok2/Private/${f}`);

expect('عُثر على ملفات طبقة العرض', WIDGET_FILES.length >= 14, `${WIDGET_FILES.length}`);

// ---------------------------------------------------------------------------
// 1. الرموز المشتركة موجودة وكاملة الطبقتين
// ---------------------------------------------------------------------------
const themeH = read('Source/Rok2/Public/Rok2VisualTheme.h');
const themeCpp = read('Source/Rok2/Private/Rok2VisualTheme.cpp');

for (const token of [
  'Ink', 'Panel', 'Card', 'Bar', 'Edge', 'Scrim',
  'Gold', 'Ivory', 'Muted', 'TabInactive', 'PrimaryAction', 'Success', 'Danger', 'Information',
  'GoldText', 'SuccessText', 'DangerText', 'InformationText',
  'ResourceFood', 'ResourceWood', 'ResourceStone', 'ResourceGold', 'ResourceGems', 'ResourceActionPoints',
  'RarityTier', 'CivilizationAccent',
]) {
  expect(`الرمز ${token} معرّف في Rok2Visual`, themeH.includes(`${token}(`) && themeCpp.includes(token));
}

expect(
  'طبقة نص فوق داكن منفصلة عن طبقة الحشو',
  themeCpp.includes('GGoldText') && themeCpp.includes('GDangerText'),
);
expect('سلم الفراغ على شبكة 8pt معرّف', themeH.includes('namespace Rok2Space') && themeH.includes('Huge   = 32.f'));
expect('سلم الاستدارة معرّف مع حالة الحبّة', themeH.includes('namespace Rok2Radius') && themeH.includes('Full  = 4096.f'));

// ---------------------------------------------------------------------------
// 2. مصنع الأسطح والأنماط
// ---------------------------------------------------------------------------
const surfaceH = read('Source/Rok2/Public/Rok2Surface.h');
const surfaceCpp = read('Source/Rok2/Private/Rok2Surface.cpp');

for (const fn of [
  'Panel', 'Sheet', 'Card', 'AccentCard', 'TopBar', 'Pill', 'OutlinedPill', 'Circle',
  'Scrim', 'SheetHandle', 'ProgressTrack', 'ProgressFill',
  'PrimaryButton', 'SecondaryButton', 'DangerButton', 'SuccessButton', 'GhostButton', 'TabButton', 'TintedButton',
]) {
  expect(`Rok2Surface::${fn} معرّف`, surfaceH.includes(`${fn}(`) && surfaceCpp.includes(`Rok2Surface::${fn}`));
}

expect(
  'الأسطح تستعمل الفرشاة المستديرة فعلاً (كانت غائبة عن الوحدة كلها)',
  surfaceCpp.includes('FSlateRoundedBoxBrush') && surfaceCpp.includes('SlateRoundedBoxBrush.h'),
);
expect(
  'كل نمط زر يغطي الحالات الأربع بما فيها المعطّلة',
  surfaceCpp.includes('SetDisabled') && surfaceCpp.includes('SetHovered') && surfaceCpp.includes('SetPressed'),
);
expect(
  'الضغط يزيح المحتوى (إحساس زر لا مجرد تغيّر لون)',
  surfaceCpp.includes('SetPressedPadding'),
);

// ---------------------------------------------------------------------------
// 3. لا لوحات ألوان محلية بقيم خام، ولا FLinearColor خام في الودجات
// ---------------------------------------------------------------------------
const RAW_COLOR = /FLinearColor\s*\(\s*[0-9]/;

for (const rel of WIDGET_FILES) {
  const code = readCode(rel);
  const base = path.basename(rel);

  // لوحة محلية مسموحة كأسماء مختصرة، لكن قيمها يجب أن تأتي من Rok2Visual.
  const nsMatch = code.match(/namespace\s+Rok2\w*Style\w*\s*\{[\s\S]*?\n\}/g) || [];
  for (const ns of nsMatch) {
    const rawInNs = (ns.match(/FLinearColor\s*\(\s*[0-9][^)]*\)/g) || []);
    expect(
      `${base}: اللوحة المحلية لا تعرّف لوناً خاماً`,
      rawInNs.length === 0,
      rawInNs.length ? rawInNs[0] : '',
    );
  }

  // القيم الخام خارج اللوحات: يُستثنى بناء لون من مكوّنات رمز قائم
  // (FLinearColor(COLOR_GOLD.R, ...)) لأنه اشتقاق لا قيمة جديدة.
  const rawAll = (code.match(/FLinearColor\s*\([^)]*\)/g) || [])
    .filter((m) => RAW_COLOR.test(m))
    .filter((m) => !/\.[RGBA]\b/.test(m));
  expect(`${base}: بلا FLinearColor خام`, rawAll.length === 0, rawAll.slice(0, 2).join(' | '));
}

// ---------------------------------------------------------------------------
// 4. الأسطح تأتي من المصنع لا من لون مسطّح
// ---------------------------------------------------------------------------
for (const rel of WIDGET_FILES) {
  const code = readCode(rel);
  const base = path.basename(rel);
  const setBrushColor = (code.match(/SetBrushColor\s*\(/g) || []).length;
  expect(`${base}: بلا SetBrushColor (السطح من Rok2Surface)`, setBrushColor === 0, `${setBrushColor}`);
}

const surfaceUsers = WIDGET_FILES.filter((rel) => readCode(rel).includes('Rok2Surface::'));
expect(
  'أغلب الودجات تستعمل مصنع الأسطح',
  surfaceUsers.length >= 12,
  `${surfaceUsers.length}/${WIDGET_FILES.length}`,
);

// ---------------------------------------------------------------------------
// 5. لا زر بحالة Normal وحدها
// ---------------------------------------------------------------------------
for (const rel of WIDGET_FILES) {
  const code = readCode(rel);
  const base = path.basename(rel);
  // ضبط حالة واحدة مباشرة على WidgetStyle يعني غياب البقية.
  const partial = (code.match(/WidgetStyle\.(Normal|Hovered|Pressed)\b/g) || []).length;
  expect(`${base}: لا ضبط حالة زر مفردة`, partial === 0, `${partial}`);
  const setNormalOnly = (code.match(/WidgetStyle\.SetNormal\(/g) || []).length;
  expect(`${base}: لا SetNormal معزولة`, setNormalOnly === 0, `${setNormalOnly}`);
}

// ---------------------------------------------------------------------------
// 6. الفراغ من السلم في الملفات المهاجَرة
// ---------------------------------------------------------------------------
const hud = readCode('Source/Rok2/Private/Rok2HudWidget.cpp');
expect('الـHUD يستعمل رموز الفراغ', (hud.match(/Rok2Space::/g) || []).length >= 8);

// ---------------------------------------------------------------------------
// 7. اتجاه التدفّق العربي مطبَّق فعلاً
// ---------------------------------------------------------------------------
const factory = readCode('Source/Rok2/Private/Rok2BlueprintLibrary.cpp');
expect(
  'مصنع الودجات يضبط اتجاه التدفّق من IsRtl',
  factory.includes('SetFlowDirectionPreference')
    && factory.includes('EFlowDirectionPreference::RightToLeft')
    && factory.includes('IsRtl()'),
);

// ---------------------------------------------------------------------------
// 8. حالة الخط العربي موثّقة (لا تُترك مجهولة)
// ---------------------------------------------------------------------------
const fontsReadme = read('Content/Fonts/README.md');
expect(
  'حالة النص العربي موثّقة (وجه Naskh الفرعي في خط المحرك)',
  fontsReadme.includes('NotoNaskhArabicUI') && fontsReadme.includes('مُحزَّم في APK'),
);
const typography = readCode('Source/Rok2/Private/Rok2Typography.cpp');
expect('الكود يوثّق مسار الخط الاحتياطي', typography.includes('GetDefaultFontStyle'));

// ---------------------------------------------------------------------------
const failed = checks.filter((c) => !c.ok);
for (const c of checks) {
  console.log(`${c.ok ? 'PASS' : 'FAIL'} — ${c.name}${c.detail ? ` (${c.detail})` : ''}`);
}
console.log(`\nP17-T1: ${checks.length - failed.length} passed, ${failed.length} failed`);
process.exit(failed.length ? 1 : 0);
