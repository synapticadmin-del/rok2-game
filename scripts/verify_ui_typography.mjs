#!/usr/bin/env node
/**
 * verify_ui_typography.mjs — P6-T2 نظام الخطوط الموحّد: حرس بنيوي بطورين
 *
 * ما يحرسه: أن الودجات تطلب **دوراً** لا حجماً، وأن كل دور له وجه ووزن وحجم
 * في مكان واحد، وأن السلم لا ينكسر، وأن التدهور اللطيف قائم، وأن الخط لا
 * يتسرّب إلى اللون.
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  الطورَان — ولماذا الطور الثاني ليس ترفاً
 * ────────────────────────────────────────────────────────────────────────────
 *   الطور 1 (إثبات): المدققون على الشجرة الحقيقية → يجب أن ينجحوا كلهم
 *   الطور 2 (نفي):   كل مدقق على العطل الذي يحرسه → يجب أن يفشل كل مرة
 *
 * فحصٌ بتعبير نمطي خاطئ «ينجح» على شجرة سليمة وعلى شجرة معطوبة سواءً. مدقق
 * ينجح في الطور 1 ولا يفشل في الطور 2 يُحتسب **عطلاً في الحرس نفسه** لا نجاحاً.
 *
 * تحفّظ: لا مترجم C++ هنا — تحقق بنيوي، ولا ادّعاء ببناء ناجح ولا بشكل نهائي
 * على الشاشة (الأخير يحتاج عيناً على اللعبة).
 *
 * Usage: node scripts/verify_ui_typography.mjs [--verbose]
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join, basename } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const MOD = join(__dirname, '..', 'game', 'client-unreal', 'Source', 'Rok2');
const VERBOSE = process.argv.includes('--verbose');

/**
 * تعليقات السطر أولاً ثم الكتل — الترتيب المعكوس عطل كامن: تعليق سطر يحتوي
 * فاتحة كتلة يفتح كتلة وهمية تمحو الكود الحقيقي بينها وأول خاتمة، فيجري الفحص
 * على فراغ ويُبلّغ نجاحاً كاذباً. (نفس العطل الذي أُصلح في verify_delegate_bind.)
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
  return vfs;
}

const get = (vfs, k) => vfs.get(k) ?? '';
const code = (vfs, k) => stripComments(get(vfs, k));
const H = 'Public/Rok2Typography.h';
const C = 'Private/Rok2Typography.cpp';

/** ملفات الودجات — كل شيء عدا مكتبة الخطوط نفسها */
const widgets = (vfs) =>
  [...vfs.keys()].filter((k) => k.startsWith('Private/') && !k.startsWith('Private/Rok2Typography'));

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
 * طفرة نصّية بمرساة **فريدة**. الالتباس عطل حقيقي وقعتُ فيه: النص
 * `case Body:\n case BodySmall:` يظهر في FaceOf و WeightOf معاً، فطفرة
 * وُصفت لـ WeightOf أصابت FaceOf وحده ولم يرصدها أحد — واختبار نفي يعدّل
 * الموضع الخطأ يُعطي طمأنينة كاذبة تماماً كفحص بتعبير نمطي خاطئ.
 */
function replaceUnique(src, needle, replacement) {
  const n = src.split(needle).length - 1;
  if (n === 0) throw new Error(`anchor not found: ${needle.slice(0, 50)}`);
  if (n > 1) throw new Error(`anchor is ambiguous (${n} matches): ${needle.slice(0, 50)}`);
  return src.replace(needle, replacement);
}

/** يستخرج أسماء الأدوار من التعداد */
function roleNames(vfs) {
  const m = /enum class ERok2TextRole : uint8\s*\{([\s\S]*?)\n\}/.exec(code(vfs, H));
  if (!m) return [];
  return [...m[1].matchAll(/^\s*(\w+),?\s*$/gm)].map((x) => x[1]);
}

/** يستخرج درجات السلم كأزواج اسم→قيمة بترتيب الظهور */
function scaleSteps(vfs) {
  const body = code(vfs, H);
  const ns = /namespace Rok2TypeScale\s*\{([\s\S]*?)\n\}/.exec(body);
  if (!ns) return [];
  return [...ns[1].matchAll(/static constexpr float\s+(\w+)\s*=\s*([\d.]+)f/g)]
    .map((m) => ({ name: m[1], px: parseFloat(m[2]) }))
    .filter((s) => s.name !== 'Min' && s.name !== 'Max');
}

/** جسم دالة تحويل الدور (FaceOf/SizeOf/WeightOf) */
function switchBody(vfs, fn) {
  const m = new RegExp(`${fn}\\(ERok2TextRole Role\\)\\s*\\{([\\s\\S]*?)\\n\\}`).exec(code(vfs, C));
  return m ? m[1] : '';
}

const DETECTORS = [];
const detector = (id, title, run, breaks) => DETECTORS.push({ id, title, run, breaks });

// —— S0: سلامة أداة الفحص ————————————————————————————————
detector(
  'S0',
  'سلامة stripComments (تعليق سطر يحتوي فاتحة كتلة)',
  () => {
    const star = '*';
    const snippet = `// glob /Engine/BasicShapes/${star} here\nERok2TextRole::Title\n/${star} block ${star}/\n`;
    const out = stripComments(snippet);
    return [
      { name: 'الكود بعد تعليق سطر يحتوي فاتحة كتلة يبقى', ok: /ERok2TextRole::Title/.test(out) },
      { name: 'تعليقات الكتل تُمحى', ok: !/block/.test(out) },
    ];
  },
  [
    {
      why: 'الترتيب المعكوس (الكتل قبل السطر)',
      selfCheck() {
        const star = '*';
        const bad = (s) => s.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
        const snippet = `// glob /Engine/BasicShapes/${star} here\nERok2TextRole::Title\n/${star} block ${star}/\n`;
        return !/ERok2TextRole::Title/.test(bad(snippet));
      },
    },
  ]
);

// —— T1: المكتبة موجودة وكل دور مغطّى في كل تحويل ————————————————
detector(
  'T1',
  'كل دور نصي مغطّى في FaceOf و SizeOf و WeightOf و AllRoles',
  (vfs) => {
    const roles = roleNames(vfs);
    const out = [{ name: `التعداد يعرّف أدواراً (${roles.length})`, ok: roles.length >= 8 }];
    for (const fn of ['FaceOf', 'SizeOf', 'WeightOf']) {
      const body = switchBody(vfs, fn);
      const missing = roles.filter((r) => !body.includes(`ERok2TextRole::${r}`));
      out.push({
        name: `${fn} يغطّي كل الأدوار`,
        ok: body.length > 0 && missing.length === 0,
        detail: missing.join(', '),
      });
    }
    const all = /AllRoles\(\)\s*\{([\s\S]*?)\n\}/.exec(code(vfs, C));
    const allBody = all ? all[1] : '';
    const missAll = roles.filter((r) => !allBody.includes(`ERok2TextRole::${r}`));
    out.push({ name: 'AllRoles يسرد كل الأدوار', ok: !!all && missAll.length === 0, detail: missAll.join(', ') });
    return out;
  },
  [
    {
      why: 'دور جديد بلا تغطية في SizeOf (يقع على default بصمت)',
      edits: { [H]: (s) => s.replace(/(\tTimer\n\};)/, '\tTimer,\n\tGhostRole\n};') },
    },
  ]
);

// —— T2: السلم تصاعدي وداخل المدى ————————————————————————
detector(
  'T2',
  'سلم الأحجام تصاعدي وداخل المدى المعلن',
  (vfs) => {
    const steps = scaleSteps(vfs);
    const body = code(vfs, H);
    const min = parseFloat((/Min\s*=\s*([\d.]+)f/.exec(body) || [])[1]);
    const max = parseFloat((/Max\s*=\s*([\d.]+)f/.exec(body) || [])[1]);
    const px = steps.map((s) => s.px);
    const ascending = px.every((v, i) => i === 0 || v > px[i - 1]);
    return [
      { name: `السلم يعرّف درجات (${steps.length})`, ok: steps.length >= 6 },
      { name: `الدرجات تصاعدية بترتيب التعريف [${px.join(', ')}]`, ok: ascending },
      { name: 'كل درجة داخل [Min, Max]', ok: Number.isFinite(min) && Number.isFinite(max) && px.every((v) => v >= min && v <= max) },
      // الحرس المزدوج: static_assert يمنع الانكسار وقت البناء لا وقت المراجعة
      { name: 'static_assert يحرس التراتب في الهيدر', ok: /static_assert\([\s\S]*?Rok2TypeScale::/.test(body) },
    ];
  },
  [
    {
      why: 'كسر التراتب (Body أصغر من Caption)',
      edits: { [H]: (s) => s.replace(/(static constexpr float\s+Body\s*=\s*)[\d.]+f/, '$19.f') },
    },
  ]
);

// —— T3: لا حجم خط خام في الودجات ————————————————————————
detector(
  'T3',
  'لا ضبط خط خام في الودجات (الأنماط الأربعة التي كانت موجودة)',
  (vfs) => {
    // الاستثناء الوحيد الموثّق: MakeIconLabel مساعد عام حجمه من المستدعي
    const ALLOWED = new Set(['Private/Rok2IconLibrary.cpp']);
    const PATTERNS = [
      ['GetFont() ثم .Size', /FSlateFontInfo\s+\w+\s*=\s*\w+->GetFont\(\)/],
      ['مساعد حجم محلي', /\b(?:Rok2Font|CardFont|BuildFont)\s*\(/],
      ['FCoreStyle مباشرة', /FCoreStyle::GetDefaultFontStyle\s*\(/],
      ['كتابة مباشرة على Font.Size', /->Font\.Size\s*=/],
    ];
    const out = [];
    let scanned = 0;
    for (const k of widgets(vfs)) {
      if (ALLOWED.has(k)) continue;
      const body = code(vfs, k);
      scanned++;
      for (const [label, re] of PATTERNS) {
        if (re.test(body)) out.push({ name: `${basename(k)}: ${label}`, ok: false });
      }
    }
    if (out.length) return out;
    return [{ name: `لا نمط خام في ${scanned} ودجت`, ok: scanned > 0, detail: scanned ? '' : 'لم تُفحص ملفات' }];
  },
  [
    {
      why: 'إرجاع ضبط حجم خام في ودجت',
      edits: {
        'Private/Rok2HudWidget.cpp': (s) =>
          s.replace(
            /URok2Typography::ApplyFont\(SeasonText, ERok2TextRole::Numeric\);/,
            'FSlateFontInfo Sf = SeasonText->GetFont(); Sf.Size = 13; SeasonText->SetFont(Sf);'
          ),
      },
    },
  ]
);

// —— T4: كل مستدعٍ يضمّن الهيدر ————————————————————————
detector(
  'T4',
  'كل ملف يستدعي URok2Typography يضمّن هيدرها',
  (vfs) => {
    const out = [];
    let users = 0;
    for (const k of widgets(vfs)) {
      const raw = get(vfs, k);
      if (!/\bURok2Typography::/.test(stripComments(raw))) continue;
      users++;
      out.push({ name: `${basename(k)} يضمّن Rok2Typography.h`, ok: raw.includes('#include "Rok2Typography.h"') });
    }
    out.push({ name: `عدد المستدعين معقول (${users})`, ok: users >= 5 });
    return out;
  },
  [
    {
      why: 'استدعاء بلا تضمين (يفشل الترجمة)',
      edits: { 'Private/Rok2MarchPanel.cpp': (s) => s.replace(/#include "Rok2Typography\.h"\n/, '') },
    },
  ]
);

// —— T5: الأوجه الثلاثة ومسارها ————————————————————————
detector(
  'T5',
  'الأوجه الثلاثة معرّفة بأسماء مميّزة ومسار على اصطلاح المشروع',
  (vfs) => {
    const c = code(vfs, C);
    const names = [...c.matchAll(/return TEXT\("(Rok2(?:Display|Ui|Numeric))"\)/g)].map((m) => m[1]);
    const uniq = new Set(names);
    return [
      { name: 'ERok2Face يعرّف Display و Ui و Numeric', ok: /enum class ERok2Face : uint8[\s\S]*?Display[\s\S]*?Ui[\s\S]*?Numeric/.test(code(vfs, H)) },
      { name: `أسماء أصول مميّزة للأوجه (${[...uniq].join(', ')})`, ok: uniq.size === 3 },
      // أصول FontFace تسكن /Game/Fonts/Faces على نفس اصطلاح
      // URok2ArtAssets::EditorPackagePath — /Game/<dir>/<name>.<name>
      { name: 'المسار على اصطلاح /Game/<dir>/<name>.<name>', ok: /\/Game\/Fonts\/Faces\/%s\.%s/.test(c) },
    ];
  },
  [
    {
      why: 'وجهان يشتركان في نفس الأصل (يضيع تمييز الرقم عن النص)',
      edits: { [C]: (s) => s.replace('return TEXT("Rok2Numeric");', 'return TEXT("Rok2Ui");') },
    },
  ]
);

// —— T6: التدهور اللطيف ————————————————————————————————
detector(
  'T6',
  'تدهور لطيف عند غياب أصل الخط (لا اعتماد على استيراد)',
  (vfs) => {
    const c = code(vfs, C);
    // القصر على جسم Font() مقصود: FontSized تحتوي نداءً مماثلاً، وفحصٌ على
    // مستوى الملف يبقى ناجحاً حتى لو فُقد الرجوع من Font() نفسها.
    const fnBody = (/FSlateFontInfo URok2Typography::Font\(ERok2TextRole Role\)\s*\{([\s\S]*?)\n\}/.exec(c) || [, ''])[1];
    const sizedBody = (/FSlateFontInfo URok2Typography::FontSized\([\s\S]*?\)\s*\{([\s\S]*?)\n\}/.exec(c) || [, ''])[1];
    return [
      { name: 'يحاول تحميل أصول الأوجه عبر LoadObject', ok: /LoadObject<UObject>\(nullptr, \*FaceAssetPath/.test(c) },
      { name: 'Font() نفسها ترجع إلى خط المحرك عند الغياب', ok: /FCoreStyle::GetDefaultFontStyle/.test(fnBody) },
      { name: 'FontSized ترجع إلى خط المحرك كذلك', ok: /FCoreStyle::GetDefaultFontStyle/.test(sizedBody) },
      { name: 'يخبّئ الفشل فلا يعيد المحاولة كل إطار', ok: /FaceMisses/.test(c) },
      { name: 'يقصر الأوزان غير المضمونة على خط المحرك', ok: /ClampWeightForEngineFont/.test(c) },
      // الخبأ صار FStandaloneCompositeFont (يرث FGCObject فيحمي FFontData
      // داخله)، وأصول الأوجه تُثبَّت بـUPROPERTY منفصل — فلا يجمعها الـGC
      // بينما الخط المركّب يشير إليها.
      { name: 'أصول الأوجه محمية من الـGC بـ UPROPERTY', ok: /UPROPERTY\(\)\s*\n\s*TSet<UObject\*>\s+FaceAssets;/.test(code(vfs, H)) },
    ];
  },
  [
    {
      why: 'حذف مسار الرجوع إلى خط المحرك (تصير الواجهة رهينة استيراد أصل)',
      edits: { [C]: (s) => s.replace(/return FCoreStyle::GetDefaultFontStyle\(ClampWeightForEngineFont\(Weight\), Size\);/, 'return FSlateFontInfo();') },
    },
  ]
);

// —— T7: الخط لا يلمس اللون ————————————————————————————
detector(
  'T7',
  'مكتبة الخطوط لا تلمس اللون (الألوان تبقى لكل ودجت)',
  (vfs) => {
    const c = code(vfs, C);
    const h = code(vfs, H);
    return [
      { name: 'لا SetColorAndOpacity في المكتبة', ok: !/SetColorAndOpacity/.test(c) },
      { name: 'لا FSlateColor في واجهة المكتبة', ok: !/FSlateColor/.test(h) },
      { name: 'ApplyFont يستدعي SetFont فقط', ok: /ApplyFont\(UTextBlock\* Text, ERok2TextRole Role\)\s*\{[\s\S]*?Text->SetFont\(Font\(Role\)\);[\s\S]*?\n\}/.test(c) },
      { name: 'ApplyFont آمن مع nullptr', ok: /ApplyFont\([\s\S]*?if\s*\(!Text\)/.test(c) },
    ];
  },
  [
    {
      why: 'تسريب اللون إلى مكتبة الخطوط',
      edits: {
        [C]: (s) => s.replace('	Text->SetFont(Font(Role));', '	Text->SetFont(Font(Role));\n	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));'),
      },
    },
  ]
);

// —— T8: لا دور معلن بلا مستدعٍ ————————————————————————
detector(
  'T8',
  'كل دور معلن يستخدمه ودجت فعلاً (لا واجهة تخمينية)',
  (vfs) => {
    const roles = roleNames(vfs);
    const used = new Set();
    for (const k of widgets(vfs)) {
      for (const m of code(vfs, k).matchAll(/ERok2TextRole::(\w+)/g)) used.add(m[1]);
    }
    const unused = roles.filter((r) => !used.has(r));
    return [
      { name: `كل دور له مستدعٍ (${used.size}/${roles.length})`, ok: unused.length === 0, detail: unused.length ? 'بلا مستدعٍ: ' + unused.join(', ') : '' },
      { name: 'لا دور مستخدم غير معلن', ok: [...used].every((u) => roles.includes(u)), detail: [...used].filter((u) => !roles.includes(u)).join(', ') },
    ];
  },
  [
    {
      why: 'دور معلن لا يستخدمه أحد (واجهة تتعفّن)',
      edits: { [H]: (s) => s.replace(/(\tTimer\n\};)/, '\tTimer,\n\tUnusedGhost\n};') },
    },
  ]
);

// —— T9: الأرقام بالوجه الرقمي والعربي بالوجه العربي ————————————
detector(
  'T9',
  'الأدوار الرقمية على الوجه الرقمي وحدها',
  (vfs) => {
    const body = switchBody(vfs, 'FaceOf');
    // القسم الذي يعيد الوجه الرقمي
    const seg = /((?:\s*case ERok2TextRole::\w+:)+)\s*\n\s*return ERok2Face::Numeric;/.exec(body);
    const numeric = seg ? [...seg[1].matchAll(/ERok2TextRole::(\w+)/g)].map((m) => m[1]).sort() : [];
    const expected = ['Numeric', 'Timer'];
    return [
      { name: `الوجه الرقمي لـ [${numeric.join(', ')}] فقط`, ok: JSON.stringify(numeric) === JSON.stringify(expected), detail: `المتوقع ${expected.join(', ')}` },
      { name: 'الوجه الفخم لدور Display', ok: /case ERok2TextRole::Display:\s*\n\s*return ERok2Face::Display;/.test(body) },
      { name: 'بقية الأدوار على الوجه العربي (default = Ui)', ok: /default:\s*\n\s*return ERok2Face::Ui;/.test(body) },
    ];
  },
  [
    {
      why: 'وضع عنوان عربي على الوجه الرقمي',
      edits: {
        [C]: (s) => s.replace('	case ERok2TextRole::Numeric:\n	case ERok2TextRole::Timer:\n		return ERok2Face::Numeric;',
                              '	case ERok2TextRole::Numeric:\n	case ERok2TextRole::Timer:\n	case ERok2TextRole::Title:\n		return ERok2Face::Numeric;'),
      },
    },
  ]
);

// —— T10: الأوزان منطقية ————————————————————————————————
detector(
  'T10',
  'الأوزان: العنوان الأكبر أثقل، والجسم عادي',
  (vfs) => {
    const body = switchBody(vfs, 'WeightOf');
    const h = code(vfs, H);
    return [
      { name: 'Display أثقل وزن (Black)', ok: /case ERok2TextRole::Display:\s*\n\s*return Rok2TypeWeight::Black;/.test(body) },
      { name: 'الجسم عادي (Regular)', ok: /case ERok2TextRole::Body:[\s\S]{0,220}?return Rok2TypeWeight::Regular;/.test(body) },
      { name: 'الأوزان الثلاثة معلنة extern في الهيدر', ok: /extern ROK2_API const FName Black/.test(h) && /extern ROK2_API const FName Bold/.test(h) && /extern ROK2_API const FName Regular/.test(h) },
      // extern لا static: static في هيدر يعطي نسخة لكل وحدة ترجمة
      { name: 'الأوزان معرّفة مرة واحدة في الـcpp', ok: /namespace Rok2TypeWeight\s*\{[\s\S]*?const FName Black\(TEXT\("Black"\)\);/.test(code(vfs, C)) },
    ];
  },
  [
    {
      why: 'الجسم بوزن ثقيل (يضيع التمييز بين عنوان ونص)',
      edits: {
        // مرساة تضمّ سطر الإرجاع فتصير فريدة على WeightOf وحدها
        [C]: (s) => replaceUnique(
          s,
          '	case ERok2TextRole::Body:\n	case ERok2TextRole::BodySmall:\n	case ERok2TextRole::Timer:',
          '	case ERok2TextRole::BodySmall:\n	case ERok2TextRole::Timer:'
        ),
      },
    },
  ]
);

// —— T11: أثر P6-T1 محفوظ ————————————————————————————
detector(
  'T11',
  'لا إيموجي في ملفات النظام (يحفظ إنجاز P6-T1)',
  (vfs) => {
    const EMOJI = /[\u{1F000}-\u{1FAFF}\u{2B00}-\u{2BFF}\u{FE0F}]/u;
    const out = [];
    for (const k of [H, C]) {
      out.push({ name: `${basename(k)} بلا إيموجي`, ok: !EMOJI.test(get(vfs, k)) });
    }
    return out;
  },
  [
    { why: 'إيموجي في ملف النظام', edits: { [H]: (s) => s + '\n// \u{1F525}\n' } },
  ]
);

// —— T12: لا اسم دور مهجور ————————————————————————————
detector(
  'T12',
  'لا إشارة إلى اسم دور مهجور (HudTitle بعد إعادة التسمية)',
  (vfs) => {
    const stale = [];
    for (const k of [...widgets(vfs), H, C]) {
      if (/\bHudTitle\b/.test(code(vfs, k))) stale.push(basename(k));
    }
    return [{ name: 'لا إشارة إلى HudTitle', ok: stale.length === 0, detail: stale.join(', ') }];
  },
  [
    {
      why: 'بقاء اسم مهجور بعد إعادة التسمية',
      edits: { 'Private/Rok2HudWidget.cpp': (s) => s.replace('ERok2TextRole::TitleCompact', 'ERok2TextRole::HudTitle') },
    },
  ]
);

// ───────────────────────────────────────────────────────────────────────────
const REAL = readTree();
if (!REAL.has(H) || !REAL.has(C)) {
  console.error('❌ لم يُعثر على مكتبة الخطوط — شغّل السكربت من جذر المستودع');
  process.exit(1);
}

let passed = 0, failed = 0;
const failures = [];

console.log('P6-T2 نظام الخطوط الموحّد — حرس بنيوي');
console.log(`الشجرة: ${REAL.size} ملفاً · الودجات: ${widgets(REAL).length}\n`);

console.log('═'.repeat(62));
console.log('الطور 1 — إثبات: المدققون على الشجرة الحقيقية');
console.log('═'.repeat(62));

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
