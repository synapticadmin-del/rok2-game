#!/usr/bin/env node
/**
 * verify_ui_ftue.mjs — P6-T4 إرشاد الدقيقة الأولى: حرس بنيوي بطورين
 *
 * ما يحرسه: أن الخطوات الثلاث معرَّفة بنصّها ورمزها ومرساتها، وأن الخطوة
 * تُستنتج من حالة السيرفر ولا تُخزَّن، وأن المِزلاج لا يرتدّ، وأن الطبقة لا
 * تحجب اللمس عن الزر الذي تُبرزه، وأن النبضة تتجدّد، وأن كل مرساة مُعلنة
 * مُسجَّلة فعلاً.
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  الطورَان — ولماذا الطور الثاني ليس ترفاً
 * ────────────────────────────────────────────────────────────────────────────
 *   الطور 1 (إثبات): المدققون على الشجرة الحقيقية → يجب أن ينجحوا كلهم
 *   الطور 2 (نفي):   كل مدقق على العطل الذي يحرسه → يجب أن يفشل كل مرة
 *
 * فحصٌ بتعبير نمطي خاطئ «ينجح» على شجرة سليمة وعلى شجرة معطوبة سواءً. مدقق
 * ينجح في الطور 1 ولا يفشل في الطور 2 يُحتسب **عطلاً في الحرس نفسه**.
 *
 * تحفّظ: لا مترجم C++ هنا — تحقق بنيوي، ولا ادّعاء ببناء ناجح ولا بشكل نهائي
 * على الشاشة (الأخير يحتاج عيناً على اللعبة).
 *
 * Usage: node scripts/verify_ui_ftue.mjs [--verbose]
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..');
const MOD = join(ROOT, 'game', 'client-unreal', 'Source', 'Rok2');
const VERBOSE = process.argv.includes('--verbose');

/**
 * تعليقات السطر أولاً ثم الكتل — الترتيب المعكوس عطل كامن: تعليق سطر يحتوي
 * فاتحة كتلة يفتح كتلة وهمية تمحو الكود الحقيقي بينها وأول خاتمة، فيجري الفحص
 * على فراغ ويُبلّغ نجاحاً كاذباً. (العطل نفسه الذي أُصلح في verify_delegate_bind
 * وحُرِس في verify_ui_typography — ويحرسه هنا S0.)
 */
function stripComments(src) {
  return src.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
}

function readTree() {
  const vfs = new Map();
  for (const sub of ['Public', 'Private']) {
    const dir = join(MOD, sub);
    if (!existsSync(dir)) continue;
    for (const f of readdirSync(dir)) {
      if (f.endsWith('.h') || f.endsWith('.cpp')) {
        vfs.set(`${sub}/${f}`, readFileSync(join(dir, f), 'utf8'));
      }
    }
  }
  // ملفات خارج الموديول تدخل الـVFS لتكون الطفرات قادرة على مسّها أيضاً
  for (const [key, rel] of [
    ['Data/buildings.json', join('data', 'buildings.json')],
    ['Doc/PLAN.md', 'PLAN.md'],
  ]) {
    const p = join(ROOT, rel);
    if (existsSync(p)) vfs.set(key, readFileSync(p, 'utf8'));
  }
  return vfs;
}

const get = (vfs, k) => vfs.get(k) ?? '';
const code = (vfs, k) => stripComments(get(vfs, k));

const OH = 'Public/Rok2Onboarding.h';
const OC = 'Private/Rok2Onboarding.cpp';
const WH = 'Public/Rok2OnboardingWidget.h';
const WC = 'Private/Rok2OnboardingWidget.cpp';
const GH = 'Public/Rok2GameMode.h';
const GC = 'Private/Rok2GameMode.cpp';
const HUD = 'Private/Rok2HudWidget.cpp';
// P24-T1: `Rok2CityWidget` تقاعدت — كانت تبني ثلاثة ألواح ثم تخفيها كلها
// بـ`ESlateVisibility::Collapsed` بلا مسار يعيد إظهارها، فمرساة التدريب
// المسجّلة فيها كانت هندستها صفرية والحلقة الذهبية تُخفى أبداً. الحبّة في
// الـHUD مرئية فعلاً، فالمرساتان الآن من ملف واحد.
const ICONS = 'Private/Rok2IconLibrary.cpp';
const BUILDINGS = 'Data/buildings.json';

function mutate(vfs, edits) {
  const copy = new Map(vfs);
  for (const [k, fn] of Object.entries(edits)) {
    const before = copy.get(k);
    if (before === undefined) throw new Error(`mutation target missing: ${k}`);
    const after = fn(before);
    if (after === before) throw new Error(`mutation was a no-op on ${k}`);
    copy.set(k, after);
  }
  return copy;
}

/**
 * طفرة نصّية بمرساة **فريدة**. الالتباس عطل حقيقي: طفرة وُصفت لدالة أصابت
 * أخرى تشترك معها في نص المرساة، فمرّت دون أن يرصدها أحد — واختبار نفي يعدّل
 * الموضع الخطأ يُعطي طمأنينة كاذبة تماماً كفحص بتعبير نمطي خاطئ.
 */
function replaceUnique(src, needle, replacement) {
  const n = src.split(needle).length - 1;
  if (n === 0) throw new Error(`anchor not found: ${needle.slice(0, 60)}`);
  if (n > 1) throw new Error(`anchor is ambiguous (${n} matches): ${needle.slice(0, 60)}`);
  return src.replace(needle, replacement);
}

const ok = (name, detail) => ({ ok: true, name, detail });
const bad = (name, detail) => ({ ok: false, name, detail });
const test = (cond, name, detail) => (cond ? ok(name) : bad(name, detail));

// ---------------------------------------------------------------------------
// مستخرِجات
// ---------------------------------------------------------------------------

/** أسماء خطوات التعداد بترتيب الظهور */
function stepNames(vfs) {
  const m = /enum class ERok2FtueStep : uint8\s*\{([\s\S]*?)\n\};/.exec(code(vfs, OH));
  if (!m) return [];
  return [...m[1].matchAll(/^\s*(\w+)\s*=\s*(\d+)\s*,?\s*$/gm)].map((x) => ({
    name: x[1],
    value: Number(x[2]),
  }));
}

/** جسم StepInfo وحده — لا CompletionInfo */
function stepInfoBody(vfs) {
  const src = code(vfs, OC);
  const m = /FRok2FtueStepInfo URok2Onboarding::StepInfo\(ERok2FtueStep Step\)([\s\S]*?)\n\}/.exec(src);
  return m ? m[1] : '';
}

/** نداءات MakeInfo في StepInfo: نستخرج معرّف الأيقونة والمرساة والرقم */
function makeInfoCalls(vfs) {
  const body = stepInfoBody(vfs);
  const calls = [];
  // MakeInfo(Step, N, TEXT("..."), TEXT("..."), TEXT("..."), TEXT("icon"), Anchor)
  const re = /MakeInfo\(\s*Step\s*,\s*(\d+)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*([\w:]+)\s*\)/g;
  for (const m of body.matchAll(re)) {
    calls.push({
      ordinal: Number(m[1]), title: m[2], story: m[3],
      action: m[4], icon: m[5], anchor: m[6],
    });
  }
  return calls;
}

/** أسماء المراسي المُعلنة في namespace Rok2FtueSpec */
function declaredAnchors(vfs) {
  const h = code(vfs, OH);
  const ns = /namespace Rok2FtueSpec\s*\{([\s\S]*?)\n\}/.exec(h);
  if (!ns) return [];
  return [...ns[1].matchAll(/extern\s+ROK2_API\s+const\s+FName\s+(\w+)\s*;/g)].map((x) => x[1]);
}

/** المراسي المُسجَّلة فعلاً عبر RegisterAnchor في الودجات */
function registeredAnchors(vfs) {
  const out = [];
  for (const k of vfs.keys()) {
    if (!k.startsWith('Private/') || k.startsWith('Private/Rok2Onboarding')) continue;
    for (const m of code(vfs, k).matchAll(/RegisterAnchor\(\s*Rok2FtueSpec::(\w+)\s*,\s*(\w+)\s*\)/g)) {
      out.push({ file: k, anchor: m[1], widget: m[2] });
    }
  }
  return out;
}

/** معرّفات الأيقونات التي تعرفها مكتبة الأيقونات */
function knownIcons(vfs) {
  return new Set([...code(vfs, ICONS).matchAll(/"([a-z_0-9]+)"/g)].map((m) => m[1]));
}

/** جسم دالة بالاسم من ملف */
function fnBody(vfs, key, signature) {
  const src = code(vfs, key);
  const i = src.indexOf(signature);
  if (i < 0) return '';
  let depth = 0, started = false;
  for (let j = i; j < src.length; j++) {
    if (src[j] === '{') { depth++; started = true; }
    else if (src[j] === '}') { depth--; if (started && depth === 0) return src.slice(i, j + 1); }
  }
  return '';
}

// ---------------------------------------------------------------------------
// المدققون
// ---------------------------------------------------------------------------

const DETECTORS = [
  {
    id: 'S0',
    title: 'سلامة المُجرِّد نفسه — تعليقات السطر قبل الكتل',
    run: () => {
      const sample = 'int a = 1; // path /Engine/BasicShapes/*\nint b = 2;\n';
      const stripped = stripComments(sample);
      return [
        test(stripped.includes('int b = 2'), 'تعليق سطر يحوي فاتحة كتلة لا يمحو ما بعده'),
        test(stripComments('/* x */ int c;').includes('int c'), 'تعليق كتلة يُزال فعلاً'),
      ];
    },
    breaks: [{
      why: 'المُجرِّد بترتيب معكوس (الكتل قبل السطر) يمحو كوداً حقيقياً',
      selfCheck: () => {
        const reversed = (s) =>
          s.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
        const sample = 'int a = 1; // path /Engine/BasicShapes/*\nint b = 2;\n*/ int d;';
        return !reversed(sample).includes('int b = 2');
      },
    }],
  },

  {
    id: 'F1',
    title: 'تغطية الخطوات: كل خطوة مرشدة معرَّفة بنصّ كامل',
    run: (vfs) => {
      const steps = stepNames(vfs);
      const calls = makeInfoCalls(vfs);
      const r = [];
      r.push(test(steps.length === 5, 'التعداد فيه 5 قيم (None + 3 + Done)', `وجد ${steps.length}`));
      r.push(test(
        steps.every((s, i) => s.value === i),
        'قيم التعداد تصاعدية بلا فجوة — المِزلاج يقارنها عدديّاً',
        steps.map((s) => `${s.name}=${s.value}`).join(', ')
      ));
      r.push(test(calls.length === 3, 'ثلاث خطوات مرشدة في StepInfo', `وجد ${calls.length}`));
      // عدّ نداءات MakeInfo وحده لا يكفي: خطوة قد تفقد **حالتها** فتسقط إلى
      // default وتعيد بنية فارغة، والنداءات الثلاثة باقية فيمرّ العدّ. فنؤكد
      // أن كل قيمة في التعداد لها حالة صريحة. (كشفه الطور الثاني في حرسي.)
      const body = stepInfoBody(vfs);
      for (const s of steps.filter((x) => x.name !== 'None')) {
        r.push(test(new RegExp(`case ERok2FtueStep::${s.name}\\s*:`).test(body),
          `الخطوة ${s.name} لها حالة صريحة في StepInfo`,
          'بلا حالة تسقط إلى default فتُعرض بطاقة فارغة'));
      }
      const ordinals = calls.map((c) => c.ordinal).sort();
      r.push(test(
        JSON.stringify(ordinals) === JSON.stringify([1, 2, 3]),
        'الأرقام 1..3 بلا تكرار', ordinals.join(',')
      ));
      for (const c of calls) {
        r.push(test(c.title.length > 0 && c.story.length > 0 && c.action.length > 0,
          `الخطوة ${c.ordinal}: عنوان وحكاية وإجراء غير فارغة`));
        // الإجراء منفصل عن الحكاية: بطاقة أدبية بلا إجراء تُلهم ولا تُرشد
        r.push(test(c.story !== c.action, `الخطوة ${c.ordinal}: الحكاية والإجراء نصّان مختلفان`));
      }
      return r;
    },
    breaks: [
      {
        why: 'حذف خطوة من StepInfo يترك بطاقة فارغة',
        edits: { [OC]: (s) => replaceUnique(s, 'case ERok2FtueStep::GatherMarch:', 'case ERok2FtueStep::NeverMatched:') },
      },
      {
        why: 'إجراء يساوي الحكاية = بطاقة تُلهم ولا تُرشد',
        edits: { [OC]: (s) => replaceUnique(s, 'TEXT("المس المطرقة، ثم اختر «المزرعة»."),', 'TEXT("لا تُبنى مملكةٌ على جوع. ابدأ بحقلٍ يُطعم من سيحملون رايتك."),') },
      },
      {
        why: 'فجوة في قيم التعداد تكسر مقارنة الترتيب',
        edits: { [OH]: (s) => replaceUnique(s, 'TrainTroops = 2,', 'TrainTroops = 7,') },
      },
    ],
  },

  {
    id: 'F2',
    title: 'أيقونات الخطوات موجودة فعلاً في مكتبة الأيقونات',
    run: (vfs) => {
      const icons = knownIcons(vfs);
      const calls = makeInfoCalls(vfs);
      const compl = /MakeInfo\([\s\S]*?TEXT\("(\w+)"\),\s*NAME_None\)/.exec(
        fnBody(vfs, OC, 'FRok2FtueStepInfo URok2Onboarding::CompletionInfo()')
      );
      const r = calls.map((c) =>
        test(icons.has(c.icon), `أيقونة الخطوة ${c.ordinal} «${c.icon}» معروفة`,
          'غير موجودة في Rok2IconLibrary — البطاقة ستظهر بلا رمز')
      );
      r.push(test(!!compl && icons.has(compl[1]), 'أيقونة بطاقة التتويج معروفة', compl ? compl[1] : 'لم تُستخرج'));
      r.push(test(calls.every((c) => !/[\u{1F300}-\u{1FAFF}\u{2600}-\u{27BF}]/u.test(c.title + c.story + c.action)),
        'لا إيموجي في نصوص الخطوات (قاعدة P6-T1)'));
      return r;
    },
    breaks: [
      {
        why: 'معرّف أيقونة غير موجود يجعل البطاقة بلا رمز بصمت',
        edits: { [OC]: (s) => replaceUnique(s, 'TEXT("farm"),\n\t\t\tRok2FtueSpec::AnchorBuild', 'TEXT("no_such_icon"),\n\t\t\tRok2FtueSpec::AnchorBuild') },
      },
      {
        why: 'إيموجي في نص الخطوة يخالف قاعدة الأيقونات الإجرائية',
        edits: { [OC]: (s) => replaceUnique(s, 'TEXT("رغيفٌ قبل السيف")', 'TEXT("🌾 رغيفٌ قبل السيف")') },
      },
    ],
  },

  {
    id: 'F3',
    title: 'الخطوة تُستنتج من الحالة ولا تُخزَّن',
    run: (vfs) => {
      const oc = code(vfs, OC);
      const wc = code(vfs, WC);
      const derive = fnBody(vfs, OC, 'ERok2FtueStep URok2Onboarding::DeriveRawStep');
      return [
        test(!/USaveGame|SaveGameToSlot|LoadGameFromSlot|GConfig|SetStringToConfig/.test(oc + wc),
          'لا طبقة تخزين للخطوة (لا SaveGame ولا GConfig)',
          'الخطوة تُخزَّن — مصدر حقيقة ثانٍ يخالف «الخادم هو السلطة»'),
        test(/HasFarm\(Api\)/.test(derive) && /HasTroops\(Api\)/.test(derive) && /HasOwnMarch\(Api\)/.test(derive),
          'الاستنتاج يسأل المُسنَدات الثلاثة على الـApi'),
        test(/IsStateReady\(Api\)/.test(derive), 'الاستنتاج يتحقق من جاهزية الحالة أولاً'),
        test(/Api->GetBuildings\(\)/.test(oc) && /Api->GetTroops\(\)/.test(oc) &&
             /Api->GetWorldSnapshot\(\)\.Marches/.test(oc),
          'المُسنَدات تقرأ حالة السيرفر مباشرة'),
        test(/ActiveQueues/.test(oc), 'الطابور يُحتسب إتماماً (لا إلحاح على طلبٍ نُفِّذ)'),
      ];
    },
    breaks: [
      {
        why: 'إدخال تخزين محلي للخطوة',
        edits: { [OC]: (s) => s.replace('#include "Rok2Api.h"', '#include "Rok2Api.h"\n#include "GameFramework/SaveGame.h"\n// USaveGame') + '\nvoid Persist() { SaveGameToSlot(nullptr, TEXT("ftue"), 0); }\n' },
      },
      {
        why: 'حذف احتساب الطابور يجعل البطاقة تُلحّ بطلبٍ نُفِّذ',
        edits: { [OC]: (s) => s.replace(/ActiveQueues/g, 'NoSuchField') },
      },
      {
        why: 'الاستنتاج لا يتحقق من جاهزية الحالة',
        edits: { [OC]: (s) => replaceUnique(s, 'if (!IsStateReady(Api)) return ERok2FtueStep::None;', 'if (false) return ERok2FtueStep::None;') },
      },
    ],
  },

  {
    id: 'F4',
    title: 'المِزلاج أحادي الاتجاه — لا ارتداد',
    run: (vfs) => {
      const ev = fnBody(vfs, OC, 'ERok2FtueStep URok2Onboarding::Evaluate');
      return [
        test(/OrdinalOf\(Raw\)\s*>\s*OrdinalOf\(CurrentStep\)/.test(ev),
          'التقدّم بمقارنة ترتيب تصاعدية صريحة',
          'بلا مقارنة «أكبر من» يمكن للخطوة أن ترتدّ إلى الوراء'),
        test(/if\s*\(\s*!bArmed\s*\)[\s\S]{0,80}return ERok2FtueStep::Done/.test(ev),
          'اللاعب غير المسلَّح يُعامل Done نهائياً'),
        test(/bArmed\s*=\s*\(Raw\s*!=\s*ERok2FtueStep::Done\)/.test(ev),
          'التسليح يشترط أن اللاعب لم يُنجز الثلاثة سلفاً'),
        test((ev.match(/bArmed\s*=/g) ?? []).length === 1,
          'التسليح يحدث في موضع واحد فقط',
          'إسناد bArmed في أكثر من موضع يفتح باب إعادة التسليح'),
        test(/!bEvaluatedOnce/.test(ev) && /bEvaluatedOnce\s*=\s*true/.test(ev),
          'أول تقييم محروس براية تُرفع مرة'),
      ];
    },
    breaks: [
      {
        why: 'تحويل المقارنة إلى != يسمح بالارتداد إلى خطوة سابقة',
        edits: { [OC]: (s) => replaceUnique(s, 'if (OrdinalOf(Raw) > OrdinalOf(CurrentStep))', 'if (OrdinalOf(Raw) != OrdinalOf(CurrentStep))') },
      },
      {
        why: 'تسليح اللاعب العائد (حذف شرط Done)',
        edits: { [OC]: (s) => replaceUnique(s, 'bArmed = (Raw != ERok2FtueStep::Done);', 'bArmed = true;') },
      },
      {
        why: 'إعادة التسليح في كل تقييم',
        edits: { [OC]: (s) => replaceUnique(s, '\tif (!bArmed)\n\t{\n\t\treturn ERok2FtueStep::Done;\n\t}', '\tbArmed = true;\n\tif (!bArmed)\n\t{\n\t\treturn ERok2FtueStep::Done;\n\t}') },
      },
    ],
  },

  {
    id: 'F5',
    title: 'بوابة الجاهزية — لا تقييم على مدينة لم تصل',
    run: (vfs) => {
      const ev = fnBody(vfs, OC, 'ERok2FtueStep URok2Onboarding::Evaluate');
      const ready = fnBody(vfs, OC, 'bool URok2Onboarding::IsStateReady');
      const gateIdx = ev.indexOf('IsStateReady');
      const armIdx = ev.indexOf('bEvaluatedOnce = true');
      return [
        test(gateIdx >= 0, 'Evaluate يستدعي بوابة الجاهزية'),
        test(gateIdx >= 0 && armIdx >= 0 && gateIdx < armIdx,
          'البوابة **قبل** رفع راية أول تقييم',
          'التقييم على مدينة فارغة يُسلّح الإرشاد لمحاربٍ قديم — الفخّ الأهم'),
        test(/return CurrentStep;/.test(ev.slice(gateIdx, armIdx)),
          'الحالة غير الجاهزة تُرجع بلا أثر جانبي'),
        test(/UpdatedAt\s*>\s*0/.test(ready),
          'دليل الجاهزية هو UpdatedAt غير الصفري'),
        test(!/HallLevel/.test(ready),
          'الجاهزية لا تعتمد HallLevel',
          'افتراضي HallLevel هو 1 فمدينة فارغة تُشبه مدينة حقيقية'),
        test(/HasPlayer\(\)/.test(ready), 'الجاهزية تشترط وصول اللاعب'),
      ];
    },
    breaks: [
      {
        why: 'حذف البوابة يسمح بالتقييم على مدينة فارغة فيُسلَّح محاربٌ قديم',
        edits: { [OC]: (s) => replaceUnique(s, '\tif (!IsStateReady(Api))\n\t{\n\t\treturn CurrentStep;\n\t}', '\t{\n\t}') },
      },
      {
        why: 'الاعتماد على HallLevel دليلَ جاهزية (افتراضيه 1)',
        edits: { [OC]: (s) => replaceUnique(s, 'return Api->HasPlayer() && Api->GetCity().UpdatedAt > 0;', 'return Api->HasPlayer() && Api->GetCity().HallLevel >= 1;') },
      },
    ],
  },

  {
    id: 'F6',
    title: 'الطبقة لا تحجب اللمس عن الزر الذي تُبرزه',
    run: (vfs) => {
      const wc = code(vfs, WC);
      const ring = fnBody(vfs, WC, 'void URok2OnboardingWidget::BuildRing');
      const tick = fnBody(vfs, WC, 'void URok2OnboardingWidget::NativeTick');
      return [
        test(/SetVisibility\(ESlateVisibility::HitTestInvisible\)/.test(
              fnBody(vfs, WC, 'void URok2OnboardingWidget::NativeConstruct')),
          'الودجة كلها HitTestInvisible'),
        test(/HitTestInvisible/.test(ring), 'أشرطة الحلقة HitTestInvisible'),
        test(/ESlateVisibility::HitTestInvisible/.test(tick),
          'الحلقة تُظهَر بـHitTestInvisible لا Visible',
          'حلقة تلتقط اللمس تجعل الزر الذي تشير إليه غير قابل للضغط'),
        test(!/SetVisibility\(ESlateVisibility::Visible\)/.test(wc),
          'لا ظهور حاجب في أي موضع من الطبقة',
          'أي ESlateVisibility::Visible في طبقة إرشاد يحجب ما تحتها'),
      ];
    },
    breaks: [
      {
        why: 'إظهار الحلقة بـVisible يبتلع لمسة الزر المُبرَز',
        edits: { [WC]: (s) => replaceUnique(s, '? ESlateVisibility::HitTestInvisible   //', '? ESlateVisibility::Visible   //') },
      },
      {
        why: 'إزالة HitTestInvisible عن الودجة الجذر',
        edits: { [WC]: (s) => replaceUnique(s, '\tSetVisibility(ESlateVisibility::HitTestInvisible);', '\tSetVisibility(ESlateVisibility::SelfHitTestInvisible);') },
      },
    ],
  },

  {
    id: 'F7',
    title: 'الحلقة إطار مفرَّغ ينبض كوحدة',
    run: (vfs) => {
      const ring = fnBody(vfs, WC, 'void URok2OnboardingWidget::BuildRing');
      const tick = fnBody(vfs, WC, 'void URok2OnboardingWidget::NativeTick');
      const bars = (ring.match(/AddChildToCanvas\(Bar\)/g) ?? []).length
        + (ring.match(/FAnchors\(/g) ?? []).length;
      return [
        test(/FBarSpec\s+Bars\[\]/.test(ring) && (ring.match(/FAnchors\(/g) ?? []).length >= 5,
          'أربعة أشرطة تُشكّل الإطار (+ شريحة الحاوية)', `وجد ${bars} إشارة`),
        test((ring.match(/\{\s*FAnchors\(/g) ?? []).length === 4,
          'أربعة أشرطة بالضبط — لا ثلاثة فينفتح الإطار',
          `وجد ${(ring.match(/\{\s*FAnchors\(/g) ?? []).length}`),
        test(/Play\(\s*Ring\s*,\s*ERok2Motion::Pulse\s*\)/.test(tick),
          'النبضة تُطبَّق على الحاوية لا على شريط مفرد',
          'Pulse حركة مقياس بمحور مركزي، فنبض الأشرطة فرادى يشوّه الإطار'),
        test(/SetRenderTransformPivot\(FVector2D\(0\.5f, 0\.5f\)\)/.test(ring),
          'محور الحاوية مركزي فلا قفزة في أول إطار'),
        test(!/Ring->SetBrushColor|SetContent\(Ring\)/.test(ring),
          'الحاوية غير مصبوغة — الإطار مفرَّغ لا مستطيل ممتلئ يغطّي الزر'),
      ];
    },
    breaks: [
      {
        why: 'ثلاثة أشرطة تفتح ضلعاً من الإطار',
        edits: { [WC]: (s) => replaceUnique(s, '\t\t{ FAnchors(1.f, 0.f, 1.f, 1.f), FVector2D(1.f, 0.f), FVector2D(Rok2FtueStyle::RingThickness, 0.f) },\n', '') },
      },
      {
        why: 'نبض شريط مفرد بدل الحاوية يشوّه الإطار',
        edits: { [WC]: (s) => replaceUnique(s, 'URok2MotionLibrary::Play(Ring, ERok2Motion::Pulse);', 'URok2MotionLibrary::Play(Card, ERok2Motion::Pulse);') },
      },
    ],
  },

  {
    id: 'F8',
    title: 'النبضة تتجدّد — Pulse لقطة واحدة لا حلقة',
    run: (vfs) => {
      const wc = code(vfs, WC);
      const tick = fnBody(vfs, WC, 'void URok2OnboardingWidget::NativeTick');
      return [
        test(/PulseInterval\s*=\s*[\d.]+f/.test(wc), 'دورية إعادة النبضة معرَّفة كثابت'),
        test(/PulseTimer\s*\+=\s*InDeltaTime/.test(tick) && /PulseTimer\s*>=\s*Rok2FtueStyle::PulseInterval/.test(tick),
          'النبضة تُعاد بدورية زمنية',
          'Pulse تنتهي بعد 0.40s — بلا إعادة يصير الإطار ساكناً لا نابضاً'),
        test(/GeometryTimer\s*\+=\s*InDeltaTime/.test(tick) && /GeometryInterval/.test(tick),
          'تتبّع الهندسة مخفوض التردّد لا كل إطار'),
        test(/IsShowingGuidance\(\)/.test(tick),
          'لا عمل في الـTick ما لم تكن هناك خطوة تُرشد',
          'اللاعب العائد يجب ألا يكلّف شيئاً'),
      ];
    },
    breaks: [
      {
        why: 'حذف إعادة النبضة يجعل الإطار ساكناً بعد 0.40s',
        edits: { [WC]: (s) => replaceUnique(s, 'PulseTimer += InDeltaTime;', 'PulseTimer += 0.f;') },
      },
      {
        why: 'الـTick يعمل للاعب العائد أيضاً',
        edits: { [WC]: (s) => replaceUnique(s, 'if (!Model->IsShowingGuidance())', 'if (false)') },
      },
    ],
  },

  {
    id: 'F9',
    title: 'المراسي: كل مُعلنة مُسجَّلة وكل مُسجَّلة مُعلنة',
    run: (vfs) => {
      const declared = declaredAnchors(vfs);
      const registered = registeredAnchors(vfs);
      const regNames = new Set(registered.map((r) => r.anchor));
      const usedInSteps = new Set(makeInfoCalls(vfs).map((c) => c.anchor.replace('Rok2FtueSpec::', '')));
      const r = [];
      r.push(test(declared.length === 3, 'ثلاث مراسٍ مُعلنة', declared.join(', ')));
      for (const d of declared) {
        r.push(test(regNames.has(d), `المرساة ${d} مُسجَّلة في ودجة فعلاً`,
          'مرساة مُعلنة بلا تسجيل = إبراز لا يظهر أبداً بصمت'));
      }
      for (const rg of registered) {
        r.push(test(declared.includes(rg.anchor), `المرساة المُسجَّلة ${rg.anchor} مُعلنة`, rg.file));
      }
      for (const u of usedInSteps) {
        r.push(test(regNames.has(u), `مرساة الخطوة ${u} مُسجَّلة`,
          'خطوة تشير إلى مرساة لا يسجّلها أحد = بطاقة بلا إبراز'));
      }
      r.push(test(registered.some((x) => x.file === HUD && x.anchor === 'AnchorBuild'),
        'مرساة البناء من الـHUD'));
      r.push(test(registered.some((x) => x.file === HUD && x.anchor === 'AnchorTrain'),
        'مرساة التدريب من الـHUD (حبّة مرئية لا لوح مطوي)'));
      // الغلاف المرئي لا الزر الداخلي: الحلقة تحيط بما يراه اللاعب
      r.push(test(registered.some((x) => x.anchor === 'AnchorBuild' && /Circle/i.test(x.widget)),
        'مرساة البناء هي الدائرة المرئية لا الزر الداخلي',
        registered.filter(x=>x.anchor==='AnchorBuild').map(x=>x.widget).join(',')));
      return r;
    },
    breaks: [
      {
        why: 'حذف تسجيل مرساة يترك خطوة بلا إبراز بصمت',
        edits: { [HUD]: (s) => replaceUnique(s, 'URok2Onboarding::Get()->RegisterAnchor(Rok2FtueSpec::AnchorBuild, Circle);', '// removed') },
      },
      {
        why: 'ترسية الزر الداخلي بدل الغلاف المرئي',
        edits: { [HUD]: (s) => replaceUnique(s, 'RegisterAnchor(Rok2FtueSpec::AnchorBuild, Circle);', 'RegisterAnchor(Rok2FtueSpec::AnchorBuild, Btn);') },
      },
      {
        why: 'خطوة تشير إلى مرساة غير مُعلنة',
        edits: { [OC]: (s) => replaceUnique(s, 'Rok2FtueSpec::AnchorTrain);', 'Rok2FtueSpec::AnchorGhost);') },
      },
    ],
  },

  {
    id: 'F10',
    title: 'ترتيب الطبقات: فوق اللوحات وتحت شاشة التحميل',
    run: (vfs) => {
      const gc = code(vfs, GC);
      const z = /OnboardingWidget->AddToViewport\((\d+)\)/.exec(gc);
      const boot = /BootWidget->AddToViewport\((\d+)\)/.exec(gc);
      const panel = /BuildMenuWidget->AddToViewport\((\d+)\)/.exec(gc);
      const zv = z ? Number(z[1]) : -1, bv = boot ? Number(boot[1]) : -1, pv = panel ? Number(panel[1]) : -1;
      return [
        test(z !== null, 'الطبقة تُضاف للمنفذ بترتيب صريح'),
        test(zv > pv, `الإرشاد (${zv}) فوق اللوحات (${pv})`,
          'البطاقة تُدفن تحت ورقة البناء — وهي الورقة التي تُرشد إليها الخطوة الأولى'),
        test(zv < bv, `الإرشاد (${zv}) تحت شاشة التحميل (${bv})`,
          'الإرشاد يظهر فوق شاشة التحميل'),
        test(/OnboardingWidget->Setup\(Api\)/.test(gc), 'الطبقة تُربط بالـApi'),
        test(/class URok2OnboardingWidget;/.test(code(vfs, GH)), 'تصريح أمامي في هيدر GameMode'),
      ];
    },
    breaks: [
      {
        why: 'ترتيب أدنى من اللوحات يدفن البطاقة تحت ورقة البناء',
        edits: { [GC]: (s) => replaceUnique(s, 'OnboardingWidget->AddToViewport(60);', 'OnboardingWidget->AddToViewport(40);') },
      },
      {
        why: 'ترتيب أعلى من شاشة التحميل يُظهر الإرشاد فوقها',
        edits: { [GC]: (s) => replaceUnique(s, 'OnboardingWidget->AddToViewport(60);', 'OnboardingWidget->AddToViewport(120);') },
      },
    ],
  },

  {
    id: 'F11',
    title: 'التقدّم من المفوَّضات لا من الاستقصاء',
    run: (vfs) => {
      const setup = fnBody(vfs, WC, 'void URok2OnboardingWidget::Setup');
      const wc = code(vfs, WC);
      return [
        test(/OnCityLoaded\.AddDynamic/.test(setup), 'مشترك في OnCityLoaded (المباني والطوابير)'),
        test(/OnWorldSnapshot\.AddDynamic/.test(setup),
          'مشترك في OnWorldSnapshot (المسيرة)',
          'بلا هذا الاشتراك لا تُرصد المسيرة فتبقى الخطوة الثالثة عالقة أبداً'),
        test(/LastRenderedStep/.test(wc),
          'حرس ضد إعادة الرسم مع كل نبضة شبكة',
          'بطاقة تُعيد الانزلاق كل بثّ تصير وميضاً مزعجاً لا إرشاداً'),
        test(/if \(Step == LastRenderedStep\)/.test(wc), 'الخروج المبكر عند عدم التغيّر'),
      ];
    },
    breaks: [
      {
        why: 'حذف اشتراك لقطة العالم يترك خطوة المسيرة عالقة أبداً',
        edits: { [WC]: (s) => replaceUnique(s, 'Api->OnWorldSnapshot.AddDynamic(this, &URok2OnboardingWidget::OnWorldSnapshotHandler);', '// removed') },
      },
      {
        why: 'حذف حرس إعادة الرسم يعيد الحركة مع كل بثّ',
        edits: { [WC]: (s) => replaceUnique(s, '\tif (Step == LastRenderedStep)\n\t{\n\t\treturn;\n\t}', '\t{\n\t}') },
      },
    ],
  },

  {
    id: 'F12',
    title: 'الخطوط بأدوار لا أحجام، والثوابت بلا تعريف في الهيدر',
    run: (vfs) => {
      const wc = code(vfs, WC);
      const oh = code(vfs, OH);
      return [
        test(/URok2Typography::ApplyFont\(/.test(wc), 'البطاقة تطلب أدواراً من نظام الخطوط'),
        test(!/\.Size\s*=\s*\d/.test(wc), 'لا حجم خط سحري في الطبقة',
          'P6-T2: الودجات تطلب دوراً لا رقماً'),
        test(/ERok2TextRole::Caption/.test(wc),
          'لافتة «من» بدور Caption (وجه عربي) لا Numeric',
          'الوجه الرقمي Cinzel بلا محارف عربية فتسقط الكلمة بصمت'),
        test(!/ERok2TextRole::(Numeric|Timer)/.test(wc),
          'لا وجه رقمي على نص يخلط العربية بالأرقام'),
        test(/extern\s+ROK2_API\s+const\s+FName/.test(oh),
          'أسماء المراسي مُعلنة extern في الهيدر'),
        test(!/^\s*const\s+(FName|FString)\s+\w+\s*\(/m.test(oh),
          'لا تعريف FName/FString في الهيدر العام',
          'const عند نطاق النطاق = ربط داخلي: نسخة ومُهيِّئ ساكن لكل وحدة ترجمة'),
        test(/namespace Rok2FtueSpec\s*\{[\s\S]*?\}/.test(code(vfs, OC)),
          'التعريف الوحيد للثوابت في ملف التنفيذ'),
      ];
    },
    breaks: [
      {
        why: 'حجم خط سحري في الطبقة',
        edits: { [WC]: (s) => replaceUnique(s, 'URok2Typography::ApplyFont(TitleText, ERok2TextRole::Title);', 'FSlateFontInfo F = TitleText->GetFont(); F.Size = 22; TitleText->SetFont(F);') },
      },
      {
        why: 'تعريف FName في الهيدر العام بدل extern',
        edits: { [OH]: (s) => replaceUnique(s, '\textern ROK2_API const FName AnchorBuild;', '\tconst FName AnchorBuild(TEXT("Ftue.Build"));') },
      },
      {
        why: 'وجه رقمي على لافتة تخلط العربية بالأرقام',
        edits: { [WC]: (s) => replaceUnique(s, 'URok2Typography::ApplyFont(OrdinalText, ERok2TextRole::Caption);', 'URok2Typography::ApplyFont(OrdinalText, ERok2TextRole::Numeric);') },
      },
    ],
  },

  {
    id: 'F13',
    title: 'لا قيم لعب ثابتة في الكود (AGENTS.md §3)',
    run: (vfs) => {
      const oc = code(vfs, OC);
      const farm = /FarmBuildingId\(TEXT\("([^"]+)"\)\)/.exec(oc);
      const json = get(vfs, BUILDINGS);
      const r = [];
      r.push(test(!!farm, 'معرّف المزرعة معرَّف كثابت واحد'));
      if (farm) {
        r.push(test(new RegExp(`"id"\\s*:\\s*"${farm[1]}"`).test(json),
          `معرّف المزرعة «${farm[1]}» موجود في data/buildings.json`,
          'معرّف لا يطابق البيانات = خطوة لا تكتمل أبداً'));
      }
      r.push(test(!/Count\s*[<>]=?\s*[1-9]\d+/.test(oc),
        'لا أعداد جنود أو تكاليف مُقحَمة في منطق الإرشاد'));
      r.push(test(/QueueTypeBuilding|QueueTypeTraining/.test(oc),
        'أنواع الطوابير ثوابت مسمّاة لا نصوص متفرقة'));
      return r;
    },
    breaks: [
      {
        why: 'معرّف مبنى لا يطابق data/buildings.json فلا تكتمل الخطوة أبداً',
        edits: { [OC]: (s) => replaceUnique(s, 'FarmBuildingId(TEXT("farm"))', 'FarmBuildingId(TEXT("wheat_field"))') },
      },
      {
        why: 'المعرّف صحيح لكن البيانات تغيّرت (الحرس يقيس التطابق لا النص)',
        edits: { [BUILDINGS]: (s) => replaceUnique(s, '"id": "farm"', '"id": "farmstead"') },
      },
    ],
  },
];

// ---------------------------------------------------------------------------
// المُشغِّل
// ---------------------------------------------------------------------------

const REAL = readTree();

for (const key of [OH, OC, WH, WC, GC, GH, HUD, ICONS, BUILDINGS]) {
  if (!REAL.has(key)) {
    console.error(`❌ ملف مفقود من الشجرة: ${key}`);
    process.exit(1);
  }
}

console.log('═'.repeat(62));
console.log('verify_ui_ftue — P6-T4 إرشاد الدقيقة الأولى (حرس بطورين)');
console.log('═'.repeat(62));
console.log('الطور 1 — إثبات: المدققون على الشجرة الحقيقية');
console.log('═'.repeat(62));

let passed = 0, failed = 0;
const failures = [];

for (const d of DETECTORS) {
  console.log(`\n[${d.id}] ${d.title}`);
  let results;
  try {
    results = d.run(REAL);
  } catch (e) {
    console.error(`  ❌ المدقق رمى استثناءً — ${e.message}`);
    failed++; failures.push(`${d.id} threw: ${e.message}`);
    continue;
  }
  for (const r of results) {
    if (r.ok) {
      passed++;
      if (VERBOSE) console.log(`  ✅ ${r.name}`);
    } else {
      failed++;
      failures.push(`${d.id}: ${r.name}${r.detail ? ' — ' + r.detail : ''}`);
      console.error(`  ❌ ${r.name}${r.detail ? ' — ' + r.detail : ''}`);
    }
  }
  if (!VERBOSE && results.every((r) => r.ok)) console.log(`  ✅ ${results.length}/${results.length}`);
}

console.log('\n' + '═'.repeat(62));
console.log('الطور 2 — نفي: كل مدقق على عطله (يجب أن يفشل)');
console.log('═'.repeat(62));

let negOk = 0, negBad = 0;
for (const d of DETECTORS) {
  for (const b of d.breaks ?? []) {
    let detected, note = '';
    try {
      if (b.selfCheck) detected = b.selfCheck();
      else detected = d.run(mutate(REAL, b.edits)).some((r) => !r.ok);
    } catch (e) {
      detected = true; note = ` (باستثناء: ${e.message})`;
    }
    if (detected) { negOk++; console.log(`  ✅ [${d.id}] يرصد: ${b.why}${note}`); }
    else {
      negBad++;
      console.error(`  ❌ [${d.id}] لا يرصد: ${b.why} — الحرس نفسه معطوب`);
      failures.push(`${d.id} negative test did not fire: ${b.why}`);
    }
  }
}

console.log('\n' + '═'.repeat(62));
console.log(`الطور 1 (إثبات): ${passed} ناجح، ${failed} فاشل`);
console.log(`الطور 2 (نفي):   ${negOk} مدققاً رصد عطله، ${negBad} لم يرصد`);
console.log('تحفّظ: تحقق بنيوي بلا مترجم C++ — ولا حكم على الشكل النهائي على الشاشة.');
console.log('═'.repeat(62));

if (failed || negBad) {
  console.log('\n❌ FAILED');
  for (const f of failures) console.log(`   • ${f}`);
  process.exit(1);
}
console.log(`\n✅ ALL PASSED (${passed} فحصاً + ${negOk} اختبار نفي)`);
process.exit(0);
