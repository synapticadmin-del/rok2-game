#!/usr/bin/env node
/**
 * verify_p18_t5_back_button.mjs — P18-T5: زر الرجوع (Escape + Android Back).
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. `ARok2PlayerController::OnEscape` جسمه تعليق واحد («could close UI
 *      panels») — فمفتاح Escape وزر الرجوع لا يفعلان شيئاً مهما تراكمت
 *      اللوحات.
 *   2. أربع شاشات بلا أي مسار إزالة في المشروع كله: القادة والدردشة ولوحة
 *      المسيرة (وشاشة التحالف كانت تُزال بـ`RemoveFromParent` عارياً).
 *   3. `URok2MarchPanel` كانت تُضاف بـ`AddToViewport()` بلا معامل — ZOrder = 0،
 *      أي **تحت الـHUD (20)**.
 *   4. حركة الخروج تنتهي بشفافية 0 ولا تُعاد — فأي لوحة يملكها GameMode تُفتح
 *      شفافة تماماً في المرة الثانية.
 *
 * فحص بنيوي: لا يحتاج بناء UE5 ولا خادماً يعمل.
 *
 * Usage: node scripts/verify_p18_t5_back_button.mjs
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

/** الكود بلا تعليقات — الفحص يسأل عن سلوك لا عن شرحٍ يذكر الاسم. */
function stripComments(src) {
  return src.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
}

/** جسم دالة واحدة بمطابقة أقواس متوازنة (لا regex هشّ). */
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

// ---------------------------------------------------------------------------
console.log('\n[1] عقد الطبقة القابلة للتسريح');
// ---------------------------------------------------------------------------
const layerH = pub('Rok2DismissibleLayer.h');
const layerCpp = priv('Rok2DismissibleLayer.cpp');

check('Rok2DismissibleLayer.h موجود', layerH.length > 0);
check('واجهة UINTERFACE معلنة', layerH.includes('UINTERFACE(') && layerH.includes('class URok2DismissibleLayer : public UInterface'));
check('DismissLayer دالة خالصة (كل لوحة تُلزم بتنفيذها)',
  /virtual void DismissLayer\(\)\s*=\s*0;/.test(layerH));
check('IsLayerOpen معلنة بتنفيذ افتراضي',
  /virtual bool IsLayerOpen\(\) const;/.test(layerH));
check('«مفتوحة» = في المنفذ وغير مطويّة',
  layerCpp.includes('IsInViewport()')
  && layerCpp.includes('ESlateVisibility::Collapsed')
  && layerCpp.includes('ESlateVisibility::Hidden'),
  'لوحة الحكاية تبقى في المنفذ وتُطوى فقط — الوجود ليس فتحاً');

// ---------------------------------------------------------------------------
console.log('\n[2] حلّال الطبقات يقرأ المنفذ لا سِجلاً محلياً');
// ---------------------------------------------------------------------------
const stackH = pub('Rok2UiStack.h');
const stackCpp = stripComments(priv('Rok2UiStack.cpp'));

check('Rok2UiStack.h موجود', stackH.length > 0);
check('DismissTopLayer معلنة', stackH.includes('static bool DismissTopLayer('));
check('FindTopDismissibleLayer معلنة', stackH.includes('static UUserWidget* FindTopDismissibleLayer('));
check('CountOpenLayers للتشخيص', stackH.includes('static int32 CountOpenLayers('));
check('ترتيب الـHUD ثابتٌ مُسمّى لا رقم مبثوث',
  /HudZOrder\s*=\s*20/.test(stackH));

check('الترتيب يُقرأ من UGameViewportSubsystem',
  stackCpp.includes('UGameViewportSubsystem') && stackCpp.includes('GetWidgetSlot('),
  'أي مكدّس محلي يصبح مصدر حقيقة ثانياً ينحرف عند الإغلاق من مسار آخر');
check('لا مكدّس لوحات محلي في الحلّال',
  !/TArray<[^>]*>\s*(G|s)?LayerStack/.test(stackCpp));
check('شاشة الدخول مستثناة صريحاً',
  stackCpp.includes('URok2BootWidget'),
  'إغلاق Boot يترك اللاعب بلا مسار عودة');
check('ما دون ترتيب الـHUD ليس طبقة',
  stackCpp.includes('Slot.ZOrder <= URok2UiStack::HudZOrder'));
check('الطبقة المطويّة لا تُحتسب',
  stackCpp.includes('IsLayerOpen()'));
check('الفرز تنازلي (الأعلى ترتيباً أولاً)',
  /OrderA\s*>\s*OrderB/.test(stackCpp));
check('DismissTopLayer تعيد false عند خلو الشاشة',
  /if \(!Top\) return false;/.test(stackCpp));

// ---------------------------------------------------------------------------
console.log('\n[3] كل لوحة تُنفّذ العقد وتُسرّح بحركة');
// ---------------------------------------------------------------------------
// [هيدر, صنف, ملف التنفيذ]
const LAYERS = [
  ['Rok2BuildingDetailWidget.h', 'URok2BuildingDetailWidget', 'Rok2BuildingDetailWidget.cpp'],
  ['Rok2BuildMenuWidget.h', 'URok2BuildMenuWidget', 'Rok2BuildMenuWidget.cpp'],
  ['Rok2BattleReportWidget.h', 'URok2BattleReportWidget', 'Rok2BattleReportWidget.cpp'],
  ['Rok2MarchPanel.h', 'URok2MarchPanel', 'Rok2MarchPanel.cpp'],
  ['Rok2CommanderWidget.h', 'URok2CommanderWidget', 'Rok2CommanderWidget.cpp'],
  ['Rok2AllianceRosterWidget.h', 'URok2AllianceRosterWidget', 'Rok2AllianceRosterWidget.cpp'],
  ['Rok2CivInfoWidget.h', 'URok2CivInfoWidget', 'Rok2CivInfoWidget.cpp'],
  ['Rok2ChatWidget.h', 'URok2ChatWidget', 'Rok2ChatWidget.cpp'],
  ['Rok2SeasonStoryWidget.h', 'URok2SeasonStoryWidget', 'Rok2SeasonStoryWidget.cpp'],
  ['Rok2ResearchWidget.h', 'URok2ResearchWidget', 'Rok2ResearchWidget.cpp'],
  ['Rok2TrainHealSheetWidget.h', 'URok2TrainHealSheetWidget', 'Rok2TrainHealSheetWidget.cpp'],
  ['Rok2ExitConfirmWidget.h', 'URok2ExitConfirmWidget', 'Rok2ExitConfirmWidget.cpp'],
];

for (const [header, cls, cpp] of LAYERS) {
  const h = pub(header);
  check(`${cls}: يرث IRok2DismissibleLayer`,
    new RegExp(`class\\s+(?:ROK2_API\\s+)?${cls}\\s*:\\s*public UUserWidget,\\s*public IRok2DismissibleLayer`).test(h));
  check(`${cls}: يضمّن عقد الطبقة`, h.includes('#include "Rok2DismissibleLayer.h"'));
  check(`${cls}: ينفّذ DismissLayer`, /virtual void DismissLayer\(\) override/.test(h));

  // التسريح يمرّ بحركة: لا `RemoveFromParent()` عارية في ملف التنفيذ.
  const c = stripComments(priv(cpp));
  const bareRemove = /(^|\n)\s*RemoveFromParent\(\);/.test(c);
  check(`${cls}: بلا RemoveFromParent عارية في التسريح`, !bareRemove,
    '§1 «لا قفزات جامدة» — الإغلاق بحركة PlayFadeOut');
}

// ---------------------------------------------------------------------------
console.log('\n[4] الشاشات التي كانت بلا أي مسار إغلاق');
// ---------------------------------------------------------------------------
// القادة: كانت تُضاف للمنفذ ولا تُزال أبداً — لا زر ولا حجاب ولا مسار.
const cmdrH = pub('Rok2CommanderWidget.h');
const cmdrC = stripComments(priv('Rok2CommanderWidget.cpp'));
check('القادة: CloseSelf معلنة', cmdrH.includes('void CloseSelf();'));
check('القادة: زر إغلاق مبني في الترويسة',
  cmdrC.includes('CommanderCloseButton')
  && cmdrC.includes('&URok2CommanderWidget::OnCloseClicked'));
check('القادة: الإغلاق بتلاشٍ',
  fnBody(cmdrC, 'void URok2CommanderWidget::CloseSelf').includes('PlayFadeOut(this)'));

// الدردشة: زر «_» يطوي المحتوى ويبقي الترويسة معلقة إلى الأبد.
const chatH = pub('Rok2ChatWidget.h');
const chatC = stripComments(priv('Rok2ChatWidget.cpp'));
check('الدردشة: CloseSelf معلنة', chatH.includes('void CloseSelf();'));
check('الدردشة: زر إغلاق منفصل عن زر التصغير',
  chatC.includes('ChatCloseButton')
  && chatC.includes('&URok2ChatWidget::OnCloseClicked')
  && chatC.includes('&URok2ChatWidget::OnMinimizeClicked'));
const chatClose = fnBody(chatC, 'void URok2ChatWidget::CloseSelf');
check('الدردشة: الإغلاق بتلاشٍ', chatClose.includes('PlayFadeOut(this)'));
check('الدردشة: الإغلاق يفكّ حالة التصغير',
  chatClose.includes('bMinimized') && chatClose.includes('OnMinimizeClicked()'),
  'وإلا فُتحت في المرة القادمة بشريط ترويسة وحده');

// لوحة المسيرة: تُفتح بلمس هدف ولا تُزال إلا بإرسال قوات.
const marchH = pub('Rok2MarchPanel.h');
const marchC = stripComments(priv('Rok2MarchPanel.cpp'));
check('المسيرة: CloseSelf معلنة', marchH.includes('void CloseSelf();'));
check('المسيرة: زر إغلاق في الترويسة',
  marchC.includes('MarchCloseButton') && marchC.includes('&URok2MarchPanel::OnCloseClicked'));
check('المسيرة: الإغلاق بتلاشٍ',
  fnBody(marchC, 'void URok2MarchPanel::CloseSelf').includes('PlayFadeOut(this)'));

// شاشة التحالف: زر «✕» كان مربوطاً بـRemoveFromParent مباشرة.
const allianceC = stripComments(priv('Rok2AllianceRosterWidget.cpp'));
check('التحالف: زر ✕ لم يبق مربوطاً بـRemoveFromParent',
  !allianceC.includes('AddDynamic(this, &URok2AllianceRosterWidget::RemoveFromParent)'));
check('التحالف: زر ✕ يمرّ بمعالج الإغلاق',
  allianceC.includes('&URok2AllianceRosterWidget::OnCloseClicked'));

// ---------------------------------------------------------------------------
console.log('\n[5] ترتيب لوحة المسيرة في المنفذ');
// ---------------------------------------------------------------------------
const pcC = stripComments(priv('Rok2PlayerController.cpp'));
check('لوحة المسيرة تُضاف بترتيب اللوحات 50',
  pcC.includes('Panel->AddToViewport(50)'),
  'كانت AddToViewport() بلا معامل — ZOrder 0، تحت الـHUD (20)');
check('لا AddToViewport بلا معامل في المتحكم',
  !/AddToViewport\(\s*\)/.test(pcC));

// ---------------------------------------------------------------------------
console.log('\n[6] OnEscape لم يبق فارغاً + مسار الرجوع التدريجي');
// ---------------------------------------------------------------------------
const pcH = pub('Rok2PlayerController.h');
check('HandleBackRequested معلنة عامة', /void HandleBackRequested\(\);/.test(pcH));
check('OnAndroidBack معلنة', /void OnAndroidBack\(\);/.test(pcH));

const escapeBody = fnBody(pcC, 'void ARok2PlayerController::OnEscape');
check('OnEscape لم يبق فارغاً', escapeBody.includes('HandleBackRequested()'),
  'كان جسمها تعليقاً واحداً بلا سطر تنفيذي');
check('OnAndroidBack تشترك في نفس المسار',
  fnBody(pcC, 'void ARok2PlayerController::OnAndroidBack').includes('HandleBackRequested()'));

const backBody = fnBody(pcC, 'void ARok2PlayerController::HandleBackRequested');
check('الرجوع يبدأ بأعلى طبقة', backBody.includes('URok2UiStack::DismissTopLayer(this)'));
check('طبقة واحدة لكل ضغطة (return بعد الإغلاق)',
  /DismissTopLayer\(this\)\)\s*\{\s*return;/.test(backBody));
check('الرجوع يخرج من وضع تحرير المدينة',
  backBody.includes('IsEditModeActive()') && backBody.includes('ToggleEditMode()'));
check('الرجوع من الخريطة يعود للمدينة',
  backBody.includes('IsCityView()') && backBody.includes('SwitchToCityView()'));
check('لا انتقال أثناء انتقال جارٍ',
  backBody.includes('IsTransitioning()'));
check('تأكيد الخروج آخر مرحلة',
  backBody.includes('URok2ExitConfirmWidget'));
check('لوحة التأكيد تُنشأ مرة وتُعاد',
  backBody.includes('if (!ExitConfirmWidget)') && backBody.includes('!ExitConfirmWidget->IsInViewport()'));
check('لوحة التأكيد فوق اللوحات وتحت شاشة الدخول',
  backBody.includes('ExitConfirmWidget->AddToViewport(90)'));

// ترتيب المراحل: الطبقات ← التحرير ← الخريطة ← الخروج.
const idxLayer = backBody.indexOf('DismissTopLayer');
const idxEdit = backBody.indexOf('IsEditModeActive');
const idxMap = backBody.indexOf('SwitchToCityView');
const idxExit = backBody.indexOf('ExitConfirmWidget');
check('ترتيب المراحل: طبقات ← تحرير ← خريطة ← خروج',
  idxLayer >= 0 && idxLayer < idxEdit && idxEdit < idxMap && idxMap < idxExit,
  'الرجوع يُلغي آخر ما فعله اللاعب، فلا يُقذف من فوق الخريطة خارج التطبيق');

// ---------------------------------------------------------------------------
console.log('\n[7] Android Back: مفتاح مربوط لا مجرد ActionMapping');
// ---------------------------------------------------------------------------
const setupInput = fnBody(pcC, 'void ARok2PlayerController::SetupInputComponent');
check('EKeys::Android_Back مربوط صريحاً',
  setupInput.includes('BindKey(EKeys::Android_Back'),
  'AKEYCODE_BACK مسجّل باسمين في GetKeyMap — أي منهما قد يصل');
check('Escape يبقى مربوطاً عبر ActionMapping',
  setupInput.includes("BindAction(TEXT(\"Escape\")"));

const inputIni = readOr(join(ROOT, '..', '..', 'Config', 'DefaultInput.ini'));
check('DefaultInput.ini يحتفظ بربط Escape',
  inputIni.includes('ActionName="Escape"'));

// ---------------------------------------------------------------------------
console.log('\n[8] حارس الضغطة المزدوجة');
// ---------------------------------------------------------------------------
check('نافذة تجاهل معلنة كثابت مُسمّى', /BackDebounceSeconds\s*=\s*0\.15f/.test(pcH));
check('زمن آخر ضغطة محفوظ', pcH.includes('LastBackHandledSeconds'));
check('الحارس يعمل قبل أي إغلاق',
  backBody.indexOf('BackDebounceSeconds') >= 0
  && backBody.indexOf('BackDebounceSeconds') < idxLayer,
  'وإلا أغلقت الضغطة الواحدة طبقتين على أندرويد');

// ---------------------------------------------------------------------------
console.log('\n[9] لوحة تأكيد الخروج');
// ---------------------------------------------------------------------------
const exitH = pub('Rok2ExitConfirmWidget.h');
const exitC = stripComments(priv('Rok2ExitConfirmWidget.cpp'));

check('اللوحة موجودة', exitH.length > 0 && exitC.length > 0);
check('الرجوع داخلها = إلغاء لا تأكيد',
  /DismissLayer\(\) override \{ OnCancelClicked\(\); \}/.test(exitH),
  'وإلا كانت ضغطة رجوع ثانية تُغلق التطبيق');
check('الخروج عبر QuitGame (سلوك واحد في PIE وعلى الجهاز)',
  exitC.includes('UKismetSystemLibrary::QuitGame'));
check('حجاب يُلمس للإلغاء',
  exitC.includes('ExitConfirmBackdrop') && exitC.includes('&URok2ExitConfirmWidget::OnCancelClicked'));
check('زر «البقاء» أساسي وزر «خروج» خَطِر',
  exitC.includes('Rok2Surface::PrimaryButton()') && exitC.includes('Rok2Surface::DangerButton()'));
check('نص اللوحة يذكر أن المملكة تبقى على الخادم',
  exitC.includes('على الخادم'));
check('تفتح من المركز (نافذة لا ورقة سفلية)',
  exitC.includes('PlayScaleInCenter('));
check('الإلغاء بتلاشٍ',
  fnBody(exitC, 'void URok2ExitConfirmWidget::OnCancelClicked').includes('PlayFadeOut(this)'));
check('بلا FLinearColor خام (الألوان من Rok2Visual)',
  !/FLinearColor\s*\(\s*[0-9]/.test(exitC));
check('بلا SetBrushColor (السطح من Rok2Surface)', !exitC.includes('SetBrushColor('));
check('الفراغ من سلم Rok2Space', (exitC.match(/Rok2Space::/g) || []).length >= 6);
check('الخطوط من URok2Typography', exitC.includes('URok2Typography::ApplyFont'));

// ---------------------------------------------------------------------------
console.log('\n[10] إعادة خصائص الرندر بعد حركة الخروج');
// ---------------------------------------------------------------------------
const motionC = priv('Rok2MotionLibrary.cpp');
const tickBody = fnBody(stripComments(motionC), 'bool URok2MotionLibrary::TickTweens');
check('الشفافية تُعاد بعد الإزالة',
  /RemoveFromParent\(\);[\s\S]{0,400}SetRenderOpacity\(1\.f\)/.test(tickBody),
  'حركة الخروج تنتهي بشفافية 0 — بلا إعادة تُفتح اللوحة شفافة في المرة الثانية');
check('الإزاحة والمقياس يُعادان كذلك',
  tickBody.includes('SetRenderTranslation(FVector2D::ZeroVector)')
  && tickBody.includes('SetRenderScale(FVector2D(1.f, 1.f))'));
check('الحركة تُسقط من المصفوفة قبل الإزالة (أمان الاستدعاء المتعاود)',
  /const bool bRemoveWidget = T\.bRemoveOnFinish;[\s\S]{0,200}Tweens\.RemoveAt\(i\);[\s\S]{0,200}RemoveFromParent/.test(tickBody));

// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(60));
console.log(`P18-T5 structural verification: ${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
