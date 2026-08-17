#!/usr/bin/env node
/**
 * verify_p18_t6_settings.mjs — P18-T6: شاشة الإعدادات.
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. `URok2Accessibility::SetUiScale` و`SetHighContrast` معرّفتان منذ P7-T7
 *      **بلا أي مستدعٍ في المشروع** — فمقياس الواجهة 1.0 دائماً والتباين مطفأ
 *      دائماً، وكل ما بُني عليهما (`ScaledSize`/`ScaledIconSize`/`GetScaledPx`/
 *      `AccessibleTextFor` في عشرات المواضع) يعمل على قيمة ثابتة.
 *   2. `MasterVolume` و`bAudioEnabled` حقلان عامّان في مدير الصوت لا شاشة
 *      تلمسهما، ولا فصل بين الموسيقى والمؤثرات — فلا سبيل لخفض الموسيقى وحدها
 *      وهي أول ما يخفضه اللاعب.
 *   3. لا طبقة تخزين للإعدادات: أي تغيير (لو أمكن) يضيع بإغلاق التطبيق.
 *
 * فحص بنيوي: لا يحتاج بناء UE5 ولا خادماً يعمل.
 *
 * Usage: node scripts/verify_p18_t6_settings.mjs
 */

import { readFileSync, existsSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const here = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(here, '..', 'Source', 'Rok2');

let passed = 0;
let failed = 0;

function check(name, condition, detail = '') {
  if (condition) {
    console.log(`  ✅ ${name}`);
    passed++;
  } else {
    console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`);
    failed++;
  }
}

const readOr = (p) => (existsSync(p) ? readFileSync(p, 'utf8') : '');
const pub = (f) => readOr(join(ROOT, 'Public', f));
const priv = (f) => readOr(join(ROOT, 'Private', f));
const strip = (s) => s.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');

function fnBody(src, signature) {
  const at = src.indexOf(signature);
  if (at < 0) return '';
  const open = src.indexOf('{', at);
  if (open < 0) return '';
  let depth = 0;
  for (let i = open; i < src.length; i++) {
    if (src[i] === '{') depth++;
    else if (src[i] === '}') {
      depth--;
      if (depth === 0) return src.slice(open, i + 1);
    }
  }
  return '';
}

const setH = pub('Rok2SettingsWidget.h');
const setC = strip(priv('Rok2SettingsWidget.cpp'));
const accH = pub('Rok2Accessibility.h');
const accC = strip(priv('Rok2Accessibility.cpp'));
const audH = pub('Rok2AudioManager.h');
const audC = strip(priv('Rok2AudioManager.cpp'));
const hudH = pub('Rok2HudWidget.h');
const hudC = strip(priv('Rok2HudWidget.cpp'));
const gmH = pub('Rok2GameMode.h');
const gmC = strip(priv('Rok2GameMode.cpp'));

// ---------------------------------------------------------------------------
console.log('\n[1] الشاشة موجودة وطبقة قابلة للتسريح');
// ---------------------------------------------------------------------------
check('Rok2SettingsWidget.h موجود', setH.length > 0);
check('Rok2SettingsWidget.cpp موجود', setC.length > 0);
check('يرث IRok2DismissibleLayer',
  /class\s+ROK2_API\s+URok2SettingsWidget\s*:\s*public UUserWidget,\s*public IRok2DismissibleLayer/.test(setH),
  'زر الرجوع (P18-T5) يجب أن يغلقها كبقية اللوحات');
check('ينفّذ DismissLayer', /virtual void DismissLayer\(\) override/.test(setH));

// ---------------------------------------------------------------------------
console.log('\n[2] الدوال الميتة صار لها مستدعٍ');
// ---------------------------------------------------------------------------
check('SetUiScale لها مستدعٍ في الشاشة',
  setC.includes('SetUiScale('),
  'كانت معرّفة في P7-T7 بلا مستدعٍ واحد في المشروع');
check('SetHighContrast لها مستدعٍ في الشاشة', setC.includes('SetHighContrast('));
check('bAudioEnabled يُضبط من الشاشة', setC.includes('bAudioEnabled'));
check('الشاشة تقرأ الحالة الحيّة لا قيمة مفترضة',
  setC.includes('A11y->GetUiScale()') && setC.includes('A11y->IsHighContrast()'));

// ---------------------------------------------------------------------------
console.log('\n[3] مستويا الصوت منفصلان ويسريان فوراً');
// ---------------------------------------------------------------------------
check('MusicVolume معرّف', /float MusicVolume\s*=/.test(audH));
check('SfxVolume معرّف', /float SfxVolume\s*=/.test(audH));
check('SetMusicVolume معلنة', audH.includes('void SetMusicVolume(float NewVolume);'));
check('SetSfxVolume معلنة', audH.includes('void SetSfxVolume(float NewVolume);'));
check('المستوى الفعلي يُضرب في العام (MasterVolume يبقى ذا معنى)',
  audH.includes('GetEffectiveMusicVolume') && audH.includes('MasterVolume * MusicVolume'));

const setMusic = fnBody(audC, 'void URok2AudioManager::SetMusicVolume');
check('تغيير الموسيقى يسري على العاملة الآن',
  setMusic.includes('SetVolumeMultiplier('),
  'بلا هذا لا يُسمع أثر الشريح حتى تُعاد الموسيقى — يخالف رد الفعل الفوري');
check('المستوى مقصوص [0..1]', /FMath::Clamp\(NewVolume, 0\.f, 1\.f\)/.test(setMusic));
check('تشغيل الموسيقى يستخدم المستوى الفعلي',
  audC.includes('SpawnSound2D(World, Music, GetEffectiveMusicVolume()'));
check('المؤثرات تستخدم المستوى الفعلي',
  fnBody(audC, 'void URok2AudioManager::PlaySfx').includes('GetEffectiveSfxVolume()'));
check('همس الحضارة كذلك على مستوى المؤثرات',
  audC.includes('GetEffectiveSfxVolume() * 0.7f'),
  'كان يضرب في MasterVolume مباشرة فلا يتأثر بشريح المؤثرات');

// ---------------------------------------------------------------------------
console.log('\n[4] مقياس الواجهة يسري على ما هو معروض');
// ---------------------------------------------------------------------------
const setScale = fnBody(accC, 'void URok2Accessibility::SetUiScale');
check('مقياس Slate العام يُضبط',
  setScale.includes('SetApplicationScale('),
  'UiScale يُقرأ وقت البناء فقط — الودجات القائمة لا تتأثر بلا مقياس Slate');
check('محروس بـIsInitialized (لا يسقط في سياق بلا Slate)',
  setScale.includes('FSlateApplication::IsInitialized()'));
check('المقياس مقصوص بنفس حدود الشريح',
  /FMath::Clamp\(NewScale, 0\.85f, 1\.6f\)/.test(setScale));
check('حدود الشريح تطابق حدود الـsetter',
  /UiScaleMin\s*=\s*0\.85f/.test(setC) && /UiScaleMax\s*=\s*1\.6f/.test(setC),
  'شريح يسمح بما يقصّه الـsetter يُري اللاعب قيمة لا تُطبَّق');
check('التغيير يُبثّ للمستمعين', setScale.includes('OnAccessibilityChanged.Broadcast()'));

// ---------------------------------------------------------------------------
console.log('\n[5] الحفظ والاستعادة');
// ---------------------------------------------------------------------------
const saveH = pub('Rok2SettingsSaveGame.h');
check('Rok2SettingsSaveGame.h موجود', saveH.length > 0);
check('يرث USaveGame', /class\s+ROK2_API\s+URok2SettingsSaveGame\s*:\s*public USaveGame/.test(saveH));
check('إصدار بنية للترحيل', /int32 SchemaVersion\s*=\s*1;/.test(saveH));
for (const field of ['MusicVolume', 'SfxVolume', 'UiScale', 'bHighContrast']) {
  check(`الحفظ يشمل ${field}`, new RegExp(`UPROPERTY\\(SaveGame[^)]*\\)\\s*\\n\\s*\\w+ ${field}`).test(saveH));
}
check('لا معرّف لاعب في الحفظ (تفضيلات جهاز لا حالة لعب)',
  !/PlayerId/.test(saveH),
  'تخطيط المدينة يُربط بمعرّف لاعب لأنه حالة لعب؛ الصوت والمقياس ليسا كذلك');

check('LoadAndApplySavedSettings معلنة', accH.includes('void LoadAndApplySavedSettings();'));
check('SaveSettings معلنة', accH.includes('void SaveSettings() const;'));
check('اسم الفتحة ثابتٌ مُسمّى', accH.includes('static const TCHAR* SettingsSlotName;'));

const loadBody = fnBody(accC, 'void URok2Accessibility::LoadAndApplySavedSettings');
check('الاستعادة تقرأ من الفتحة', loadBody.includes('LoadGameFromSlot(SettingsSlotName'));
check('الاستعادة تتحقق من إصدار البنية', loadBody.includes('SchemaVersion != 1'));
check('الاستعادة تمرّ بالـsetters لا بإسناد الحقول',
  loadBody.includes('SetUiScale(Save->UiScale)') && loadBody.includes('SetHighContrast(Save->bHighContrast)'),
  'الإسناد المباشر يحمّل القيم بلا أن يسري مقياس Slate ولا الصوت العامل');
check('الاستعادة تعيد مستويي الصوت',
  loadBody.includes('SetMusicVolume(Save->MusicVolume)') && loadBody.includes('SetSfxVolume(Save->SfxVolume)'));
check('غياب الحفظ لا يكتب شيئاً (يبقى على الافتراضيات)',
  /if \(!Save \|\| Save->SchemaVersion != 1\)\s*\{[\s\S]{0,120}return;/.test(loadBody));

const saveBody = fnBody(accC, 'void URok2Accessibility::SaveSettings');
check('الكتابة إلى الفتحة نفسها', saveBody.includes('SaveGameToSlot(Save, SettingsSlotName'));
check('الكتابة تأخذ الصوت من مديره لا من نسخة محلية',
  saveBody.includes('Audio->MusicVolume') && saveBody.includes('Audio->SfxVolume'),
  'نسخة ثانية للقيمة تصبح مصدر حقيقة ينحرف');

// ---------------------------------------------------------------------------
console.log('\n[6] الاستعادة تسبق بناء أي ودجة');
// ---------------------------------------------------------------------------
const beginPlay = fnBody(gmC, 'void ARok2GameMode::BeginPlay');
check('BeginPlay يستعيد الإعدادات', beginPlay.includes('LoadAndApplySavedSettings()'));
const iLoad = beginPlay.indexOf('LoadAndApplySavedSettings');
const iBoot = beginPlay.indexOf('URok2BootWidget::StaticClass');
check('الاستعادة قبل إنشاء شاشة الدخول',
  iLoad >= 0 && iBoot >= 0 && iLoad < iBoot,
  'المقياس يُقرأ وقت البناء — استعادته بعد الودجات تتركها على 1.0 حتى إعادة البناء');

// ---------------------------------------------------------------------------
console.log('\n[7] سلسلة الوصول: زر → مفوَّض → معالج → شاشة');
// ---------------------------------------------------------------------------
check('الـHUD يعلن مفوَّض الإعدادات', /FOnHudAction OnSettingsAction/.test(hudH));
check('زر في الشريط العلوي', hudC.includes('SettingsBtn'));
check('الزر يبثّ المفوَّض',
  hudC.includes('&URok2HudWidget::OnSettingsClickedHandler')
  && hudC.includes('OnSettingsAction.Broadcast()'));
check('GameMode يربط المفوَّض',
  gmC.includes('OnSettingsAction.AddDynamic(this, &ARok2GameMode::HandleSettingsAction)'));
check('GameMode يملك الشاشة', /URok2SettingsWidget\* SettingsWidget/.test(gmH));

const handler = fnBody(gmC, 'void ARok2GameMode::HandleSettingsAction');
check('إنشاء كسول (مرة واحدة)', handler.includes('if (!SettingsWidget)'));
check('لا إضافة مزدوجة للمنفذ', handler.includes('!SettingsWidget->IsInViewport()'));
check('ترتيب فوق اللوحات وتحت الإرشاد',
  /AddToViewport\(58\)/.test(handler),
  'اللوحات 50 · الإرشاد 60 — الإعدادات تعلو ما تضبط شكله ولا تحجب بطاقة الإرشاد');
check('صوت فتح لوحة', handler.includes('ERok2AudioType::UiPanelOpen'));

// ---------------------------------------------------------------------------
console.log('\n[8] كل تغيير يسري ويُحفظ (لا حالة معلّقة)');
// ---------------------------------------------------------------------------
for (const [fn, label] of [
  ['void URok2SettingsWidget::OnMusicVolumeChanged', 'الموسيقى'],
  ['void URok2SettingsWidget::OnSfxVolumeChanged', 'المؤثرات'],
  ['void URok2SettingsWidget::OnUiScaleChanged', 'المقياس'],
  ['void URok2SettingsWidget::OnHighContrastChanged', 'التباين'],
]) {
  const body = fnBody(setC, fn);
  check(`${label}: يُحفظ لحظة التغيير`, body.includes('SaveSettings()'));
}
check('لا زر «تطبيق» ولا حالة معلّقة',
  !/OnApplyClicked|PendingSettings/.test(setC),
  'ضبط الصوت والحجم يحتاج سماعاً ورؤية فورية');
check('ضبط المؤثرات يُسمع نموذجاً بالمستوى الجديد',
  fnBody(setC, 'void URok2SettingsWidget::OnSfxVolumeChanged').includes('PlaySfx('),
  'بلا نموذج مسموع يضبط اللاعب رقماً بلا مرجع');
const audioToggle = fnBody(setC, 'void URok2SettingsWidget::OnAudioEnabledChanged');
check('كتم الصوت يوقف الموسيقى العاملة',
  audioToggle.includes('StopMusic()') && audioToggle.includes('PlayMusic()'));

const reset = fnBody(setC, 'void URok2SettingsWidget::OnResetClicked');
check('إعادة الافتراضيات تُعيد الأربعة',
  reset.includes('SetMusicVolume(1.f)') && reset.includes('SetSfxVolume(1.f)')
  && reset.includes('SetUiScale(1.f)') && reset.includes('SetHighContrast(false)'));
check('إعادة الافتراضيات تُزامن الودجات',
  reset.includes('SyncFromState()'),
  'بلا مزامنة تبقى الشرائح على مواضعها القديمة بينما القيم تغيّرت');

// ---------------------------------------------------------------------------
console.log('\n[9] قيمة كل شريح مقروءة رقماً (§8.3)');
// ---------------------------------------------------------------------------
check('نص قيمة لكل شريح',
  /UTextBlock\* MusicValueText/.test(setH) && /UTextBlock\* SfxValueText/.test(setH)
  && /UTextBlock\* UiScaleValueText/.test(setH));
const updateTexts = fnBody(setC, 'void URok2SettingsWidget::UpdateValueTexts');
check('الصوت بنسبة مئوية', updateTexts.includes('%d%%'));
check('المقياس بمضاعف', updateTexts.includes('×%.2f'));
check('مربع الاختيار يحمل نصاً لا علامة صحّ وحدها',
  setC.includes('Check->AddChild(StateText)'));
check('كل صف تبديل له سطر شرح', setC.includes('HintText'));

// ---------------------------------------------------------------------------
console.log('\n[10] نظام التصميم — لا لون خام ولا نمط مفرد');
// ---------------------------------------------------------------------------
check('الأسطح من Rok2Surface', setC.includes('Rok2Surface::Panel()') && setC.includes('Rok2Surface::Card()'));
check('نمط الشريح من نظام التصميم لا افتراضي Slate',
  setC.includes('FSliderStyle') && setC.includes('Rok2Surface::ProgressTrack()')
  && setC.includes('Rok2Surface::Circle('));
check('نمط مربع الاختيار بحالاته الست',
  setC.includes('SetUncheckedImage') && setC.includes('SetCheckedImage')
  && setC.includes('SetUncheckedHoveredImage') && setC.includes('SetCheckedPressedImage'));
check('بلا FLinearColor خام', !/FLinearColor\s*\(\s*[0-9]/.test(setC));
check('بلا SetBrushColor', !setC.includes('SetBrushColor('));
check('الخطوط من URok2Typography', setC.includes('URok2Typography::ApplyFont'));
check('الفراغ من سلم Rok2Space', (setC.match(/Rok2Space::/g) || []).length >= 10);
check('حركة الدخول من المكتبة', setC.includes('URok2MotionLibrary::PlayScaleInCenter('));
check('التسريح بحركة لا إزالة مفاجئة',
  setC.includes('URok2MotionLibrary::PlayFadeOut(this)') && !/\n\tRemoveFromParent\(\);/.test(setC));
// الشاشة فيها زرّان حقيقيان («تم» و«إعادة الافتراضيات»)؛ الباقي شرائح ومربعات
// اختيار وهي `USlider`/`UCheckBox` لا `UButton`، فـ`BindPress` لا تنطبق عليها
// (ردّ فعلها اللمسي من نمطها: مقبض متحرّك وحالات ست). فالفحص على الزرّين.
check('كل زر UButton بضغطة محسوسة',
  (setC.match(/BindPress\(/g) || []).length >= 2);

check('حجاب يُلمس للإغلاق', setC.includes('SettingsBackdrop'));

// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(60));
console.log(`P18-T6 structural verification: ${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
