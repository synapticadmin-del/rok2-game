#!/usr/bin/env node
/**
 * verify_p18_t3_toasts.mjs — P18-T3: بطاقات الإشعارات (توست) بحركة دخول وخروج.
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. `URok2MotionLibrary::PlayToastIn` و`PlayToastOut` معرّفتان منذ P6-T3
 *      **ولا مستدعٍ لهما في أي ودجة** — grep يجدهما في `Rok2MotionLibrary.cpp`
 *      وحده. فالبطاقة تظهر فجأة مكتملة الشفافية ولا تختفي بحركة، وهو نقيض
 *      `ui-ux-design-system.md` §7 «بطاقة تنزلق … + تتلاشى».
 *   2. لا عمر للبطاقة: `OnToast` تبنيها وتضيفها ولا شيء يزيلها. البطاقات
 *      الثلاث تبقى على الشاشة حتى يدفعها توست رابع.
 *   3. السقف كان يُطبَّق بـ`ToastsBox->RemoveChildAt(0)` — **إزالة مفاجئة**
 *      تُقصّ البطاقة من الشاشة، وهو ما تمنعه §1 «لا قفزات جامدة».
 *   4. `PlayGoldFlash` كذلك بلا أي مستدعٍ، فقاعدة §1 «كل تأكيد له وميض ذهبي»
 *      لم تكن مطبَّقة في أي موضع في اللعبة.
 *
 * فحص بنيوي: لا يحتاج بناء UE5 ولا خادماً يعمل.
 *
 * Usage: node scripts/verify_p18_t3_toasts.mjs
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
const stripComments = (s) => s.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');

/** جسم دالة بمطابقة أقواس متوازنة. */
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

const hudH = readOr(join(ROOT, 'Public', 'Rok2HudWidget.h'));
const hudC = readOr(join(ROOT, 'Private', 'Rok2HudWidget.cpp'));
const hudHCode = stripComments(hudH);
const hudCCode = stripComments(hudC);
const motionC = stripComments(readOr(join(ROOT, 'Private', 'Rok2MotionLibrary.cpp')));

check('Rok2HudWidget.h موجود', hudH.length > 0);
check('Rok2HudWidget.cpp موجود', hudC.length > 0);

// ---------------------------------------------------------------------------
console.log('\n[1] حركة الدخول والخروج صارت مستعملة فعلاً');
// ---------------------------------------------------------------------------
check('PlayToastIn لها مستدعٍ في الـHUD',
  hudCCode.includes('URok2MotionLibrary::PlayToastIn('),
  'كانت معرّفة في المكتبة بلا أي مستدعٍ في المشروع');
check('PlayToastOut لها مستدعٍ في الـHUD',
  hudCCode.includes('URok2MotionLibrary::PlayToastOut('));
check('الدخول يُطلق على البطاقة المبنيّة داخل OnToast',
  fnBody(hudCCode, 'void URok2HudWidget::OnToast').includes('PlayToastIn(Card)'));
check('الخروج يُطلق من دالة واحدة (BeginToastExit)',
  fnBody(hudCCode, 'void URok2HudWidget::BeginToastExit').includes('PlayToastOut('),
  'مسار خروج واحد يمنع تفرّق المنطق');

// المكتبة لم تتغيّر: الحركتان تُزيلان الودجة عند الانتهاء.
check('ToastOut تُزيل الودجة عند انتهاء الحركة (bRemoveOnFinish)',
  /case ERok2Motion::ToastOut:[\s\S]{0,200}bRemoveOnFinish = true;/.test(motionC),
  'وإلا بقيت بطاقة شفافة في الشجرة');

// ---------------------------------------------------------------------------
console.log('\n[2] حارس bExiting — العطل الجوهري');
// ---------------------------------------------------------------------------
check('FToastEntry تحمل راية bExiting', /bool bExiting\s*=\s*false;/.test(hudHCode));
const exitBody = fnBody(hudCCode, 'void URok2HudWidget::BeginToastExit');
check('BeginToastExit تعود مبكراً إن كان الخروج جارياً',
  /if \(Entry\.bExiting\) return;/.test(exitBody),
  'بلا الحارس يُطلق NativeTick الحركة كل إطار فلا تختفي البطاقة أبداً');
check('الراية تُرفع قبل تشغيل الحركة',
  exitBody.indexOf('Entry.bExiting = true') >= 0
  && exitBody.indexOf('Entry.bExiting = true') < exitBody.indexOf('PlayToastOut('));
check('لا PlayToastOut خارج BeginToastExit',
  (hudCCode.match(/PlayToastOut\(/g) || []).length === 1,
  'مسار خروج وحيد — أي موضع ثانٍ يتجاوز الحارس');

// ---------------------------------------------------------------------------
console.log('\n[3] عمر البطاقة يُحسب بالـDelta الحقيقي');
// ---------------------------------------------------------------------------
check('TickToasts معلنة', /void TickToasts\(float InDeltaTime\);/.test(hudHCode));
const tickBody = fnBody(hudCCode, 'void URok2HudWidget::NativeTick');
check('TickToasts تُنادى من NativeTick', tickBody.includes('TickToasts(InDeltaTime)'));
check('تُنادى قبل بوابة الربع ثانية',
  tickBody.indexOf('TickToasts(InDeltaTime)') >= 0
  && tickBody.indexOf('TickToasts(InDeltaTime)') < tickBody.indexOf('HudRefreshAccumulator'),
  'لو مرّت عبر البوابة لخُصمت المدة بقفزات 0.25s وتأخّر الخروج');

const toastTick = fnBody(hudCCode, 'void URok2HudWidget::TickToasts');
check('المدة تُخصم بالـDelta', /Remaining -= InDeltaTime;/.test(toastTick));
check('الخروج يبدأ عند نفاد المدة',
  /Remaining <= 0\.f/.test(toastTick) && toastTick.includes('BeginToastExit('));
check('مدة البقاء ثابتٌ مُسمّى لا رقم مبثوث',
  /ToastLifetimeSeconds\s*=\s*[0-9.]+f/.test(hudHCode));
check('التكرار عكسي (الإزالة أثناء المرور آمنة)',
  /for \(int32 i = ActiveToasts\.Num\(\) - 1; i >= 0; --i\)/.test(toastTick));
check('البطاقة المختفية تُسقط من القائمة',
  toastTick.includes('!Entry.Card.IsValid()') && toastTick.includes('ActiveToasts.RemoveAt(i)'));
check('لا حساب شفافية يدوي في الودجة',
  !/SetRenderOpacity\(/.test(hudCCode),
  'التلاشي مسؤولية المكتبة — تكراره في الودجة ينحرف عن المعيار الموحد');

// ---------------------------------------------------------------------------
console.log('\n[4] سقف ثلاث بطاقات — بحركة لا بقصّ');
// ---------------------------------------------------------------------------
check('السقف ثابتٌ مُسمّى', /MaxVisibleToasts\s*=\s*3;/.test(hudHCode));
const onToast = fnBody(hudCCode, 'void URok2HudWidget::OnToast');
check('السقف يُطبَّق على البطاقات الحيّة',
  onToast.includes('MaxVisibleToasts') && onToast.includes('bExiting'),
  'العدّ على أطفال الصندوق يشمل بطاقة في منتصف خروجها فيتقلّص السقف بلا سبب');
check('تجاوز السقف يُخرج الأقدم بحركة',
  onToast.includes('BeginToastExit('));
check('لا RemoveChildAt للتقليم',
  !hudCCode.includes('RemoveChildAt('),
  '§1 «لا قفزات جامدة» — كانت RemoveChildAt(0) تقصّ البطاقة');
check('لا GetChildrenCount كمصدر للسقف',
  !hudCCode.includes('ToastsBox->GetChildrenCount()'));
check('لا إزالة يدوية لبطاقة (المكتبة تتولّاها)',
  !/Card->RemoveFromParent\(\)/.test(hudCCode));

// ---------------------------------------------------------------------------
console.log('\n[5] مرساة الـGC صحيحة (لا UPROPERTY في بنية غير منعكسة)');
// ---------------------------------------------------------------------------
check('ToastCardRefs مصفوفة UPROPERTY',
  /UPROPERTY\(Transient\)\s*TArray<UBorder\*> ToastCardRefs;/.test(hudHCode),
  'البطاقة تُزال من الشجرة عند انتهاء الخروج فلا مالك منعكس لها');
const entryStruct = /struct FToastEntry\s*\{([\s\S]*?)\n\t\};/.exec(hudHCode);
check('FToastEntry بلا UPROPERTY داخلها',
  entryStruct !== null && !/UPROPERTY/.test(entryStruct[1]),
  'UPROPERTY في بنية غير USTRUCT لا يفعل شيئاً — يوهم بحماية غير موجودة');
check('FToastEntry تحفظ البطاقة كمؤشر ضعيف',
  entryStruct !== null && /TWeakObjectPtr<UBorder> Card;/.test(entryStruct[1]));
check('المراسي تُنظَّف مما لم يبق حيّاً',
  toastTick.includes('ToastCardRefs.RemoveAt(i)'),
  'بلا تنظيف تكبر القائمة بلا حدّ خلال جلسة طويلة');
check('كل بطاقة جديدة تُرسى',
  onToast.includes('ToastCardRefs.Add(Card)'));

// ---------------------------------------------------------------------------
console.log('\n[6] صوت الإشعار');
// ---------------------------------------------------------------------------
check('توست جديد يُشغّل صوت الإشعار',
  onToast.includes('ERok2AudioType::Notification'));
check('الصوت مرة واحدة لكل بطاقة (داخل OnToast لا في التِك)',
  !toastTick.includes('PlaySfx('));

// ---------------------------------------------------------------------------
console.log('\n[7] الوميض الذهبي صار مستعملاً (§1: كل تأكيد له وميض)');
// ---------------------------------------------------------------------------
check('PlayGoldFlash لها مستدعٍ',
  hudCCode.includes('URok2MotionLibrary::PlayGoldFlash('),
  'كانت معرّفة في المكتبة بلا مستدعٍ واحد في المشروع');
const notifBody = fnBody(hudCCode, 'void URok2HudWidget::OnNotification');
check('الوميض على أيقونة الجرس عند وصول إشعار',
  /PlayGoldFlash\(BellIcon\)/.test(notifBody),
  'الهدف UImage فتصبغه المكتبة ذهباً فعلاً لا نبضة شفافية');

// ---------------------------------------------------------------------------
console.log('\n[8] الأسطح والألوان والخطوط من نظام التصميم');
// ---------------------------------------------------------------------------
check('بطاقة التوست من Rok2Surface', onToast.includes('Rok2Surface::AccentCard('));
check('اللون من Rok2Visual', onToast.includes('Rok2Visual::'));
check('الخط من URok2Typography', onToast.includes('URok2Typography::ApplyFont'));
check('الفراغ من سلم Rok2Space', onToast.includes('Rok2Space::'));
check('بلا FLinearColor خام في الـHUD',
  !/FLinearColor\s*\(\s*[0-9]/.test(hudCCode.replace(/FLinearColor\s*\([^)]*\.[RGBA]\b[^)]*\)/g, '')));

// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(60));
console.log(`P18-T3 structural verification: ${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
