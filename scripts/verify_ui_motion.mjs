#!/usr/bin/env node
/**
 * verify_ui_motion.mjs — P6-T3 structural verification
 *
 * Checks that the shared UMG motion library is properly defined and wired
 * through every client screen: URok2MotionLibrary (tween engine driven by the
 * core ticker), the unified 0.25s ease-out standard from
 * 07-game-design/ui-ux-design-system.md §9, tactile button press feedback
 * (scale + click sound), bottom-sheet slide-ins, centre-opening windows,
 * bottom-popping toast cards — and that no widget keeps a local ad-hoc
 * animation loop any more.
 *
 * Static/structural test — does not require a running UE5 build or backend.
 *
 * Usage: node scripts/verify_ui_motion.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..', 'game', 'client-unreal', 'Source', 'Rok2');

let passed = 0;
let failed = 0;

function ok(name) {
  console.log(`  ✅ ${name}`);
  passed++;
}
function fail(name, detail = '') {
  console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`);
  failed++;
}
function check(name, condition, detail = '') {
  if (condition) ok(name);
  else fail(name, detail);
}

const readOr = (p) => (existsSync(p) ? readFileSync(p, 'utf8') : '');

// ---------------------------------------------------------------------------
// 1. URok2MotionLibrary — الملفات والواجهة
// ---------------------------------------------------------------------------
console.log('\n[1] URok2MotionLibrary — مكتبة الحركات المشتركة');

const motionH = join(ROOT, 'Public', 'Rok2MotionLibrary.h');
const motionCpp = join(ROOT, 'Private', 'Rok2MotionLibrary.cpp');

check('Rok2MotionLibrary.h exists', existsSync(motionH));
check('Rok2MotionLibrary.cpp exists', existsSync(motionCpp));

const mh = readOr(motionH);
const mc = readOr(motionCpp);

if (mh) {
  check('has URok2MotionLibrary UCLASS', mh.includes('class ROK2_API URok2MotionLibrary'));
  check('has Get singleton', mh.includes('static URok2MotionLibrary* Get()'));
  check('is a UObject (AddToRoot pattern)', mh.includes(': public UObject'));
  check('includes Containers/Ticker.h', mh.includes('#include "Containers/Ticker.h"'));
  check('declares FTSTicker handle', mh.includes('FTSTicker::FDelegateHandle'));
  check('has generated.h include', mh.includes('Rok2MotionLibrary.generated.h'));
}

// ---------------------------------------------------------------------------
// 2. الحركات المطلوبة في البند صريحة في الـ enum
// ---------------------------------------------------------------------------
console.log('\n[2] ERok2Motion — تغطية بنود P6-T3');

// كل بند في نص المهمة له حركة مقابلة
const requiredMotions = [
  'FadeIn',           // انتقالات شاشات (fade)
  'FadeOut',
  'SlideInBottom',    // انتقالات شاشات (slide) + Bottom Sheet
  'SlideOutBottom',
  'SlideInRight',
  'SlideInLeft',
  'ScaleInCenter',    // نوافذ تفتح من المركز
  'ScaleOutCenter',
  'ToastIn',          // بطاقات إشعارات منبثقة من الأسفل
  'ToastOut',
  'Press',            // أزرار بضغطة محسوسة
  'Release',
  'GoldFlash',        // كل تأكيد له وميض ذهبي (§1)
  'Pulse',
];
for (const m of requiredMotions) {
  check(`ERok2Motion has ${m}`, new RegExp(`\\b${m}\\b`).test(mh));
}

console.log('\n[2b] ERok2Ease — منحنيات التسهيل');
for (const e of ['Linear', 'OutCubic', 'InCubic', 'InOutCubic', 'OutQuad', 'OutBack']) {
  check(`ERok2Ease has ${e}`, new RegExp(`\\b${e}\\b`).test(mh));
}

// ---------------------------------------------------------------------------
// 3. المعيار الموحد من وثيقة UI §9 و§8.6
// ---------------------------------------------------------------------------
console.log('\n[3] Rok2MotionSpec — المعيار الموحد من وثيقة UI');

check('has Rok2MotionSpec namespace', mh.includes('namespace Rok2MotionSpec'));
check('Std = 0.25f (معيار §9 ease-out)', /Std\s*=\s*0\.25f/.test(mh));
check('Fast < 0.1f (رد فعل لمس <100ms §8.6)', /Fast\s*=\s*0\.0\d+f/.test(mh));
check('has SheetOffset (مسافة اللوحة السفلية)', /SheetOffset\s*=/.test(mh));
check('has ToastOffset (انبثاق الإشعار)', /ToastOffset\s*=/.test(mh));
check('has PressScale (مقياس الضغطة)', /PressScale\s*=/.test(mh));
check('has ScaleInFrom (مقياس فتح النافذة)', /ScaleInFrom\s*=/.test(mh));
check('has FlashDuration (الوميض الذهبي)', /FlashDuration\s*=/.test(mh));

// القيم الافتراضية في UFUNCTION حرفية (UHT لا يفهم الثوابت المُنَمّطة) ومحروسة
check('UFUNCTION defaults are literal 0.25f (UHT-safe)',
  /PlaySlideInBottom\(UWidget\* Target, float Duration = 0\.25f\)/.test(mh));
check('static_assert guards Std literal drift',
  mh.includes('static_assert(Rok2MotionSpec::Std == 0.25f'));
check('static_assert guards FlashDuration literal drift',
  mh.includes('static_assert(Rok2MotionSpec::FlashDuration == 0.45f'));
// القسم العام فقط (تصاريح UFUNCTION) — أعضاء FRok2Tween تحت protected: ولها
// أن تستخدم الثابت المُنَمّط بحرية لأن UHT لا يقرأها.
const publicSection = mh.split('protected:')[0];
check('no namespaced constant used as UFUNCTION default',
  !publicSection.includes('= Rok2MotionSpec::'),
  'UHT would parse it as 0');
check('FRok2Tween (non-UFUNCTION) may still use the spec constant',
  mh.includes('float Duration = Rok2MotionSpec::Std;'));

// ---------------------------------------------------------------------------
// 4. الواجهة العامة — دوال التشغيل
// ---------------------------------------------------------------------------
console.log('\n[4] الواجهة العامة');

const api = [
  'static void Play(',
  'static void PlayFadeIn(',
  'static void PlayFadeOut(',
  'static void PlaySlideInBottom(',
  'static void PlayScaleInCenter(',
  'static void PlayToastIn(',
  'static void PlayToastOut(',
  'static void PlayGoldFlash(',
  'static void PlayPress(',
  'static void BindPress(',
  'static void StopAll(',
  'static float DefaultDuration(',
  'static float ApplyEase(',
];
for (const fn of api) {
  check(`declares ${fn.replace('static void ', '').replace('static float ', '').replace('(', '')}`,
    mh.includes(fn));
}
check('GetActiveCount for diagnostics', mh.includes('GetActiveCount'));

// كل دالة معلنة لها تعريف في الـ cpp
console.log('\n[4b] كل دالة معلنة مُنفَّذة في الـ cpp');
const impls = [
  'URok2MotionLibrary::Play(',
  'URok2MotionLibrary::PlayFadeIn(',
  'URok2MotionLibrary::PlayFadeOut(',
  'URok2MotionLibrary::PlayInternal(',
  'URok2MotionLibrary::BeginDestroy(',
  'URok2MotionLibrary::PlaySlideInBottom(',
  'URok2MotionLibrary::PlayScaleInCenter(',
  'URok2MotionLibrary::PlayToastIn(',
  'URok2MotionLibrary::PlayToastOut(',
  'URok2MotionLibrary::PlayGoldFlash(',
  'URok2MotionLibrary::PlayPress(',
  'URok2MotionLibrary::BindPress(',
  'URok2MotionLibrary::StopAll(',
  'URok2MotionLibrary::DefaultDuration(',
  'URok2MotionLibrary::ApplyEase(',
  'URok2MotionLibrary::TickTweens(',
  'URok2MotionLibrary::ApplyTween(',
  'URok2MotionLibrary::AddTween(',
  'URok2MotionLibrary::EnsureTicker(',
  'URok2MotionLibrary::PrunePressProxies(',
];
for (const fn of impls) {
  check(`implements ${fn.replace('URok2MotionLibrary::', '').replace('(', '')}`, mc.includes(fn));
}

// ---------------------------------------------------------------------------
// 5. محرك التوين
// ---------------------------------------------------------------------------
console.log('\n[5] محرك التوين');

check('registers on core ticker', mc.includes('FTSTicker::GetCoreTicker().AddTicker'));
check('ticker delegate bound to TickTweens',
  mc.includes('&URok2MotionLibrary::TickTweens'));
check('ticker returns true (stays registered)', /return true;\s*\/\//.test(mc) || mc.includes('return true;'));
check('uses TWeakObjectPtr for targets (no dangling widget)',
  mh.includes('TWeakObjectPtr<UWidget> Target'));
check('drops tween when widget is gone', /if\s*\(!W\)\s*\{[\s\S]{0,80}RemoveAt/.test(mc));
check('animates RenderTranslation', mc.includes('SetRenderTranslation'));
check('animates RenderScale', mc.includes('SetRenderScale'));
check('animates RenderOpacity', mc.includes('SetRenderOpacity'));
check('sets centre pivot for scale motions',
  mc.includes('SetRenderTransformPivot(FVector2D(0.5f, 0.5f))'));
check('replaces conflicting tween on same widget',
  mc.includes('Tweens[i].Target == Tween.Target'));
check('removes widget after exit motion (bRemoveOnFinish)',
  mc.includes('bRemoveOnFinish') && mc.includes('RemoveFromParent()'));
check('prunes stale press proxies (no unbounded growth)',
  mc.includes('PrunePressProxies') && /PruneTimer\s*\+=/.test(mc));

console.log('\n[5b] منحنيات التسهيل منفَّذة رياضياً');
check('OutCubic implemented', /1\.f\s*-\s*FMath::Pow\(1\.f\s*-\s*T,\s*3\.f\)/.test(mc));
check('OutBack overshoot implemented', /1\.70158f/.test(mc));
check('InOutCubic implemented', mc.includes('4.f * T * T * T'));
check('ease input clamped', mc.includes('FMath::Clamp(T, 0.f, 1.f)'));

// ---------------------------------------------------------------------------
// 5c. صلابة المحرك (نتائج مراجعة مخاطر البناء)
// ---------------------------------------------------------------------------
console.log('\n[5c] صلابة المحرك');

check('unregisters ticker in BeginDestroy (no delegate on dead object)',
  mc.includes('RemoveTicker(TickerHandle)') && mc.includes('Super::BeginDestroy()'));
check('BeginDestroy declared as override in header',
  mh.includes('virtual void BeginDestroy() override;'));
check('removes tween from array BEFORE RemoveFromParent (re-entrancy safe)',
  /const bool bRemoveWidget = T\.bRemoveOnFinish;[\s\S]{0,120}Tweens\.RemoveAt\(i\);[\s\S]{0,120}RemoveFromParent/.test(mc));
check('BaseColor passed as parameter, not stamped onto Tweens.Last()',
  mc.includes('PlayInternal(Target, ERok2Motion::GoldFlash, Duration, BaseColor)') &&
  !mc.includes('Tweens.Last()'));
check('start frame drawn through ApplyTween (single source of from-state)',
  mc.includes('ApplyTween(T, ApplyEase(0.f, T.Ease))'));
check('no hand-set from-state duplicated in Play',
  !/Target->SetRenderOpacity\(0\.f\);/.test(mc),
  'from-state should come from ApplyTween only');
check('centre pivot applied via single bPivotCenter branch',
  mc.includes('if (bPivotCenter)'));
check('backwards index iteration everywhere (no invalidation)',
  !/for\s*\(\s*(?:auto|const auto)\s*&\s*\w+\s*:\s*Tweens\s*\)/.test(mc));

// ---------------------------------------------------------------------------
// 6. الضغطة المحسوسة: scale + صوت
// ---------------------------------------------------------------------------
console.log('\n[6] الضغطة المحسوسة (scale + صوت)');

check('URok2ButtonPressFx proxy class exists',
  mh.includes('class ROK2_API URok2ButtonPressFx'));
check('proxy has UFUNCTION HandlePressed', /UFUNCTION\(\)\s*void HandlePressed/.test(mh));
check('proxy has UFUNCTION HandleReleased', /UFUNCTION\(\)\s*void HandleReleased/.test(mh));
check('binds UButton::OnPressed', mc.includes('Button->OnPressed.AddDynamic'));
check('binds UButton::OnReleased', mc.includes('Button->OnReleased.AddDynamic'));
check('press plays ButtonClick sfx',
  mc.includes('ERok2AudioType::ButtonClick'));
check('includes AudioManager for the click', mc.includes('#include "Rok2AudioManager.h"'));
check('proxies kept from GC (UPROPERTY array)',
  /UPROPERTY\(Transient\)\s*TArray<UObject\*> PressProxies/.test(mh));

// ---------------------------------------------------------------------------
// 7. الوميض الذهبي للتأكيد
// ---------------------------------------------------------------------------
console.log('\n[7] الوميض الذهبي (§1: كل تأكيد له وميض ذهبي)');

check('gold palette matches #C9A227',
  /Gold\(0\.79f,\s*0\.64f,\s*0\.15f/.test(mc));
check('flash tints UBorder brush', mc.includes('Cast<UBorder>(W)') && mc.includes('SetBrushColor'));
check('flash tints UImage colour', mc.includes('Cast<UImage>(W)') && mc.includes('SetColorAndOpacity'));
check('flash takes explicit BaseColor (no UE-version-specific getter)',
  mh.includes('FLinearColor BaseColor') && !mc.includes('GetBrushColor()'));
check('flash returns to base colour (triangle envelope)',
  /Tri\s*=\s*1\.f\s*-\s*FMath::Abs/.test(mc));

// البند يطلب وميضاً فعلياً لا واجهة معطّلة: يجب وجود موضع استدعاء واحد على الأقل
const hudForFlash = readOr(join(ROOT, 'Private', 'Rok2HudWidget.cpp'));
check('gold flash actually wired to a confirmation (bell on notification)',
  hudForFlash.includes('URok2MotionLibrary::PlayGoldFlash'));
check('flash target is a UImage (real gold tint, not opacity fallback)',
  /PlayGoldFlash\(BellIcon\)/.test(hudForFlash));

// ---------------------------------------------------------------------------
// 8. ربط كل الشاشات بالمكتبة
// ---------------------------------------------------------------------------
console.log('\n[8] ربط الشاشات — كل شاشة تستخدم المكتبة');

// [ملف, [حركات متوقعة]]
const widgetWiring = [
  ['Rok2BuildingDetailWidget.cpp', ['PlaySlideInBottom', 'BindPress']],
  ['Rok2BuildMenuWidget.cpp', ['PlaySlideInBottom', 'BindPress']],
  ['Rok2HudWidget.cpp', ['PlayToastIn', 'PlayToastOut', 'BindPress']],
  ['Rok2BattleReportWidget.cpp', ['PlayScaleInCenter', 'PlayFadeIn', 'BindPress']],
  ['Rok2MarchPanel.cpp', ['PlayScaleInCenter', 'BindPress']],
  ['Rok2CommanderWidget.cpp', ['PlayFadeIn', 'BindPress']],
  ['Rok2BootWidget.cpp', ['PlayFadeIn', 'BindPress']],
  ['Rok2CityWidget.cpp', ['PlayFadeIn', 'BindPress']],
  ['Rok2ResearchWidget.cpp', ['PlayFadeIn', 'BindPress']],
  ['Rok2AllianceRosterWidget.cpp', ['PlayScaleInCenter', 'BindPress']],
];

for (const [file, motions] of widgetWiring) {
  const p = join(ROOT, 'Private', file);
  check(`${file} exists`, existsSync(p));
  if (!existsSync(p)) continue;
  const c = readFileSync(p, 'utf8');
  check(`  ${file} includes Rok2MotionLibrary.h`,
    c.includes('#include "Rok2MotionLibrary.h"'));
  for (const m of motions) {
    check(`  ${file} uses ${m}`, c.includes(`URok2MotionLibrary::${m}`));
  }
}

// أي ملف ينادي المكتبة يجب أن يضمّ ترويستها
console.log('\n[8b] كل نداء للمكتبة مصحوب بالـ include');
const allWidgetFiles = widgetWiring.map(([f]) => f);
for (const file of allWidgetFiles) {
  const p = join(ROOT, 'Private', file);
  if (!existsSync(p)) continue;
  const c = readFileSync(p, 'utf8');
  const calls = c.includes('URok2MotionLibrary::');
  const inc = c.includes('#include "Rok2MotionLibrary.h"');
  check(`  ${file}: calls ⇒ include`, !calls || inc);
}

// ---------------------------------------------------------------------------
// 9. لا حركات محلية مرتجلة باقية
// ---------------------------------------------------------------------------
console.log('\n[9] لا حركات محلية مرتجلة (كل الحركة من المكتبة)');

const detailCpp = readOr(join(ROOT, 'Private', 'Rok2BuildingDetailWidget.cpp'));
const detailH = readOr(join(ROOT, 'Public', 'Rok2BuildingDetailWidget.h'));

check('BuildingDetail no longer eases slide by hand in Tick',
  !/SlideT\s*\+=/.test(detailCpp),
  'local slide should be migrated to the library');
check('BuildingDetail local slide state removed from header',
  !detailH.includes('float SlideT') && !detailH.includes('bSlidIn'));
check('BuildingDetail slide comes from the library',
  detailCpp.includes('URok2MotionLibrary::PlaySlideInBottom'));

// §1 «لا قفزات جامدة»: لا لوحة تُزال فجأة — كلها تُسرَّح بحركة
console.log('\n[9c] التسريح بحركة (لا إزالة مفاجئة)');
const dismissFiles = [
  'Rok2BuildingDetailWidget.cpp',
  'Rok2BuildMenuWidget.cpp',
  'Rok2BattleReportWidget.cpp',
  'Rok2MarchPanel.cpp',
];
for (const file of dismissFiles) {
  const c = readOr(join(ROOT, 'Private', file));
  check(`  ${file} dismisses via PlayFadeOut`,
    c.includes('URok2MotionLibrary::PlayFadeOut(this)'));
  // لا يبقى RemoveFromParent عارياً على مستوى الدالة
  check(`  ${file} has no bare RemoveFromParent`,
    !/\n\tRemoveFromParent\(\);/.test(c));
}
const hudForTrim = readOr(join(ROOT, 'Private', 'Rok2HudWidget.cpp'));
check('  HUD toast overflow also exits with motion',
  !/ActiveToasts\[0\]\.Card->RemoveFromParent\(\)/.test(hudForTrim));

const hudCpp = readOr(join(ROOT, 'Private', 'Rok2HudWidget.cpp'));
check('HUD toast fade no longer computed per-frame by hand',
  !/E\.Card->SetRenderOpacity\(FMath::Clamp\(E\.Remaining/.test(hudCpp),
  'toast exit should use PlayToastOut');
check('HUD toast exit triggered once (bExiting guard)',
  readOr(join(ROOT, 'Public', 'Rok2HudWidget.h')).includes('bExiting'));

// لا ودجة تُحرِّك خصائص الرندر يدوياً خارج المكتبة
console.log('\n[9b] خصائص الرندر تُحرَّك من المكتبة فقط');
const renderApis = ['SetRenderTranslation', 'SetRenderScale', 'SetRenderTransformPivot'];
for (const file of allWidgetFiles) {
  const p = join(ROOT, 'Private', file);
  if (!existsSync(p)) continue;
  const c = readFileSync(p, 'utf8');
  const offenders = renderApis.filter((a) => c.includes(a));
  check(`  ${file}: no direct render-transform animation`,
    offenders.length === 0,
    offenders.join(', '));
}

// ---------------------------------------------------------------------------
// 10. توثيق ووضوح
// ---------------------------------------------------------------------------
console.log('\n[10] التوثيق والمرجعية');

check('header cites ui-ux-design-system.md', mh.includes('ui-ux-design-system.md'));
check('header explains why a tween engine (not WBP animations)',
  mh.includes('WidgetTree->ConstructWidget'));
check('header has usage examples', mh.includes('URok2MotionLibrary::PlaySlideInBottom'));
check('P6-T3 tagged in library header', mh.includes('P6-T3'));
check('P6-T3 tagged in library cpp', mc.includes('P6-T3'));

let taggedWidgets = 0;
for (const file of allWidgetFiles) {
  const p = join(ROOT, 'Private', file);
  if (existsSync(p) && readFileSync(p, 'utf8').includes('P6-T3')) taggedWidgets++;
}
check(`all ${allWidgetFiles.length} widgets carry a P6-T3 note`,
  taggedWidgets === allWidgetFiles.length,
  `${taggedWidgets}/${allWidgetFiles.length}`);

// ---------------------------------------------------------------------------
// 11. سلامة البناء (فحوص بنيوية خفيفة)
// ---------------------------------------------------------------------------
console.log('\n[11] سلامة البناء');

function balanced(src) {
  let d = 0;
  for (const ch of src) {
    if (ch === '{') d++;
    else if (ch === '}') { d--; if (d < 0) return false; }
  }
  return d === 0;
}
check('Rok2MotionLibrary.h braces balanced', balanced(mh));
check('Rok2MotionLibrary.cpp braces balanced', balanced(mc));
for (const file of allWidgetFiles) {
  const p = join(ROOT, 'Private', file);
  if (!existsSync(p)) continue;
  check(`  ${file} braces balanced`, balanced(readFileSync(p, 'utf8')));
}

// القيم الافتراضية لا تُكرَّر في التعريف (خطأ compile في C++)
const dupDefault = /URok2MotionLibrary::\w+\([^)]*=\s*[^)]*\)\s*\{/.test(mc);
check('no default args repeated in cpp definitions', !dupDefault);

// UMG متاح في وحدة البناء
const buildCs = readOr(join(ROOT, 'Rok2.Build.cs'));
check('Rok2.Build.cs depends on UMG', buildCs.includes('"UMG"'));
check('Rok2.Build.cs depends on Slate', buildCs.includes('"Slate"'));

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`P6-T3 structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
