#!/usr/bin/env node
/**
 * verify_civ_lore.mjs — P6-T5 النَفَس القصصي للحضارات: حرس بنيوي بطورين
 *
 * ما يحرسه: أن النصّ مؤلَّف مرة واحدة في data/civilizations.json ويصل **للطرفين**
 * بلا انحراف — نسخة الـbackend طبق الأصل، والنسخة المدمجة في العميل مطابقة
 * حرفياً، والقائمة التي يختار منها اللاعب مشتقّة من الملف لا مكتوبة في الكود،
 * والنبذة تظهر عند الاختيار وفي شاشة المعلومات، والتحية مرة واحدة لا مع كل
 * نبضة شبكة، ولا رقم توازن مُقحَم في نصٍّ أدبي.
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
 * على الشاشة، ولا حكم على **جودة** النصّ الأدبي (تلك عينُ قارئ لا تعبير نمطي).
 *
 * Usage: node scripts/verify_civ_lore.mjs [--verbose]
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

// المسار يُحلّ نسبةً إلى **جذر المستودع** لا إلى مجلد السكربت. الأربعة
// المعطوبة في DEBT-1 تستخدم `const ROOT = __dirname` أي تتوقع الملفات بجانبها
// في scripts/ — فتفشل جميعاً بلا علاقة بالكود المفحوص. هذا هو الاصطلاح الصحيح.
const __dirname = fileURLToPath(new URL('.', import.meta.url));
const ROOT = join(__dirname, '..');
const MOD = join(ROOT, 'game', 'client-unreal', 'Source', 'Rok2');
const VERBOSE = process.argv.includes('--verbose');

/**
 * تعليقات السطر أولاً ثم الكتل — الترتيب المعكوس عطل كامن: تعليق سطر يحتوي
 * فاتحة كتلة يفتح كتلة وهمية تمحو الكود الحقيقي بينها وأول خاتمة، فيجري الفحص
 * على فراغ ويُبلّغ نجاحاً كاذباً. (العطل الذي أُصلح في verify_delegate_bind
 * وحُرِس في verify_ui_typography/verify_ui_ftue — ويحرسه هنا S0.)
 */
function stripComments(src) {
  return src.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
}

const DATA = 'Data/civilizations.json';
const BACKEND_DATA = 'Backend/civilizations.json';
const GAMEDATA = 'Backend/gameData.ts';
const ROUTER = 'Backend/router.ts';
const LH = 'Public/Rok2CivLore.h';
const LC = 'Private/Rok2CivLore.cpp';
const IH = 'Public/Rok2CivInfoWidget.h';
const IC = 'Private/Rok2CivInfoWidget.cpp';
const BC = 'Private/Rok2BootWidget.cpp';
const BH = 'Public/Rok2BootWidget.h';
const BLIB = 'Private/Rok2BlueprintLibrary.cpp';
const API_C = 'Private/Rok2Api.cpp';
const API_H = 'Public/Rok2Api.h';
const HUD_C = 'Private/Rok2HudWidget.cpp';
const HUD_H = 'Public/Rok2HudWidget.h';
const GM_C = 'Private/Rok2GameMode.cpp';
const GM_H = 'Public/Rok2GameMode.h';
const TYPES = 'Public/Rok2Types.h';
const ICONS = 'Private/Rok2IconLibrary.cpp';

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
  // ملفات خارج الموديول تدخل الـVFS لتكون الطفرات قادرة على مسّها أيضاً —
  // فيقيس الحرس **التطابق** بين الطرفين لا وجود نصٍّ في أحدهما.
  for (const [key, rel] of [
    [DATA, join('data', 'civilizations.json')],
    [BACKEND_DATA, join('game', 'backend', 'src', 'data', 'civilizations.json')],
    [GAMEDATA, join('game', 'backend', 'src', 'lib', 'gameData.ts')],
    [ROUTER, join('game', 'backend', 'src', 'http', 'router.ts')],
  ]) {
    const p = join(ROOT, rel);
    if (existsSync(p)) vfs.set(key, readFileSync(p, 'utf8'));
  }
  return vfs;
}

const get = (vfs, k) => vfs.get(k) ?? '';
const code = (vfs, k) => stripComments(get(vfs, k));

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
 * طفرة نصّية بمرساة **فريدة**. الالتباس عطل حقيقي: طفرة وُصفت لموضع أصابت
 * آخر يشترك معه في النص، فمرّت دون أن يرصدها أحد — واختبار نفي يعدّل الموضع
 * الخطأ يُعطي طمأنينة كاذبة تماماً كفحص بتعبير نمطي خاطئ.
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

function parseData(vfs, key = DATA) {
  try {
    return JSON.parse(get(vfs, key));
  } catch {
    return null;
  }
}

function civs(vfs, key = DATA) {
  const d = parseData(vfs, key);
  return Array.isArray(d?.civilizations) ? d.civilizations : [];
}

/** حدود المواصفة من الهيدر — لا أرقام مكرَّرة هنا */
function spec(vfs) {
  const h = code(vfs, LH);
  const num = (name) => {
    const m = new RegExp(`constexpr\\s+int32\\s+${name}\\s*=\\s*(\\d+)`).exec(h);
    return m ? Number(m[1]) : null;
  };
  return {
    min: num('MinStoryLines'),
    max: num('MaxStoryLines'),
    count: num('ExpectedCivCount'),
  };
}

/**
 * نداءات Add في BuildDefaults — النسخة المدمجة. نستخرج كل نصوصها بالترتيب
 * لنقارنها بالملف قيمةً بقيمة.
 */
function embeddedEntries(vfs) {
  const src = code(vfs, LC);
  const m = /void URok2CivLore::BuildDefaults\(\)([\s\S]*)$/.exec(src);
  if (!m) return [];
  const body = m[1];
  const out = [];
  // Add(TEXT(id), TEXT(name_ar), TEXT(name), TEXT(fantasy_ar),
  //     TEXT(fantasy), TEXT(unit), { story... }, TEXT(greeting), { hints... });
  const re = /\bAdd\(\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*TEXT\("([^"]*)"\)\s*,\s*\{([\s\S]*?)\}\s*,\s*TEXT\("([^"]*)"\)\s*,\s*\{([\s\S]*?)\}\s*\)\s*;/g;
  const lines = (blob) => [...blob.matchAll(/TEXT\("([^"]*)"\)/g)].map((x) => x[1]);
  for (const c of body.matchAll(re)) {
    out.push({
      id: c[1], name_ar: c[2], name: c[3], fantasy_ar: c[4],
      fantasy: c[5], unit: c[6], story: lines(c[7]),
      greeting: c[8], hints: lines(c[9]),
    });
  }
  return out;
}

/** جسم دالة بالاسم (توازن أقواس معقوفة) */
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

/** كل النصوص الأدبية في البيانات (نبذة + تحية + تلميحات) */
function allProse(vfs) {
  const out = [];
  for (const c of civs(vfs)) {
    for (const l of c.story ?? []) out.push({ civ: c.id, field: 'story', text: l });
    if (c.greeting) out.push({ civ: c.id, field: 'greeting', text: c.greeting });
    for (const h of c.hints ?? []) out.push({ civ: c.id, field: 'hints', text: h });
  }
  return out;
}

function knownIcons(vfs) {
  return new Set([...code(vfs, ICONS).matchAll(/Map\.Add\(TEXT\("([a-z_0-9]+)"\)/g)].map((m) => m[1]));
}

// ---------------------------------------------------------------------------
// المدققون
// ---------------------------------------------------------------------------

const DETECTORS = [
  {
    id: 'S0',
    title: 'المُجرِّد نفسه سليم (تعليقات السطر قبل الكتل)',
    run() {
      const r = [];
      // العطل: لو جُرِّدت الكتل أولاً، فتعليق سطرٍ يحتوي "/*" يفتح كتلة وهمية
      // تمحو ما بعدها حتى أول "*/" — فيجري الفحص على فراغ ويُبلّغ نجاحاً.
      const sample = 'int keep_a = 1; // fake /* opener\nint keep_b = 2;\n/* real */ int keep_c = 3;';
      const out = stripComments(sample);
      r.push(test(out.includes('keep_a') && out.includes('keep_b') && out.includes('keep_c'),
        'تعليق سطر يحتوي فاتحة كتلة لا يمحو الكود بعده',
        `نتيجة التجريد: ${JSON.stringify(out)}`));
      r.push(test(!out.includes('fake') && !out.includes('real'),
        'التعليقات تُجرَّد فعلاً (لا تُترك في النص المفحوص)'));
      return r;
    },
    breaks: [
      {
        why: 'ترتيب معكوس في المُجرِّد يمحو الكود ويُبلّغ نجاحاً كاذباً',
        selfCheck() {
          const wrong = (s) => s.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
          const out = wrong('int keep_a = 1; // fake /* opener\nint keep_b = 2;\n/* real */ int keep_c = 3;');
          return !out.includes('keep_b');	// الكود ضاع ⇒ العطل حقيقي
        },
      },
    ],
  },

  {
    id: 'D1',
    title: 'البيانات: الحضارات الست تحمل طبقة النصّ كاملة',
    run(vfs) {
      const r = [];
      const s = spec(vfs);
      const list = civs(vfs);

      r.push(test(s.min !== null && s.max !== null && s.count !== null,
        'حدود المواصفة معرَّفة كثوابت في الهيدر', 'MinStoryLines/MaxStoryLines/ExpectedCivCount'));
      r.push(test(list.length === s.count,
        `عدد الحضارات ${list.length} يطابق ExpectedCivCount ${s.count}`));

      for (const c of list) {
        const tag = c.id ?? '(بلا معرّف)';
        r.push(test(typeof c.name_ar === 'string' && c.name_ar.trim().length > 0,
          `${tag}: name_ar موجود`));
        r.push(test(typeof c.fantasy_ar === 'string' && c.fantasy_ar.trim().length > 0,
          `${tag}: fantasy_ar موجود`));
        r.push(test(typeof c.greeting === 'string' && c.greeting.trim().length > 0,
          `${tag}: greeting موجود`));
        // مصفوفة لا نصّ واحد: مواضع القطع قرار تأليف لا لفٌّ آلي
        r.push(test(Array.isArray(c.story),
          `${tag}: story مصفوفة أسطر`, 'نصّ واحد طويل يُقطع آلياً حيث اتّفق فتنكسر الجملة'));
        if (Array.isArray(c.story)) {
          r.push(test(c.story.length >= s.min && c.story.length <= s.max,
            `${tag}: النبذة ${c.story.length} أسطر داخل المدى [${s.min}..${s.max}]`));
          r.push(test(c.story.every((l) => typeof l === 'string' && l.trim().length > 0),
            `${tag}: لا سطر فارغ في النبذة`));
        }
        r.push(test(Array.isArray(c.hints) && c.hints.length > 0
          && c.hints.every((h) => typeof h === 'string' && h.trim().length > 0),
          `${tag}: تلميحات غير فارغة`));
      }
      return r;
    },
    breaks: [
      {
        why: 'حضارة بنبذة من سطرين (أقلّ من المدى الذي تنصّ عليه المواصفة)',
        edits: {
          [DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations[0].story = d.civilizations[0].story.slice(0, 2);
            return JSON.stringify(d, null, 2);
          },
        },
      },
      {
        why: 'story صار نصّاً واحداً بدل مصفوفة أسطر',
        edits: {
          [DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations[1].story = d.civilizations[1].story.join(' ');
            return JSON.stringify(d, null, 2);
          },
        },
      },
      {
        why: 'حضارة بلا تحية',
        edits: {
          [DATA]: (s) => {
            const d = JSON.parse(s);
            delete d.civilizations[2].greeting;
            return JSON.stringify(d, null, 2);
          },
        },
      },
    ],
  },

  {
    id: 'D2',
    title: '«فيُخدم للطرفين»: نسخة الـbackend طبق الأصل',
    run(vfs) {
      const r = [];
      const a = get(vfs, DATA);
      const b = get(vfs, BACKEND_DATA);
      r.push(test(b.length > 0, 'نسخة الـbackend موجودة'));
      // تطابق **محتوى** لا حرفي: فاصلة سطر مختلفة ليست انحراف نصّ
      let same = false;
      try { same = JSON.stringify(JSON.parse(a)) === JSON.stringify(JSON.parse(b)); } catch { same = false; }
      r.push(test(same,
        'data/civilizations.json ونسخة backend متطابقتان',
        'انحراف النسختين يعني نصّاً يُقرأ عند العميل وآخر يخدمه الخادم'));
      return r;
    },
    breaks: [
      {
        why: 'نصّ الخادم انحرف عن نصّ المستودع (تعديل نسخة واحدة)',
        edits: {
          [BACKEND_DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations[0].greeting = 'تحية أخرى لا توجد في نسخة المستودع.';
            return JSON.stringify(d, null, 2);
          },
        },
      },
    ],
  },

  {
    id: 'D3',
    title: 'الخادم يخدم الحقول ولا يُرشّحها',
    run(vfs) {
      const r = [];
      const gd = code(vfs, GAMEDATA);
      const rt = code(vfs, ROUTER);

      r.push(test(/export function getCivilizations\(\)\s*\{\s*return civilizations;\s*\}/.test(gd),
        'getCivilizations يعيد الملف كما هو',
        'أي انتقاء حقول هنا يُسقط النصّ الأدبي قبل أن يصل العميل'));
      r.push(test(/path === "\/v1\/meta\/civilizations"/.test(rt),
        '/v1/meta/civilizations قائم'));
      r.push(test(/civilizations: getCivilizations\(\)/.test(rt),
        '/v1/meta/all يضمّ الحضارات (وهو ما يقرأه العميل عند البدء)'));
      // معرّفات الملف هي ما يقبله /v1/city/init — فلا معرّف يُعرض ويُرفض
      r.push(test(/getCivilizations\(\) as any\)\.civilizations/.test(rt)
        && /Unknown civilization/.test(rt),
        'تحقّق init يقارن بمعرّفات الملف نفسه'));
      return r;
    },
    breaks: [
      {
        why: 'الخادم يُرشّح الحقول فيسقط النصّ الأدبي قبل العميل',
        edits: {
          [GAMEDATA]: (s) => replaceUnique(s,
            'export function getCivilizations() {\n  return civilizations;\n}',
            'export function getCivilizations() {\n  return { civilizations: civilizations.civilizations.map((c: any) => ({ id: c.id, name: c.name })) };\n}'),
        },
      },
      {
        why: '/v1/meta/all لم يعد يضمّ الحضارات',
        edits: {
          [ROUTER]: (s) => replaceUnique(s, 'civilizations: getCivilizations(),', 'buildingsOnly: true,'),
        },
      },
    ],
  },

  {
    id: 'C1',
    title: 'النسخة المدمجة مطابقة للملف حرفياً',
    run(vfs) {
      const r = [];
      const file = civs(vfs);
      const emb = embeddedEntries(vfs);

      r.push(test(emb.length === file.length,
        `النسخة المدمجة ${emb.length} سجلاً بعدد الملف ${file.length}`,
        'نسخة ناقصة = حضارة لا تظهر في بناء Android (مجلد data/ خارج الحزمة)'));

      const byId = new Map(emb.map((e) => [e.id, e]));
      for (const c of file) {
        const e = byId.get(c.id);
        r.push(test(!!e, `${c.id}: موجودة في النسخة المدمجة`));
        if (!e) continue;
        r.push(test(e.name_ar === c.name_ar, `${c.id}: name_ar مطابق`,
          `مدمج «${e.name_ar}» ≠ ملف «${c.name_ar}»`));
        r.push(test(e.name === c.name, `${c.id}: name مطابق`));
        r.push(test(e.fantasy_ar === c.fantasy_ar, `${c.id}: fantasy_ar مطابق`));
        r.push(test(e.fantasy === c.fantasy, `${c.id}: fantasy مطابق`));
        r.push(test(e.unit === c.special_unit?.id, `${c.id}: معرّف الوحدة الخاصة مطابق`,
          `مدمج «${e.unit}» ≠ ملف «${c.special_unit?.id}»`));
        r.push(test(e.greeting === c.greeting, `${c.id}: التحية مطابقة`));
        r.push(test(JSON.stringify(e.story) === JSON.stringify(c.story),
          `${c.id}: النبذة مطابقة سطراً بسطر`));
        r.push(test(JSON.stringify(e.hints) === JSON.stringify(c.hints),
          `${c.id}: التلميحات مطابقة`));
      }
      return r;
    },
    breaks: [
      {
        why: 'سطر في النسخة المدمجة انحرف عن الملف (اللاعب يقرأ نصّاً غير المؤلَّف)',
        edits: {
          [LC]: (s) => replaceUnique(s,
            'TEXT("ارفع نسورك الذهبية: النظام أطول عمراً من الفتح.")',
            'TEXT("ارفع نسورك الذهبية: النظام اطول عمرا من الفتح.")'),
        },
      },
      {
        why: 'النصّ صحيح لكن الملف تغيّر (الحرس يقيس التطابق لا النصّ)',
        edits: {
          [DATA]: (s) => replaceUnique(s,
            'الصفّ منتظم يا حاكم، والقناطر تنتظر أمرك.',
            'الصفّ منتظم يا حاكم، والقناطر تنتظر إشارتك.'),
        },
      },
      {
        why: 'حضارة سقطت من النسخة المدمجة فتغيب في بناء Android',
        edits: {
          [LC]: (s) => replaceUnique(s, 'Add(TEXT("japan")', 'AddDisabled(TEXT("japan")'),
        },
      },
      {
        why: 'معرّف الوحدة الخاصة في النسخة المدمجة لا يطابق البيانات',
        edits: {
          [LC]: (s) => replaceUnique(s, 'TEXT("huskarl")', 'TEXT("berserker")'),
        },
      },
    ],
  },

  {
    id: 'C2',
    title: 'قائمة الحضارات مشتقّة من البيانات لا مكتوبة في الكود',
    run(vfs) {
      const r = [];
      const lib = code(vfs, BLIB);
      const body = fnBody(vfs, BLIB, 'TArray<FRok2Civilization> URok2BlueprintLibrary::GetDefaultCivilizations()');

      r.push(test(body.length > 0, 'جسم GetDefaultCivilizations موجود'));
      r.push(test(/URok2CivLore::Get\(\)/.test(body) && /GetCivIds\(\)/.test(body),
        'القائمة تُبنى من سجلّ النصّ (الذي يقرأ الملف)',
        'قائمة مكتوبة يدوياً تنحرف عن الملف بلا أن يرصدها المترجم'));

      // لا معرّفات حضارات محفورة في الجسم
      const civIds = civs(vfs).map((c) => c.id);
      const hardcoded = civIds.filter((id) => new RegExp(`TEXT\\("${id}"\\)`).test(body));
      r.push(test(hardcoded.length === 0,
        'لا معرّف حضارة محفور في جسم الدالة', `محفورة: ${hardcoded.join(', ')}`));

      // byzantium: كانت معروضة ويرفضها الخادم — ووثيقة التصميم تُدرجها في
      // «التوسع المستقبلي» لا في الإطلاق.
      const clientFiles = [...vfs.keys()].filter((k) => k.startsWith('Public/') || k.startsWith('Private/'));
      const ghosts = clientFiles.filter((k) => /byzantium/i.test(code(vfs, k)));
      r.push(test(ghosts.length === 0,
        'لا حضارة غير موجودة في البيانات معروضة في العميل',
        `byzantium في: ${ghosts.join(', ')} — /v1/city/init يرفضها بـ400`));

      // وكل حضارة في الملف تصير قابلة للاختيار — egypt كانت غائبة
      r.push(test(civIds.includes('egypt'), 'egypt موجودة في البيانات'));

      // ولا نسبة توازن في وصف تُعرَض للاعب من الكود
      r.push(test(!/%/.test(body),
        'لا نسبة بونص مكتوبة في القائمة', 'AGENTS.md §3: لا قيم توازن ثابتة في الكود'));
      return r;
    },
    breaks: [
      {
        why: 'عودة قائمة مكتوبة يدوياً بدل الاشتقاق من الملف',
        edits: {
          [BLIB]: (s) => replaceUnique(s, 'URok2CivLore* Lore = URok2CivLore::Get();',
            'URok2CivLore* Lore = nullptr; FRok2Civilization Hard; Hard.Id = TEXT("rome"); List.Add(Hard);'),
        },
      },
      {
        why: 'رجوع حضارة يرفضها الخادم إلى قائمة الاختيار (العطل الأصلي)',
        edits: {
          // المرساة فريدة عن قصد: «return List;» وحدها تتكرر (حرس !Lore والعودة
          // الأخيرة)، وطفرة على مرساة ملتبسة تُخفق بالاستثناء لا بالرصد — أي
          // تُعطي طمأنينة كاذبة تماماً كفحص بتعبير نمطي خاطئ.
          [BLIB]: (s) => replaceUnique(s, '\t\tList.Add(C);\n\t}\n\n\treturn List;',
            '\t\tList.Add(C);\n\t}\n\n\tFRok2Civilization Byz;\n\tByz.Id = TEXT("byzantium");\n\tList.Add(Byz);\n\treturn List;'),
        },
      },
      {
        why: 'نسبة بونص عادت إلى نصّ يُعرض من الكود',
        edits: {
          [BLIB]: (s) => replaceUnique(s, 'C.FantasyAr = L.FantasyAr;',
            'C.FantasyAr = L.FantasyAr + TEXT(" — مشاة +5%");'),
        },
      },
    ],
  },

  {
    id: 'C3',
    title: 'النبذة تظهر عند اختيار الحضارة',
    run(vfs) {
      const r = [];
      const bc = code(vfs, BC);
      const bh = code(vfs, BH);

      r.push(test(/OnSelectionChanged\.AddDynamic\(this,\s*&URok2BootWidget::OnCivSelectionChanged\)/.test(bc),
        'تغيّر الاختيار مربوط بمعالج',
        'بلا ربط، النبذة تُبنى ولا تتبدّل — أي نصّ حضارة واحدة للست'));
      r.push(test(/void URok2BootWidget::OnCivSelectionChanged/.test(bc)
        && /ShowLoreFor\(SelectedCivId\(\)\)/.test(bc),
        'المعالج يعرض نبذة الحضارة المختارة'));
      r.push(test(/URok2CivLore::Get\(\)/.test(bc), 'النصّ من السجلّ لا من نصوص محلية'));

      const show = fnBody(vfs, BC, 'void URok2BootWidget::ShowLoreFor(const FString& CivId)');
      r.push(test(show.length > 0, 'جسم ShowLoreFor موجود'));
      r.push(test(/StoryText\(CivId\)/.test(show), 'النبذة تُعرض من StoryText'));
      r.push(test(/L\.Greeting/.test(show), 'التحية تُعرض في البطاقة'));
      r.push(test(/Collapsed/.test(show),
        'حضارة بلا نبذة تُطوى اللوحة', 'لوحة فارغة أو نصّ بديل مخترع كلاهما أسوأ'));

      // حرس تكرار الحركة: UComboBoxString يبثّ الحدث عند إعادة الملء أيضاً
      r.push(test(/LastLoreCivId/.test(show) && /LastLoreCivId/.test(bh),
        'حرس ضد إعادة الحركة على اختيار لم يتغيّر',
        'بطاقة تُعيد الظهور مع كل بثّ = وميض مزعج لا انتقال'));

      // الحدّ الأفقي: SetAutoWrapText لا يلتفّ بلا حدّ (درس P6-T4)
      const build = fnBody(vfs, BC, 'void URok2BootWidget::BuildLorePanel(UVerticalBox* VBox)');
      r.push(test(/SetWidthOverride/.test(build) && /SizeBox/.test(build),
        'للنبذة حدٌّ أفقي (SizeBox) لا AutoWrap وحده',
        'SetAutoWrapText لا يلتفّ بلا حدٍّ أفقي فيخرج السطر العربي من الشاشة'));
      r.push(test(/SetAutoWrapText\(true\)/.test(build), 'اللفّ مفعَّل عند الحاجة'));
      return r;
    },
    breaks: [
      {
        why: 'ربط تغيّر الاختيار محذوف فلا تتبدّل النبذة',
        edits: {
          [BC]: (s) => replaceUnique(s,
            'CivCombo->OnSelectionChanged.AddDynamic(this, &URok2BootWidget::OnCivSelectionChanged);',
            '// (unbound)'),
        },
      },
      {
        why: 'حدّ العرض أُزيل فتخرج أسطر النبذة العربية من البطاقة',
        edits: {
          [BC]: (s) => replaceUnique(s, 'Bounds->SetWidthOverride(Rok2BootLoreStyle::StoryWidth);', '// (no width)'),
        },
      },
      {
        why: 'حرس التكرار أُزيل فتُعاد الحركة مع كل بثّ',
        edits: {
          [BC]: (s) => replaceUnique(s,
            'if (LastLoreCivId == CivId && LorePanel->GetVisibility() != ESlateVisibility::Collapsed)',
            'if (false)').replace(/LastLoreCivId/g, 'PrevCivIdUnused'),
        },
      },
    ],
  },

  {
    id: 'C4',
    title: 'شاشة معلومات الحضارة تعرض النبذة وتُفتح من الـHUD',
    run(vfs) {
      const r = [];
      const ic = code(vfs, IC);
      const hudH = code(vfs, HUD_H);
      const hudC = code(vfs, HUD_C);
      const gmC = code(vfs, GM_C);
      const gmH = code(vfs, GM_H);

      r.push(test(/StoryText\(CivId\)/.test(ic), 'الشاشة تعرض النبذة من السجلّ'));
      r.push(test(/L\.Greeting/.test(ic), 'الشاشة تعرض التحية'));
      r.push(test(/FillHints\(L\.Hints\)/.test(ic), 'الشاشة تعرض التلميحات'));

      // الحضارة من الخادم لا من اختيار محلي — لاعب عائد لم يمرّ بالقائمة
      r.push(test(/Api->GetPlayer\(\)\.Civ/.test(ic),
        'الحضارة تُقرأ من حالة اللاعب (الخادم)',
        'اختيار محلي يُري اللاعب العائد حكاية أول خيار في القائمة'));

      // سلسلة الوصول كاملة: زر → مفوَّض → معالج → لوحة
      r.push(test(/FOnHudAction OnCivInfoAction/.test(hudH), 'الـHUD يعلن مفوَّض الحضارة'));
      r.push(test(/SpawnSmall\(TEXT\("crown"\)/.test(hudC), 'زر دائري بأيقونة تاج في العنقود'));
      r.push(test(/OnCivInfoClickedHandler/.test(hudC) && /OnCivInfoAction\.Broadcast\(\)/.test(hudC),
        'الزر يبثّ المفوَّض'));
      r.push(test(/OnCivInfoAction\.AddDynamic\(this,\s*&ARok2GameMode::HandleCivInfoAction\)/.test(gmC),
        'GameMode يربط المفوَّض'));
      r.push(test(/URok2CivInfoWidget\* CivInfoWidget/.test(gmH), 'GameMode يملك اللوحة'));

      const handler = fnBody(vfs, GM_C, 'void ARok2GameMode::HandleCivInfoAction()');
      r.push(test(/AddToViewport\(50\)/.test(handler),
        'اللوحة على ترتيب اللوحات 50', 'فوق الـHUD 20 وتحت طبقة الإرشاد 60'));
      r.push(test(/RefreshFromPlayer\(\)/.test(handler),
        'تُعاد القراءة عند كل فتح',
        'اللوحة تُنشأ مرة وتُعاد للعرض مراراً — بلا إعادة قراءة تبقى على حضارة قديمة'));

      // أيقونات إجرائية معروفة لا إيموجي (قاعدة P6-T1)
      const icons = knownIcons(vfs);
      const used = [...ic.matchAll(/GetIconBrush\(TEXT\("([a-z_0-9]+)"\)/g)].map((m) => m[1]);
      r.push(test(used.length > 0, 'الشاشة تستخدم أيقونات إجرائية'));
      const unknown = used.filter((i) => !icons.has(i));
      r.push(test(unknown.length === 0,
        'كل أيقونة مستخدمة معروفة للمكتبة', `مجهولة: ${unknown.join(', ')}`));
      r.push(test(!/[\u{1F300}-\u{1FAFF}\u{2600}-\u{27BF}]/u.test(ic),
        'لا إيموجي في الشاشة (قاعدة P6-T1)'));

      // §1: كل لوحة تنزلق من أسفل ولا تُزال بقفزة
      r.push(test(/PlaySlideInBottom/.test(ic), 'اللوحة تنزلق من أسفل (وثيقة UI §1)'));
      r.push(test(/PlayFadeOut\(this\)/.test(ic) && !/RemoveFromParent\(\)/.test(ic),
        'الإغلاق بتلاشٍ لا بإزالة مفاجئة'));
      return r;
    },
    breaks: [
      {
        why: 'الشاشة تقرأ اختياراً محلياً بدل حضارة اللاعب من الخادم',
        edits: {
          [IC]: (s) => replaceUnique(s, 'const FString CivId = Api->GetPlayer().Civ;',
            'const FString CivId = TEXT("rome");'),
        },
      },
      {
        why: 'زر الـHUD لا يبثّ المفوَّض فلا تُفتح الشاشة أبداً',
        edits: {
          [HUD_C]: (s) => replaceUnique(s, 'void URok2HudWidget::OnCivInfoClickedHandler() { OnCivInfoAction.Broadcast(); }',
            'void URok2HudWidget::OnCivInfoClickedHandler() { }'),
        },
      },
      {
        why: 'أيقونة غير معروفة للمكتبة (brush فارغة على الشاشة)',
        edits: {
          [IC]: (s) => replaceUnique(s, 'GetIconBrush(TEXT("crown")', 'GetIconBrush(TEXT("laurel_wreath")'),
        },
      },
      {
        why: 'الشاشة لا تُعاد قراءتها عند الفتح فتبقى على حضارة قديمة',
        edits: {
          [GM_C]: (s) => replaceUnique(s, 'CivInfoWidget->RefreshFromPlayer();', '// (stale)'),
        },
      },
    ],
  },

  {
    id: 'C5',
    title: 'التحية مرة واحدة عبر نظام الإشعارات القائم',
    run(vfs) {
      const r = [];
      const ac = code(vfs, API_C);
      const ah = code(vfs, API_H);
      const body = fnBody(vfs, API_C, 'void URok2Api::MaybeGreetCiv()');

      r.push(test(body.length > 0, 'جسم MaybeGreetCiv موجود'));
      r.push(test(/MaybeGreetCiv\(\);/.test(ac), 'تُنادى من مسار تحليل المدينة'));
      // ParseCity يُنادى مع كل نبضة — بلا مِزلاج تصير التحية ضجيجاً
      r.push(test(/bCivGreetingShown/.test(body) && /bCivGreetingShown/.test(ah),
        'مِزلاج جلسة يمنع تكرار التحية',
        'ParseCity يُنادى مع كل نبضة شبكة، فبلا مِزلاج تتكرر التحية كل ثانية'));
      r.push(test(/if\s*\(bCivGreetingShown\)\s*return;/.test(body),
        'المِزلاج يُفحص قبل أي عمل'));
      // الراية لا تُرفع قبل معرفة الحضارة، وإلا فُقدت التحية للأبد
      const civGuardIdx = body.indexOf('CivId.IsEmpty()');
      const latchIdx = body.indexOf('bCivGreetingShown = true');
      r.push(test(civGuardIdx > -1 && latchIdx > -1 && civGuardIdx < latchIdx,
        'الراية لا تُرفع قبل التحقق من وجود حضارة',
        'رفعها على حمولة بلا لاعب يُفقد التحية إلى الأبد'));
      r.push(test(/PushNotification\(/.test(body),
        'تُبثّ عبر نظام الإشعارات القائم لا بطاقة جديدة'));
      r.push(test(/HasLore\(CivId\)/.test(body),
        'لا تحية لحضارة بلا نصّ'));
      // النوع من الأنواع التي يلوّنها الـHUD
      const kinds = new Set([...code(vfs, HUD_C).matchAll(/N\.Kind == TEXT\("(\w+)"\)/g)].map((m) => m[1]));
      const used = /PushNotification\(TEXT\("(\w+)"\)/.exec(body);
      r.push(test(!!used, 'نوع الإشعار صريح'));
      if (used) {
        r.push(test(used[1] === 'toast' || kinds.has(used[1]),
          `نوع الإشعار «${used[1]}» يعرفه الـHUD`,
          `أنواع ملوّنة: ${[...kinds].join(', ')} — وما سواها يردّ إلى لون اللوحة`));
      }
      return r;
    },
    breaks: [
      {
        why: 'المِزلاج أُزيل فتتكرر التحية مع كل نبضة شبكة',
        edits: {
          [API_C]: (s) => replaceUnique(s, '\tif (bCivGreetingShown) return;', '\t// (no latch)'),
        },
      },
      {
        why: 'الراية تُرفع قبل معرفة الحضارة فتُفقد التحية إلى الأبد',
        edits: {
          [API_C]: (s) => replaceUnique(s,
            '\tconst FString CivId = Player.Civ;\n\tif (CivId.IsEmpty()) return;',
            '\tbCivGreetingShown = true;\n\tconst FString CivId = Player.Civ;\n\tif (CivId.IsEmpty()) return;'),
        },
      },
      {
        why: 'التحية لم تُنادَ من مسار المدينة إطلاقاً',
        edits: {
          [API_C]: (s) => replaceUnique(s, '\tMaybeGreetCiv();', '\t// (never greeted)'),
        },
      },
    ],
  },

  {
    id: 'C6',
    title: 'الخادم أعلى سلطة على النصّ، والاستبدال ذرّي',
    run(vfs) {
      const r = [];
      const lc = code(vfs, LC);
      const ac = code(vfs, API_C);
      const apply = fnBody(vfs, LC, 'bool URok2CivLore::ApplyServerCivs(const TArray<TSharedPtr<FJsonValue>>& CivsArray)');

      r.push(test(/ApplyServerCivs\(\*CivsArr\)/.test(ac),
        'حمولة /v1/meta/all تُطبَّق على السجلّ',
        'بلا هذا لا يصل نصّ الخادم أبداً — أي لا تحديث نصّ بلا تحديث عميل'));

      // الترتيب: النصّ أولاً ثم القائمة منه
      const applyIdx = ac.indexOf('ApplyServerCivs(*CivsArr)');
      const listIdx = ac.indexOf('Self->Civilizations = URok2BlueprintLibrary::GetDefaultCivilizations()');
      r.push(test(applyIdx > -1 && listIdx > -1 && applyIdx < listIdx,
        'النصّ يُطبَّق قبل بناء القائمة منه',
        'العكس يبني القائمة من النسخة المدمجة فتبقى على النصّ القديم'));

      r.push(test(apply.length > 0, 'جسم ApplyServerCivs موجود'));
      r.push(test(/Staged/.test(apply) && /Entries = MoveTemp\(Staged\)/.test(apply),
        'الاستبدال ذرّي (يُبنى جانباً ثم يُستبدل)',
        'استبدال مباشر بحمولة عرجاء يمحو النصّ المدمج ويُترك الشاشة بلا حكاية'));
      r.push(test(/if \(Staged\.Num\(\) <= 0\)/.test(apply) && /return false/.test(apply),
        'حمولة بلا سجلّ مكتمل لا تستبدل شيئاً'));
      r.push(test(/IsCompleteEntry\(E\)/.test(apply),
        'كل سجلّ من الخادم يُفحص اكتماله قبل قبوله'));

      // مُحلِّل واحد للقرص والخادم — لا نسختان تنحرفان
      const disk = fnBody(vfs, LC, 'int32 URok2CivLore::LoadFromJsonString(const FString& JsonString)');
      r.push(test(/ParseFromJson/.test(disk) && /ParseFromJson/.test(apply),
        'مُحلِّل واحد للقرص وللخادم',
        'مُحلِّلان ينحرفان: حقل يُقرأ من أحدهما ولا يُقرأ من الآخر'));

      // معرّف مجهول يعيد فراغاً لا أول عنصر
      const getLore = fnBody(vfs, LC, 'const FRok2CivLore& URok2CivLore::GetLore(const FString& CivId) const');
      r.push(test(/return EmptyEntry;/.test(getLore),
        'معرّف مجهول يعيد سجلاً فارغاً لا أول عنصر',
        'إعادة أول عنصر تُري حضارةً نصَّ حضارة أخرى — عطل صامت'));
      return r;
    },
    breaks: [
      {
        why: 'حمولة الخادم لا تُطبَّق فلا يتغيّر النصّ بلا تحديث عميل',
        edits: {
          [API_C]: (s) => replaceUnique(s, 'if (Lore->ApplyServerCivs(*CivsArr))', 'if (false)'),
        },
      },
      {
        why: 'استبدال مباشر بحمولة غير مفحوصة يمحو النصّ المدمج',
        edits: {
          [LC]: (s) => replaceUnique(s, 'Entries = MoveTemp(Staged);', 'Entries.Empty();'),
        },
      },
      {
        why: 'معرّف مجهول يعيد أول عنصر فتُعرض حكاية حضارة أخرى',
        edits: {
          [LC]: (s) => replaceUnique(s, '\treturn EmptyEntry;',
            '\treturn Entries.Num() > 0 ? Entries[0] : EmptyEntry;'),
        },
      },
    ],
  },

  {
    id: 'N1',
    title: 'لا رقم توازن في نصٍّ أدبي',
    run(vfs) {
      const r = [];
      const prose = allProse(vfs);
      r.push(test(prose.length > 0, 'النصوص الأدبية مقروءة من البيانات'));

      // نسبة مئوية صريحة: النصّ يصير كذبةً موثّقة لحظة تعديل قيمة في bonuses
      const withPct = prose.filter((p) => /[%٪]/.test(p.text));
      r.push(test(withPct.length === 0,
        'لا نسبة مئوية في نبذة أو تحية أو تلميح',
        withPct.map((p) => `${p.civ}/${p.field}`).join(', ')
          + ' — AGENTS.md §3: الأرقام بيانات توازن يخدمها الخادم'));

      // ولا أرقام لاتينية أصلاً: التلميح يشير إلى البونص ولا يقتبس قيمته
      const withDigits = prose.filter((p) => /[0-9٠-٩]/.test(p.text));
      r.push(test(withDigits.length === 0,
        'لا أرقام في النصوص الأدبية',
        withDigits.map((p) => `${p.civ}/${p.field}: ${p.text.slice(0, 40)}`).join(' | ')));

      // كل حضارة لها تلميح واحد على الأقل لكل بونص؟ لا — نتحقق فقط أن عدد
      // التلميحات لا يزيد على عدد البونصات، فتلميح بلا بونص يقابله وعدٌ بلا سند.
      for (const c of civs(vfs)) {
        const nb = (c.bonuses ?? []).length;
        const nh = (c.hints ?? []).length;
        r.push(test(nh <= nb,
          `${c.id}: التلميحات (${nh}) لا تزيد على البونصات (${nb})`,
          'تلميح بلا بونص يقابله = وعدٌ بلا سند في البيانات'));
      }
      return r;
    },
    breaks: [
      {
        why: 'نسبة بونص كُتبت في تلميح فتصير كذبة موثّقة عند تعديل التوازن',
        edits: {
          [DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations[0].hints[0] = 'دفاع مشاتك أعلى بـ5% من غيرك.';
            return JSON.stringify(d, null, 2);
          },
        },
      },
      {
        why: 'تلميحات أكثر من البونصات (وعد بلا سند)',
        edits: {
          [DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations[1].hints.push('وحصونك أمنع من حصون غيرك.');
            d.civilizations[1].hints.push('ومخزنك أوسع.');
            return JSON.stringify(d, null, 2);
          },
        },
      },
    ],
  },

  {
    id: 'N2',
    title: 'الأدوار النصّية من نظام Typography لا أحجام مُقحَمة',
    run(vfs) {
      const r = [];
      for (const [key, label] of [[IC, 'شاشة المعلومات'], [BC, 'بطاقة الاختيار']]) {
        const src = code(vfs, key);
        const applied = [...src.matchAll(/ApplyFont\([^,]+,\s*ERok2TextRole::(\w+)\)/g)].map((m) => m[1]);
        r.push(test(applied.length > 0, `${label}: تستخدم أدوار Typography`));
        // اسم الحضارة بدور Display — الوثيقة تخصّه بـ«اسم الحضارة عند الاختيار»
        r.push(test(applied.includes('Display'),
          `${label}: اسم الحضارة بدور Display`,
          'ERok2TextRole::Display موصوف في Rok2Typography.h بـ«اسم الحضارة عند الاختيار»'));
        // لا أحجام رقمية مباشرة على النصوص
        r.push(test(!/(?:^|\s)F\.Size\s*=|SetFont\(.*Size/.test(src),
          `${label}: لا حجم خطٍّ مُقحَم`, 'P6-T2: الودجات تطلب دوراً لا حجماً'));
      }
      return r;
    },
    breaks: [
      {
        why: 'اسم الحضارة فقد دور Display فتساوى مع نصّ الجسم',
        edits: {
          [IC]: (s) => replaceUnique(s, 'URok2Typography::ApplyFont(NameText, ERok2TextRole::Display);',
            'URok2Typography::ApplyFont(NameText, ERok2TextRole::Body);'),
        },
      },
    ],
  },

  {
    id: 'N3',
    title: 'بنية السجلّ: مصفوفة الأسطر محفوظة والحدود من مصدر واحد',
    run(vfs) {
      const r = [];
      const lh = code(vfs, LH);
      const lc = code(vfs, LC);

      r.push(test(/TArray<FString>\s+Story;/.test(lh),
        'النبذة مصفوفة أسطر في البنية لا FString واحدة',
        'نصّ واحد يُلفّ آلياً فتُقطع الجملة العربية في منتصفها'));
      r.push(test(/TArray<FString>\s+Hints;/.test(lh), 'التلميحات مصفوفة'));

      // StoryText يجمع بفواصل أسطر — لا بمسافات
      const st = fnBody(vfs, LC, 'FString URok2CivLore::StoryText(const FString& CivId) const');
      r.push(test(/FString::Join\(E\.Story,\s*TEXT\("\\n"\)\)/.test(st),
        'الأسطر تُجمع بفاصل سطر لا بمسافة',
        'الجمع بمسافة يُلغي مواضع القطع المؤلَّفة'));

      // الحدود تُقرأ من المواصفة في موضع الفحص نفسه
      const complete = fnBody(vfs, LC, 'bool URok2CivLore::IsCompleteEntry(const FRok2CivLore& Entry)');
      r.push(test(/Rok2CivLoreSpec::MinStoryLines/.test(complete)
        && /Rok2CivLoreSpec::MaxStoryLines/.test(complete),
        'الفحص يقرأ الحدود من المواصفة لا أرقاماً مكرَّرة'));
      r.push(test(!/Story\.Num\(\)\s*[<>]=?\s*[0-9]/.test(complete),
        'لا رقم حرفي في شرط عدد الأسطر'));

      // ثابت FString غير بدائي: extern في الهيدر وتعريف واحد في الـcpp
      r.push(test(/extern ROK2_API const FString JsonRelativePath;/.test(lh),
        'ثابت المسار مُعلَن extern في الهيدر',
        'const FString عند نطاق النطاق في هيدر عام يعطي نسخة لكل وحدة ترجمة'));
      r.push(test(/const FString JsonRelativePath\(TEXT\(/.test(lc),
        'وله تعريف واحد في الـcpp'));

      // لفّ الفهرس آمن مع السالب
      const hint = fnBody(vfs, LC, 'FString URok2CivLore::HintAt(const FString& CivId, int32 Index) const');
      r.push(test(/%.*\+.*%/s.test(hint.replace(/\s+/g, '')) || /\+ E\.Hints\.Num\(\)\) % E\.Hints\.Num\(\)/.test(hint),
        'لفّ الفهرس آمن مع القيم السالبة',
        '% على سالب في C++ يعطي سالباً فتتعطّل الفهرسة'));
      return r;
    },
    breaks: [
      {
        why: 'النبذة صارت FString واحدة فضاعت مواضع القطع المؤلَّفة',
        edits: {
          [LH]: (s) => replaceUnique(s, 'TArray<FString> Story;', 'FString Story;'),
        },
      },
      {
        why: 'الأسطر تُجمع بمسافة فتُلفّ آلياً حيث اتّفق',
        edits: {
          [LC]: (s) => replaceUnique(s, 'FString::Join(E.Story, TEXT("\\n"))', 'FString::Join(E.Story, TEXT(" "))'),
        },
      },
      {
        why: 'رقم حرفي بدل حدّ المواصفة (رقمان للشرط نفسه ينحرفان)',
        edits: {
          [LC]: (s) => replaceUnique(s, 'if (Entry.Story.Num() < Rok2CivLoreSpec::MinStoryLines) return false;',
            'if (Entry.Story.Num() < 2) return false;'),
        },
      },
      {
        why: 'لفّ الفهرس بلا حماية من السالب',
        edits: {
          [LC]: (s) => replaceUnique(s,
            'const int32 Wrapped = ((Index % E.Hints.Num()) + E.Hints.Num()) % E.Hints.Num();',
            'const int32 Wrapped = Index % E.Hints.Num();'),
        },
      },
    ],
  },

  {
    id: 'N4',
    title: 'أنواع غير منعكسة لا تدخل UFUNCTION',
    run(vfs) {
      const r = [];
      // UHT يرفض TSharedPtr/FJsonValue معاملاً لدالة منعكسة — والخطأ لا يظهر
      // إلا عند توليد الهيدرات، أي بعد دقائق من البناء.
      for (const key of [LH, IH, BH, API_H, HUD_H, GM_H, TYPES]) {
        const src = get(vfs, key);
        const offenders = [];
        for (const m of src.matchAll(/UFUNCTION\([^)]*\)\s*\n(?:\s*\/\/[^\n]*\n)*\s*([^\n;{]*[;{])/g)) {
          if (/TSharedPtr|TSharedRef|FJsonObject|FJsonValue|TFunction/.test(m[1])) {
            offenders.push(m[1].trim().slice(0, 70));
          }
        }
        r.push(test(offenders.length === 0,
          `${key}: لا UFUNCTION بنوع غير منعكس`, offenders.join(' | ')));
      }
      // والدالة التي تأخذ حمولة JSON ليست منعكسة فعلاً
      const lh = get(vfs, LH);
      const applyDecl = /(UFUNCTION\([^)]*\)\s*\n(?:\s*[^\n]*\n)*?)?\s*bool ApplyServerCivs/.exec(lh);
      r.push(test(!!applyDecl, 'تصريح ApplyServerCivs موجود'));
      const before = lh.slice(Math.max(0, lh.indexOf('bool ApplyServerCivs') - 260), lh.indexOf('bool ApplyServerCivs'));
      r.push(test(!/UFUNCTION\([^)]*\)\s*(?:\/\/[^\n]*\n\s*)*$/.test(before),
        'ApplyServerCivs ليست UFUNCTION (حمولتها غير منعكسة)'));
      return r;
    },
    breaks: [
      {
        why: 'UFUNCTION على دالة بحمولة JSON — UHT يفشل عند توليد الهيدرات',
        edits: {
          [LH]: (s) => replaceUnique(s, '\tbool ApplyServerCivs(',
            '\tUFUNCTION(BlueprintCallable, Category = "Rok2|Lore")\n\tbool ApplyServerCivs('),
        },
      },
    ],
  },

  {
    id: 'N5',
    title: 'كل حضارة في البيانات مخدومة كاملة عند الطرفين',
    run(vfs) {
      const r = [];
      const list = civs(vfs);
      const emb = new Map(embeddedEntries(vfs).map((e) => [e.id, e]));

      // الحلقة الكاملة: ملف → نسخة backend → نسخة مدمجة → قائمة العميل
      const backend = new Map(civs(vfs, BACKEND_DATA).map((c) => [c.id, c]));
      for (const c of list) {
        r.push(test(backend.has(c.id), `${c.id}: يخدمها الخادم`));
        r.push(test(emb.has(c.id), `${c.id}: في نسخة الشحن المدمجة`));
        const b = backend.get(c.id);
        if (b) {
          r.push(test(JSON.stringify(b.story) === JSON.stringify(c.story),
            `${c.id}: نبذة الخادم مطابقة لنبذة المستودع`));
        }
      }
      // ولا حضارة في المدمج ليست في الملف (نصّ يظهر بلا سند في البيانات)
      const extra = [...emb.keys()].filter((id) => !list.some((c) => c.id === id));
      r.push(test(extra.length === 0,
        'لا حضارة في النسخة المدمجة خارج البيانات', `زائدة: ${extra.join(', ')}`));
      return r;
    },
    breaks: [
      {
        why: 'حضارة في النسخة المدمجة لا سند لها في البيانات',
        edits: {
          [LC]: (s) => replaceUnique(s, '\tAdd(TEXT("rome")',
            '\tAdd(TEXT("byzantium"), TEXT("بيزنطة"), TEXT("Byzantium"), TEXT("حصن الشرق"),\n'
            + '\t\tTEXT("Eastern bastion"), TEXT("cataphract"),\n'
            + '\t\t{ TEXT("سطر."), TEXT("سطر."), TEXT("سطر.") },\n'
            + '\t\tTEXT("تحية."),\n'
            + '\t\t{ TEXT("تلميح.") });\n\n\tAdd(TEXT("rome")'),
        },
      },
      {
        why: 'حضارة سقطت من نسخة الخادم فيرفضها init ويعرضها العميل',
        edits: {
          [BACKEND_DATA]: (s) => {
            const d = JSON.parse(s);
            d.civilizations = d.civilizations.filter((c) => c.id !== 'egypt');
            return JSON.stringify(d, null, 2);
          },
        },
      },
    ],
  },
];

// ---------------------------------------------------------------------------
// المُشغِّل
// ---------------------------------------------------------------------------

const REAL = readTree();

for (const key of [DATA, BACKEND_DATA, GAMEDATA, ROUTER, LH, LC, IH, IC, BH, BC,
  BLIB, API_C, API_H, HUD_C, HUD_H, GM_C, GM_H, TYPES, ICONS]) {
  if (!REAL.has(key)) {
    console.error(`❌ ملف مفقود من الشجرة: ${key}`);
    process.exit(1);
  }
}

console.log('═'.repeat(62));
console.log('verify_civ_lore — P6-T5 النَفَس القصصي للحضارات (حرس بطورين)');
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
      else {
        const mutated = mutate(REAL, b.edits);
        const res = d.run(mutated);
        detected = res.some((r) => !r.ok);
        // أي فحص أخفق؟ نطبعه في الوضع المطوَّل للتأكد أن الإخفاق **للسبب
        // المقصود** لا عَرَضاً — اختبار نفي يُخفق فحصاً آخر يعطي طمأنينة كاذبة.
        if (VERBOSE && detected) {
          note = ` → أخفق: ${res.filter((r) => !r.ok).map((r) => r.name).join('؛ ')}`;
        }
      }
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
console.log('تحفّظ: تحقق بنيوي بلا مترجم C++ — ولا حكم على جودة النصّ الأدبي.');
console.log('═'.repeat(62));

if (failed || negBad) {
  console.log('\n❌ FAILED');
  for (const f of failures) console.log(`   • ${f}`);
  process.exit(1);
}
console.log(`\n✅ ALL PASSED (${passed} فحصاً + ${negOk} اختبار نفي)`);
process.exit(0);
