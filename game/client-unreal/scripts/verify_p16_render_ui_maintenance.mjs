#!/usr/bin/env node
/**
 * P16-T1 — حارس صيانة العرض: الكاميرا، الألوان، والواجهة.
 *
 * يمنع انحدار الأعطال التي أُصلحت في هذه الدفعة. كل فحص يقابل عطلاً حقيقياً
 * ظهر في APK ولم يكن يُلتقط بأي اختبار:
 *
 *   1. مواد المشروع (M_Rok2Base / M_Rok2Unlit) موجودة كـ .uasset، والكود يحمّلها
 *      لا يحمّل مادة محرك بلا بارامترات. هذا سبب «القلعة والسور بلا ألوان».
 *   2. لا SetVectorParameterValue فوق CreateAndSetMaterialInstanceDynamic —
 *      مواد /Engine/BasicShapes لا تملك بارامتر "Color" فالنداء يُهمل بصمت.
 *   3. إعدادات التحزيم في DefaultGame.ini لا DefaultEngine.ini (config=Game)،
 *      ومجلدات LoadObject مذكورة في DirectoriesToAlwaysCook.
 *   4. ثبات إطار الكاميرا: MaintainYFOV + إعادة حساب FOV حسب نسبة الشاشة.
 *   5. سلم أحجام النص يعمل: كل حالة في SizeOf تنتهي بـ break.
 *   6. أصول الواجهة المؤلَّفة مستوردة فعلاً (PNG بلا .uasset = غير موجودة للعبة).
 *   7. تحميل مجسمات kaykit غير محجوز بـ WITH_EDITOR (وإلا مكعبات في APK).
 *   8. الحواف الآمنة مطبَّقة على عناصر HUD الملتصقة بحدود الشاشة.
 *   9. لا كيانات عالم مخترعة في العميل — الخادم هو السلطة.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const CLIENT = path.resolve(here, '..');

const checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}
function read(rel) {
  return fs.readFileSync(path.join(CLIENT, rel), 'utf8');
}
/** يقرأ الكود بلا تعليقات — الفحوص تسأل عن سلوك لا عن شرحٍ يذكر الاسم القديم. */
function readCode(rel) {
  return read(rel)
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/^[ \t]*\/\/.*$/gm, '')
    .replace(/[ \t]\/\/.*$/gm, '');
}
function exists(rel) {
  return fs.existsSync(path.join(CLIENT, rel));
}
function countAssets(rel, ext) {
  const dir = path.join(CLIENT, rel);
  if (!fs.existsSync(dir)) return 0;
  return fs.readdirSync(dir).filter((f) => f.toLowerCase().endsWith(ext)).length;
}

// ---------------------------------------------------------------------------
// 1 + 2. المواد: مصدر كل ألوان القلعة والسور والمباني
// ---------------------------------------------------------------------------
expect('مادة المشروع المضاءة M_Rok2Base مستوردة', exists('Content/Art/Materials/M_Rok2Base.uasset'));
expect('مادة Unlit M_Rok2Unlit مستوردة', exists('Content/Art/Materials/M_Rok2Unlit.uasset'));
expect('سكربت توليد المواد موجود', exists('create_materials.py'));

const procedural = readCode('Source/Rok2/Private/Rok2ProceduralAssets.cpp');
expect(
  'URok2ProceduralAssets يحمّل مادة المشروع لا مادة محرك',
  procedural.includes('/Game/Art/Materials/M_Rok2Base')
    && procedural.includes('/Game/Art/Materials/M_Rok2Unlit')
);
expect(
  'المسار القديم DefaultLitMaterial (غير موجود في UE 5.4) أُزيل',
  !procedural.includes('DefaultLitMaterial')
);
expect(
  'MakeTintedMaterialOn يفرض والداً يملك البارامتر',
  procedural.includes('MakeTintedMaterialOn') && procedural.includes('UMaterialInstanceDynamic::Create(BaseMaterial, Component)')
);

const colorSites = [
  'Source/Rok2/Private/Rok2BuildingActor.cpp',
  'Source/Rok2/Private/Rok2HexWallActor.cpp',
  'Source/Rok2/Private/Rok2WorldRenderer.cpp',
  'Source/Rok2/Private/Rok2CityLayoutActor.cpp',
];
for (const rel of colorSites) {
  const src = readCode(rel);
  expect(
    `${path.basename(rel)} لا يستخدم CreateAndSetMaterialInstanceDynamic للتلوين`,
    !src.includes('CreateAndSetMaterialInstanceDynamic')
  );
  expect(
    `${path.basename(rel)} يلوّن عبر MakeTintedMaterialOn`,
    src.includes('MakeTintedMaterialOn')
  );
}

// ---------------------------------------------------------------------------
// 3. التحزيم
// ---------------------------------------------------------------------------
const defaultGame = exists('Config/DefaultGame.ini') ? read('Config/DefaultGame.ini') : '';
const defaultEngine = read('Config/DefaultEngine.ini');

expect('Config/DefaultGame.ini موجود', defaultGame.length > 0);
expect(
  'إعدادات التحزيم في DefaultGame.ini (UCLASS config=Game)',
  defaultGame.includes('[/Script/UnrealEd.ProjectPackagingSettings]')
    && defaultGame.includes('+MapsToCook=(FilePath="/Game/Maps/Rok2Main")')
);
expect(
  'قسم التحزيم لم يعد في DefaultEngine.ini حيث كان يُتجاهل',
  !defaultEngine.includes('[/Script/UnrealEd.ProjectPackagingSettings]')
);
for (const dir of [
  '/Game/Art/Materials',
  '/Game/Art/UIIcons',
  '/Game/Art/UIButtons',
  '/Game/Art/CityBuildingIcons',
  '/Game/Art/kaykit',
  '/Engine/BasicShapes',
]) {
  expect(
    `DirectoriesToAlwaysCook يضم ${dir}`,
    defaultGame.includes(`+DirectoriesToAlwaysCook=(Path="${dir}")`)
  );
}

// ---------------------------------------------------------------------------
// 4. الكاميرا
// ---------------------------------------------------------------------------
const cameraCpp = readCode('Source/Rok2/Private/Rok2IsometricCamera.cpp');
const cameraH = read('Source/Rok2/Public/Rok2IsometricCamera.h');

expect(
  'الكاميرا تفرض MaintainYFOV صراحةً على المكوّن',
  cameraCpp.includes('bOverrideAspectRatioAxisConstraint = true')
    && cameraCpp.includes('AspectRatio_MaintainYFOV')
);
expect(
  'المشروع يثبّت نفس القيد على اللاعب المحلي',
  defaultEngine.includes('[/Script/Engine.LocalPlayer]')
    && defaultEngine.includes('AspectRatioAxisConstraint=AspectRatio_MaintainYFOV')
);
expect(
  'FOV مقيس على نسبة مرجعية ثابتة لا على نسبة النافذة',
  cameraCpp.includes('ApplyProjectionSettings')
    && cameraCpp.includes('Camera->AspectRatio = FMath::Max(0.1f, ReferenceAspectRatio)')
    && cameraCpp.includes('Camera->bConstrainAspectRatio = false')
);
expect(
  'إعدادات العرض تُعاد عند BeginPlay لا في المُنشئ وحده',
  /void ARok2IsometricCamera::BeginPlay[\s\S]{0,200}ApplyProjectionSettings\(\);/.test(cameraCpp)
);
expect('النسبة المرجعية معرّفة كخاصية قابلة للضبط', cameraH.includes('ReferenceAspectRatio'));
expect(
  'إعدادات مقياس الواجهة للهاتف موجودة',
  defaultEngine.includes('[/Script/Engine.UserInterfaceSettings]')
    && defaultEngine.includes('UIScaleRule=ShortestSide')
);

// ---------------------------------------------------------------------------
// 5. سلم أحجام النص
// ---------------------------------------------------------------------------
const typography = readCode('Source/Rok2/Private/Rok2Typography.cpp');
const sizeOfBody = typography.slice(
  typography.indexOf('float URok2Typography::SizeOf'),
  typography.indexOf('FName URok2Typography::WeightOf')
);
const roleCases = sizeOfBody.match(/case ERok2TextRole::\w+:\s+Raw = [^;]+;\s*break;/g) || [];
const bareAssignments = sizeOfBody.match(/Raw = [^;]+;(?!\s*break;)/g) || [];
expect('SizeOf يعرّف كل الأدوار غير الافتراضية مع break', roleCases.length >= 11, `${roleCases.length}`);
expect(
  'لا إسناد في SizeOf بلا break (وإلا سقطت الحالات كلها إلى Body)',
  bareAssignments.length === 1,
  `${bareAssignments.length} إسناد بلا break (المتوقع 1: التهيئة الأولية)`
);

// ---------------------------------------------------------------------------
// 6. أصول الواجهة المؤلَّفة مستوردة
// ---------------------------------------------------------------------------
const importedFolders = [
  ['Content/Art/UIIcons', 20],
  ['Content/Art/UIButtons', 4],
  ['Content/Art/CityBuildingIcons', 58],
  ['Content/Art/CivIcons', 6],
  ['Content/Art/CivBackgrounds', 6],
  ['Content/Art/WorldMapIcons', 38],
];
for (const [dir, minimum] of importedFolders) {
  const uassets = countAssets(dir, '.uasset');
  expect(`${dir}: ${minimum} أصل مستورد`, uassets >= minimum, `${uassets}/${minimum}`);
}

const importer = read('import_assets.py');
for (const job of ['Art/UIIcons', 'Art/UIButtons', 'Art/CityBuildingIcons', 'Art/CivIcons', 'Art/CivBackgrounds']) {
  expect(`المستورد يعرف ${job}`, importer.includes(`("${job}"`));
}
expect(
  'المستورد يتعرّف على شجرة glTF فلا يعيد الاستيراد ويُسقط المحرك',
  importer.includes('already_imported') && importer.includes('StaticMeshes')
);
expect(
  'الاستيراد ملف-ملف مع التقاط الاستثناء',
  importer.includes('import_asset_tasks([t])') && importer.includes('except Exception')
);
expect('ImportAssets.bat يولّد المواد قبل الاستيراد', read('ImportAssets.bat').includes('create_materials.py'));

// ---------------------------------------------------------------------------
// 7. مجسمات kaykit تُحمّل في بناء مُطبَّق
// ---------------------------------------------------------------------------
const artAssets = readCode('Source/Rok2/Private/Rok2ArtAssets.cpp');
const loadMeshBody = artAssets.slice(
  artAssets.indexOf('UStaticMesh* URok2ArtAssets::LoadMesh'),
  artAssets.indexOf('FString URok2ArtAssets::GetImportedUiIconAssetPath')
);
expect(
  'LoadMesh غير محجوز بـ WITH_EDITOR (وإلا مكعبات placeholder في APK)',
  !loadMeshBody.includes('#if WITH_EDITOR')
);
expect(
  'LoadMesh يبحث في مرشحات متعددة (ملفات KayKit متعددة العقد تُسمّى بلاحقة لون)',
  artAssets.includes('MeshPackageCandidates')
    && artAssets.includes('/StaticMeshes/%s.%s')
    && artAssets.includes('_blue')
);
expect(
  'مجسم القلعة مستورد فعلاً',
  exists('Content/Art/kaykit/building_castle.uasset')
    || exists('Content/Art/kaykit/building_castle/StaticMeshes/building_castle.uasset')
);
expect(
  'مجسمات KayKit متعددة العقد مستوردة بلاحقتها',
  exists('Content/Art/kaykit/building_windmill_blue.uasset')
    && exists('Content/Art/kaykit/building_tower_A_blue.uasset')
);

const project = JSON.parse(read('Rok2.uproject'));
const interchange = (project.Plugins || []).find((p) => p.Name === 'Interchange');
expect(
  'Interchange مفعّل للمحرر فقط (استيراد glTF كان يُسقط المحرك بدونه)',
  Boolean(interchange?.Enabled) && (interchange.TargetAllowList || []).includes('Editor')
);
expect(
  'Interchange مستثنى من بناء اللعبة/الأندرويد',
  (project.DisabledPlugins || []).some((p) => p.Name === 'Interchange')
);

// ---------------------------------------------------------------------------
// 8. الحواف الآمنة
// ---------------------------------------------------------------------------
const a11yH = read('Source/Rok2/Public/Rok2Accessibility.h');
const a11yCpp = readCode('Source/Rok2/Private/Rok2Accessibility.cpp');
const hud = readCode('Source/Rok2/Private/Rok2HudWidget.cpp');

expect('GetSafeAreaPadding معرّفة', a11yH.includes('GetSafeAreaPadding') && a11yCpp.includes('RebuildDisplayMetrics'));
expect(
  'مقياس الواجهة لم يعد يضاعف مقياس DPI الذي يطبّقه المحرك',
  !a11yCpp.includes('GetDPIScaleFactorAtPoint')
);
const safeUses = (hud.match(/GetSafeAreaPadding\(\)/g) || []).length;
expect('HUD يطبّق الحواف الآمنة على ثلاث مجموعات على الأقل', safeUses >= 3, `${safeUses}`);

// ---------------------------------------------------------------------------
// 9. سلطة الخادم على العالم
// ---------------------------------------------------------------------------
const world = read('Source/Rok2/Private/Rok2WorldRenderer.cpp');
expect(
  'الراسم لا يخترع عُقداً/ممرات محلية (الخادم هو السلطة)',
  !world.includes('Local preview entities')
    && world.includes('const FRok2WorldSnapshot& W = Api->GetWorldSnapshot()')
);

// ---------------------------------------------------------------------------
// لا أسطر مكررة متتالية في ملفات الواجهات (أثر سكربتات التصحيح الآلية)
// ---------------------------------------------------------------------------
for (const rel of [
  // P24-T1: `Rok2CityWidget` تقاعدت (ألواح مطوية بلا مسار إظهار).
  'Source/Rok2/Private/Rok2Surface.cpp',
  'Source/Rok2/Private/Rok2HudWidget.cpp',
  'Source/Rok2/Private/Rok2WorldRenderer.cpp',
]) {
  const lines = read(rel).split(/\r?\n/);
  let dups = 0;
  for (let i = 1; i < lines.length; i += 1) {
    if (lines[i].trim().length > 0 && lines[i] === lines[i - 1]) dups += 1;
  }
  expect(`${path.basename(rel)} بلا أسطر مكررة متتالية`, dups === 0, `${dups}`);
}

// ---------------------------------------------------------------------------
const failed = checks.filter((c) => !c.ok);
for (const c of checks) {
  console.log(`${c.ok ? 'PASS' : 'FAIL'} — ${c.name}${c.detail ? ` (${c.detail})` : ''}`);
}
console.log(`\nP16-T1: ${checks.length - failed.length} passed, ${failed.length} failed`);
process.exit(failed.length ? 1 : 0);
