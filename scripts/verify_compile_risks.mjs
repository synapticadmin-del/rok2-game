#!/usr/bin/env node
/**
 * verify_compile_risks.mjs — تدقيق مخاطر الترجمة في وحدة Rok2: حرس بنيوي
 *
 * يحرس هذا السكربت الـ11 مجموعة مخاطر التي أُصلحت في تدقيق 2026-07-29 (كوميت
 * dd8fb08)، ونتيجتَي LOW التاليتين، إضافةً إلى اكتفاء الهيدرات العامة بذاتها.
 * كلها من صنف واحد: **كود لا يُترجم أصلاً**، لا كود يسلك سلوكاً خاطئاً.
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  لماذا اختبار النفي؟
 * ────────────────────────────────────────────────────────────────────────────
 * حرسٌ بنيوي لا يُثبت أنه يرصد شيئاً إلا إذا رأيناه يفشل على العطل الأصلي.
 * فحصٌ مكتوب بتعبير نمطي خاطئ «ينجح» على شجرة سليمة وعلى شجرة معطوبة سواءً،
 * فيعطي طمأنينة كاذبة. لذلك يعيد كل مدقق هنا دالةً نقية على نظام ملفات
 * افتراضي (VFS)، ويُشغّل الطور الثاني كل مدقق على نسخة **مُعطَّبة عمداً**
 * أُعيد فيها زرع العطل الأصلي، ويشترط أن يفشل المدقق هناك.
 *
 *   الطور 1 (إثبات):  المدققون على الشجرة الحقيقية  → يجب أن ينجحوا كلهم
 *   الطور 2 (نفي):    كل مدقق على عطله الأصلي        → يجب أن يفشل كل مرة
 *
 * مدقق ينجح في الطور 1 ولا يفشل في الطور 2 يُحتسب **عطلاً في الحرس نفسه**.
 *
 * ────────────────────────────────────────────────────────────────────────────
 *  تحفّظ صريح
 * ────────────────────────────────────────────────────────────────────────────
 * لا مترجم C++ في هذه البيئة. التحقق بنيوي واستدلال على API المحرك، وليس
 * ادّعاءً ببناء ناجح. نجاح هذا السكربت يعني: العوائق المرصودة لم تعُد موجودة،
 * ولا يعني أن الوحدة تُترجم.
 *
 * Usage: node scripts/verify_compile_risks.mjs
 *        node scripts/verify_compile_risks.mjs --verbose
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join, basename } from 'path';
import { fileURLToPath } from 'url';

const __dirname = fileURLToPath(new URL('.', import.meta.url));
const REPO = join(__dirname, '..');
const CLIENT = join(REPO, 'game', 'client-unreal');
const MOD = join(CLIENT, 'Source', 'Rok2');
const VERBOSE = process.argv.includes('--verbose');

// ───────────────────────────────────────────────────────────────────────────
//  أدوات
// ───────────────────────────────────────────────────────────────────────────

/**
 * يزيل تعليقات C++ قبل الفحص البنيوي.
 *
 * الترتيب مقصود ومُختبَر: تعليقات **السطر أولاً** ثم الكتل. العكس عطل كامن —
 * تعليق سطر يحتوي على فاتحة كتلة (كالمسار `/Engine/BasicShapes/` متبوعاً بنجمة
 * في Rok2Perf.h:4) يفتح كتلة وهمية تُغلق عند أول خاتمة كتلة بعده، فتُمحى عشرات
 * أسطر الكود الحقيقي بينهما ويصير الفحص يجري على فراغ — نجاحٌ كاذب صامت.
 * يحرس هذا الترتيبَ المدقق S0 أدناه.
 */
export function stripComments(src) {
  return src.replace(/\/\/[^\n]*/g, '').replace(/\/\*[\s\S]*?\*\//g, '');
}

/** يبني VFS: مسار نسبي مقروء → محتوى */
function readTree() {
  const vfs = new Map();
  const put = (abs, key) => {
    if (existsSync(abs)) vfs.set(key, readFileSync(abs, 'utf8'));
  };
  for (const sub of ['Public', 'Private']) {
    const dir = join(MOD, sub);
    if (!existsSync(dir)) continue;
    for (const f of readdirSync(dir)) {
      if (f.endsWith('.h') || f.endsWith('.cpp')) put(join(dir, f), `${sub}/${f}`);
    }
  }
  put(join(MOD, 'Rok2.Build.cs'), 'Rok2.Build.cs');
  put(join(CLIENT, 'Source', 'Rok2.Target.cs'), 'Rok2.Target.cs');
  put(join(CLIENT, 'Source', 'Rok2Editor.Target.cs'), 'Rok2Editor.Target.cs');
  put(join(CLIENT, 'Rok2.uproject'), 'Rok2.uproject');
  return vfs;
}

const get = (vfs, k) => vfs.get(k) ?? '';
const code = (vfs, k) => stripComments(get(vfs, k));
const headers = (vfs) => [...vfs.keys()].filter((k) => k.startsWith('Public/'));
const sources = (vfs) => [...vfs.keys()].filter((k) => k.startsWith('Private/'));

/** يعدّل ملفاً في نسخة من الـVFS (لاختبار النفي) */
function mutate(vfs, edits) {
  const copy = new Map(vfs);
  for (const [key, fn] of Object.entries(edits)) {
    const before = copy.get(key);
    if (before === undefined) throw new Error(`mutation target missing: ${key}`);
    const after = fn(before);
    if (after === before) throw new Error(`mutation was a no-op on ${key}`);
    copy.set(key, after);
  }
  return copy;
}

// أنواع محرك شائعة لا يعلنها CoreMinimal.h
const ENGINE_TYPES = [
  'UStaticMesh', 'UMaterialInstanceDynamic', 'UMaterialInterface', 'UTexture2D',
  'UStaticMeshComponent', 'UInstancedStaticMeshComponent',
  'UHierarchicalInstancedStaticMeshComponent', 'USceneComponent', 'UTextBlock',
  'UButton', 'UImage', 'UBorder', 'UCanvasPanel', 'UVerticalBox', 'UHorizontalBox',
  'UScrollBox', 'UProgressBar', 'UEditableTextBox', 'USpacer', 'UWidgetTree',
  'UUserWidget', 'USoundBase', 'UAudioComponent', 'UCameraComponent',
  'USpringArmComponent', 'UWidget', 'UMaterial', 'UFont', 'UDataTable',
  'UGameInstance', 'UWorld', 'UTexture',
];

// ───────────────────────────────────────────────────────────────────────────
//  المدققون — كل واحد دالة نقية على VFS تُعيد قائمة نتائج {name, ok, detail}
// ───────────────────────────────────────────────────────────────────────────

const DETECTORS = [];
const detector = (id, title, run, breaks) => DETECTORS.push({ id, title, run, breaks });

// —— S0: سلامة أداة الفحص نفسها ————————————————————————————————
detector(
  'S0',
  'سلامة stripComments (تعليق سطر يحتوي فاتحة كتلة)',
  () => {
    // تعليق سطر يحتوي فاتحة كتلة، ثم كود حقيقي، ثم تعليق كتلة سليم
    const star = '*';
    const snippet =
      `// path glob /Engine/BasicShapes/${star} inside a line comment\n` +
      `class UStaticMesh;\n` +
      `/${star} real block ${star}/\n` +
      `class UWorld;\n`;
    const out = stripComments(snippet);
    return [
      {
        name: 'الكود بعد تعليق سطر يحتوي فاتحة كتلة لا يُمحى',
        ok: /class UStaticMesh;/.test(out) && /class UWorld;/.test(out),
        detail: JSON.stringify(out.replace(/\n+/g, '|')),
      },
      { name: 'تعليقات الكتل السليمة تُمحى', ok: !/real block/.test(out) },
    ];
  },
  // النفي: الترتيب المعكوس (الكتل أولاً) — العطل الأصلي في verify_delegate_bind
  [
    {
      why: 'الترتيب المعكوس: إزالة الكتل قبل تعليقات السطر',
      selfCheck() {
        const star = '*';
        const bad = (src) =>
          src.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/[^\n]*/g, '');
        const snippet =
          `// path glob /Engine/BasicShapes/${star} inside a line comment\n` +
          `class UStaticMesh;\n` +
          `/${star} real block ${star}/\n` +
          `class UWorld;\n`;
        // يجب أن يُمحى الكود الحقيقي → أي أن الترتيب المعكوس معطوب فعلاً
        return !/class UStaticMesh;/.test(bad(snippet));
      },
    },
  ]
);

// —— R1: حجب AActor::OnClicked بدالة عضو ————————————————————————
detector(
  'R1',
  'لا دالة عضو تحجب خاصية delegate في أصناف المحرك (AActor::OnClicked)',
  (vfs) => {
    const out = [];
    // أي هيدر يعلن دالة عضو باسم OnClicked يحجب الخاصية الموروثة
    for (const k of headers(vfs)) {
      const body = code(vfs, k);
      if (!/\bpublic\s+AActor\b|:\s*public\s+A\w+/.test(body)) continue;
      const shadows = /\bvoid\s+OnClicked\s*\(/.test(body);
      if (shadows) out.push({ name: `${basename(k)} لا يحجب OnClicked`, ok: false, detail: 'void OnClicked( معلنة' });
    }
    out.push({
      name: 'لا هيدر يعلن void OnClicked( (الحجب يُخرج الخاصية من مجموعة البحث)',
      ok: !headers(vfs).some((k) => /\bvoid\s+OnClicked\s*\(/.test(code(vfs, k))),
    });
    // مواضع الربط يجب أن تشير إلى معالج غير محجوب
    const ba = code(vfs, 'Private/Rok2BuildingActor.cpp');
    out.push({
      name: 'Rok2BuildingActor يربط OnClicked بـ HandleActorClicked',
      ok: /OnClicked\.AddDynamic\(\s*this\s*,\s*&ARok2BuildingActor::HandleActorClicked\s*\)/.test(ba),
    });
    out.push({
      name: 'ARok2BuildingActor::HandleActorClicked معلنة في الهيدر',
      ok: /\bvoid\s+HandleActorClicked\s*\(/.test(code(vfs, 'Public/Rok2BuildingActor.h')),
    });
    const cl = code(vfs, 'Private/Rok2CityLayoutActor.cpp');
    out.push({
      name: 'Rok2CityLayoutActor يربط OnClicked بمعالج مملوك له',
      ok: /OnClicked\.AddDynamic\(\s*this\s*,\s*&ARok2CityLayoutActor::\w+\s*\)/.test(cl),
    });
    return out;
  },
  [
    {
      why: 'إعادة تسمية HandleActorClicked إلى OnClicked (العطل الأصلي)',
      edits: {
        'Public/Rok2BuildingActor.h': (s) => s.replace(/HandleActorClicked/g, 'OnClicked'),
        'Private/Rok2BuildingActor.cpp': (s) => s.replace(/HandleActorClicked/g, 'OnClicked'),
      },
    },
  ]
);

// —— R2: إعلان مزدوج ————————————————————————————————————————
detector(
  'R2',
  'لا دالة عضو معلنة مرتين في نفس الهيدر',
  (vfs) => {
    const out = [];
    for (const k of headers(vfs)) {
      const body = code(vfs, k);
      const decls = [...body.matchAll(/\bvoid\s+(\w+)\s*\(\s*\)\s*;/g)].map((m) => m[1]);
      const dupes = [...new Set(decls.filter((n, i) => decls.indexOf(n) !== i))];
      if (dupes.length) out.push({ name: `${basename(k)} بلا إعلانات مكرّرة`, ok: false, detail: dupes.join(', ') });
    }
    const bm = code(vfs, 'Public/Rok2BuildMenuWidget.h');
    const n = (bm.match(/\bvoid\s+OnCloseClicked\s*\(\s*\)\s*;/g) || []).length;
    out.push({ name: 'Rok2BuildMenuWidget::OnCloseClicked معلنة مرة واحدة', ok: n === 1, detail: `${n} إعلان` });
    if (!out.some((r) => r.ok === false)) out.push({ name: 'كل الهيدرات بلا إعلانات void()‎ مكرّرة', ok: true });
    return out;
  },
  [
    {
      why: 'تكرار إعلان OnCloseClicked (العطل الأصلي)',
      edits: {
        'Public/Rok2BuildMenuWidget.h': (s) =>
          s.replace(/(\bvoid\s+OnCloseClicked\s*\(\s*\)\s*;)/, '$1\n\tvoid OnCloseClicked();'),
      },
    },
  ]
);

// —— R3: FJsonObject نوع غير منعكس، UHT لا يولّده ————————————————
detector(
  'R3',
  'FJsonObject مُصرَّح أمامياً في كل هيدر عام يستخدمه',
  (vfs) => {
    const out = [];
    for (const k of headers(vfs)) {
      const body = code(vfs, k);
      if (!/\bFJsonObject\b/.test(body)) continue;
      const declared = /\bclass\s+FJsonObject\s*;/.test(body);
      const included = /#include\s+"(?:Dom\/JsonObject\.h|Json\.h)"/.test(get(vfs, k));
      out.push({
        name: `${basename(k)}: FJsonObject معروف (تصريح أمامي أو تضمين)`,
        ok: declared || included,
      });
    }
    out.push({ name: 'كل مستخدمي FJsonObject في الهيدرات العامة مغطّون', ok: out.length > 0 && out.every((r) => r.ok) });
    return out;
  },
  [
    {
      why: 'حذف التصريح الأمامي من Rok2Api.h (العطل الأصلي)',
      edits: { 'Public/Rok2Api.h': (s) => s.replace(/^class\s+FJsonObject\s*;\s*$/m, '') },
    },
  ]
);

// —— R4: انتهاك protected بين صنفين شقيقين ——————————————————————
detector(
  'R4',
  'لا وصول إلى أعضاء protected من صنف غير مشتق (CityLayoutActor → BuildingActor)',
  (vfs) => {
    const bh = code(vfs, 'Public/Rok2BuildingActor.h');
    const cl = code(vfs, 'Private/Rok2CityLayoutActor.cpp');
    // نقطة الدخول العامة يجب أن تكون معلنة قبل أول protected/private
    const cut = ((m) => (m ? m.index : bh.length))(/^\s*(protected|private)\s*:/m.exec(bh));
    const publicPart = bh.slice(0, cut);
    return [
      {
        name: 'MarkUsingArtAsset معلنة في القسم العام من Rok2BuildingActor.h',
        ok: /\bvoid\s+MarkUsingArtAsset\s*\(\s*\)\s*;/.test(publicPart),
      },
      {
        name: 'CityLayoutActor يستدعي MarkUsingArtAsset لا الحالة المحمية',
        ok: /\bMarkUsingArtAsset\s*\(\s*\)/.test(cl),
      },
      {
        name: 'CityLayoutActor لا يلمس bUsingArtAsset مباشرة',
        ok: !/\bbUsingArtAsset\b/.test(cl),
      },
      {
        name: 'CityLayoutActor لا يستدعي ApplyCivTheme المحمية',
        ok: !/\bApplyCivTheme\s*\(/.test(cl),
      },
    ];
  },
  [
    {
      why: 'إرجاع الوصول المباشر إلى bUsingArtAsset المحمي (العطل الأصلي)',
      edits: {
        'Private/Rok2CityLayoutActor.cpp': (s) =>
          s.replace(/B->MarkUsingArtAsset\(\);/, 'B->bUsingArtAsset = true; B->ApplyCivTheme();'),
      },
    },
  ]
);

// —— R5: دالة حرة تصل إلى أعضاء محمية بلا friend ————————————————
detector(
  'R5',
  'Rok2SendRequest عضو ساكن في URok2Api لا دالة حرة',
  (vfs) => {
    const ah = code(vfs, 'Public/Rok2Api.h');
    const ac = code(vfs, 'Private/Rok2Api.cpp');
    return [
      {
        name: 'Rok2Api.h يعلن static void Rok2SendRequest(',
        ok: /\bstatic\s+void\s+Rok2SendRequest\s*\(/.test(ah),
      },
      {
        name: 'التعريف مؤهَّل بـ URok2Api::',
        ok: /\bvoid\s+URok2Api::Rok2SendRequest\s*\(/.test(ac),
      },
      {
        name: 'لا تعريف لدالة حرة باسم Rok2SendRequest',
        ok: !/^\s*(?:static\s+)?void\s+Rok2SendRequest\s*\(/m.test(ac),
      },
    ];
  },
  [
    {
      why: 'إرجاع Rok2SendRequest دالةً حرة (العطل الأصلي)',
      edits: {
        'Private/Rok2Api.cpp': (s) => s.replace(/void\s+URok2Api::Rok2SendRequest\s*\(/, 'void Rok2SendRequest('),
      },
    },
  ]
);

// —— R6: BuildSettingsVersion.V7 لا وجود له (توقف تام قبل أي C++) ——
detector(
  'R6',
  'BuildSettingsVersion / EngineIncludeOrderVersion من قيم التعداد الموجودة',
  (vfs) => {
    // UBT يترجم ملفات Target بـ C# قبل أي C++ — قيمة غير موجودة توقف البناء كلياً
    const VALID_BUILD = ['V1', 'V2', 'V3', 'V4', 'V5', 'Latest'];
    const out = [];
    for (const k of ['Rok2.Target.cs', 'Rok2Editor.Target.cs']) {
      const s = get(vfs, k);
      const m = /DefaultBuildSettings\s*=\s*BuildSettingsVersion\.(\w+)/.exec(s);
      out.push({
        name: `${k}: BuildSettingsVersion.${m ? m[1] : '?'} قيمة موجودة`,
        ok: !!m && VALID_BUILD.includes(m[1]),
        detail: m ? '' : 'لم يُعثر على الإسناد',
      });
      const io_ = /IncludeOrderVersion\s*=\s*EngineIncludeOrderVersion\.(\w+)/.exec(s);
      out.push({
        name: `${k}: EngineIncludeOrderVersion.${io_ ? io_[1] : '?'} قيمة موجودة`,
        ok: !!io_ && ['Oldest', 'Unreal5_0', 'Unreal5_1', 'Unreal5_2', 'Unreal5_3', 'Unreal5_4', 'Latest'].includes(io_[1]),
      });
    }
    return out;
  },
  [
    {
      why: 'إرجاع BuildSettingsVersion.V7 غير الموجود (العطل الأصلي)',
      edits: {
        'Rok2.Target.cs': (s) => s.replace(/BuildSettingsVersion\.Latest/, 'BuildSettingsVersion.V7'),
      },
    },
  ]
);

// —— R7: USpacer بلا تضمين ————————————————————————————————
detector(
  'R7',
  'كل مستخدم لنوع UMG يضمّن هيدره (USpacer وأخواته)',
  (vfs) => {
    const WIDGETS = {
      USpacer: 'Components/Spacer.h',
      UBorder: 'Components/Border.h',
      UScrollBox: 'Components/ScrollBox.h',
      UProgressBar: 'Components/ProgressBar.h',
      UEditableTextBox: 'Components/EditableTextBox.h',
    };
    const out = [];
    for (const k of sources(vfs)) {
      const raw = get(vfs, k);
      const body = stripComments(raw);
      for (const [type, header] of Object.entries(WIDGETS)) {
        if (!new RegExp(`\\b${type}\\b`).test(body)) continue;
        out.push({ name: `${basename(k)}: ${type} → ${header}`, ok: raw.includes(header) });
      }
    }
    return out.filter((r) => !r.ok).length ? out : [{ name: `كل استخدامات أنواع UMG مضمَّنة (${out.length} موضعاً)`, ok: true }];
  },
  [
    {
      why: 'حذف تضمين Components/Spacer.h مع بقاء USpacer (العطل الأصلي)',
      edits: {
        'Private/Rok2BuildMenuWidget.cpp': (s) => s.replace(/#include\s+"Components\/Spacer\.h"\s*\n/, ''),
      },
    },
  ]
);

// —— R8: استخدام بنية قبل تعريفها في نفس الملف ————————————————
detector(
  'R8',
  'كل بنية FRok2* معرَّفة قبل أول استخدام لها في Rok2Types.h',
  (vfs) => {
    const body = code(vfs, 'Public/Rok2Types.h');
    const out = [];
    for (const m of body.matchAll(/struct\s+(?:ROK2_API\s+)?(FRok2\w+)/g)) {
      const name = m[1];
      const defAt = m.index;
      // أول ذكر للاسم في الملف (خارج التعريف نفسه)
      const first = new RegExp(`\\b${name}\\b`).exec(body);
      const ok = first ? first.index >= defAt : true;
      if (!ok) out.push({ name: `${name} مستخدمة قبل تعريفها`, ok: false, detail: `use@${first.index} < def@${defAt}` });
    }
    out.push({
      name: 'FRok2ZoneStatus معرَّفة قبل مستهلكها (العطل الأصلي)',
      ok: (() => {
        const def = /struct\s+(?:ROK2_API\s+)?FRok2ZoneStatus/.exec(body);
        const use = /TArray<FRok2ZoneStatus>/.exec(body);
        return !!def && (!use || use.index > def.index);
      })(),
    });
    return out;
  },
  [
    {
      why: 'نقل استخدام FRok2ZoneStatus فوق تعريفها (العطل الأصلي)',
      edits: {
        'Public/Rok2Types.h': (s) =>
          s.replace(/(struct\s+(?:ROK2_API\s+)?FRok2ZoneStatus)/, 'struct FRok2Probe { TArray<FRok2ZoneStatus> Z; };\n$1'),
      },
    },
  ]
);

// —— R9: هيدر يسمّي بنية من Rok2Types.h بلا تضمينه ————————————
detector(
  'R9',
  'كل هيدر يستهلك نوعاً من Rok2Types.h يضمّنه',
  (vfs) => {
    const types = code(vfs, 'Public/Rok2Types.h');
    const defined = new Set([
      ...[...types.matchAll(/struct\s+(?:ROK2_API\s+)?(FRok2\w+)/g)].map((m) => m[1]),
      ...[...types.matchAll(/enum\s+class\s+(ERok2\w+)/g)].map((m) => m[1]),
    ]);
    const out = [];
    for (const k of headers(vfs)) {
      if (basename(k) === 'Rok2Types.h') continue;
      const raw = get(vfs, k);
      const body = stripComments(raw);
      const local = new Set([...body.matchAll(/struct\s+(?:ROK2_API\s+)?(FRok2\w+)/g)].map((m) => m[1]));
      const used = [...defined].filter((t) => new RegExp(`\\b${t}\\b`).test(body) && !local.has(t));
      if (!used.length) continue;
      out.push({
        name: `${basename(k)} يضمّن Rok2Types.h (يستهلك ${used.length} نوعاً)`,
        ok: raw.includes('Rok2Types.h'),
        detail: raw.includes('Rok2Types.h') ? '' : used.slice(0, 4).join(', '),
      });
    }
    return out;
  },
  [
    {
      why: 'حذف تضمين Rok2Types.h من هيدر يستهلك أنواعه (العطل الأصلي)',
      edits: {
        'Public/Rok2HudWidget.h': (s) => s.replace(/#include\s+"Rok2Types\.h"\s*\n/, ''),
      },
    },
  ]
);

// —— R10: تضمينات محرك ناقصة في ملفات .cpp ————————————————————
detector(
  'R10',
  'تضمينات أنواع المحرك المستخدمة في .cpp حاضرة',
  (vfs) => {
    const NEEDS = {
      UMaterialInstanceDynamic: 'Materials/MaterialInstanceDynamic.h',
      UStaticMesh: 'Engine/StaticMesh.h',
      FSlateColorBrush: 'Brushes/SlateColorBrush.h',
    };
    const out = [];
    for (const k of sources(vfs)) {
      const raw = get(vfs, k);
      const body = stripComments(raw);
      for (const [type, header] of Object.entries(NEEDS)) {
        if (!new RegExp(`\\b${type}\\b`).test(body)) continue;
        out.push({ name: `${basename(k)}: ${type} → ${header}`, ok: raw.includes(header) });
      }
    }
    return out.filter((r) => !r.ok).length ? out : [{ name: `كل تضمينات أنواع المحرك في .cpp حاضرة (${out.length} موضعاً)`, ok: true }];
  },
  [
    {
      why: 'حذف تضمين Engine/StaticMesh.h مع بقاء UStaticMesh (العطل الأصلي)',
      edits: {
        'Private/Rok2ArtAssets.cpp': (s) => s.replace(/#include\s+"Engine\/StaticMesh\.h"\s*\n/, ''),
      },
    },
  ]
);

// —— R11: UPROPERTY خامد داخل بنية غير USTRUCT ————————————————
detector(
  'R11',
  'لا UPROPERTY داخل بنية غير USTRUCT (يوهم بتتبّع GC غير موجود)',
  (vfs) => {
    const out = [];
    for (const k of headers(vfs)) {
      const body = code(vfs, k);
      // كل struct بلا USTRUCT قبله مباشرة
      for (const m of body.matchAll(/(USTRUCT\s*\([^)]*\)\s*)?struct\s+(\w+)\s*(?:final\s*)?\{/g)) {
        if (m[1]) continue; // USTRUCT حقيقي
        const start = m.index + m[0].length;
        let depth = 1, i = start;
        while (i < body.length && depth > 0) {
          if (body[i] === '{') depth++;
          else if (body[i] === '}') depth--;
          i++;
        }
        const inner = body.slice(start, i);
        if (/\bUPROPERTY\s*\(/.test(inner)) {
          out.push({ name: `${basename(k)}: struct ${m[2]} بلا USTRUCT يحتوي UPROPERTY`, ok: false });
        }
      }
    }
    const hud = code(vfs, 'Public/Rok2HudWidget.h');
    out.push({
      name: 'FToastEntry بلا UPROPERTY، ومرساة الـGC هي ToastCardRefs',
      ok: /\bTArray<UBorder\*>\s+ToastCardRefs\s*;/.test(hud),
    });
    if (!out.some((r) => !r.ok)) out.push({ name: 'لا بنية غير USTRUCT تحتوي UPROPERTY', ok: true });
    return out;
  },
  [
    {
      why: 'زرع UPROPERTY داخل FToastEntry غير المنعكسة (العطل الأصلي)',
      edits: {
        'Public/Rok2HudWidget.h': (s) =>
          s.replace(/(struct\s+FToastEntry\s*\{)/, '$1\n\t\tUPROPERTY()\n\t\tUBorder* Card;'),
      },
    },
  ]
);

// —— L1 (LOW مُغلقة): EngineAssociation ————————————————————————
detector(
  'L1',
  'EngineAssociation نسخة محرك موجودة فعلاً',
  (vfs) => {
    // قرار PLAN.md §29 + UNREAL_ENGINE_GUIDE.md: الهدف UE5 مستقر ≥ 5.4
    // جهاز المطوّر على 5.8 (مؤكَّد 2026-08-12) — القائمة تقبل حتى 5.8
    const KNOWN = ['5.0', '5.1', '5.2', '5.3', '5.4', '5.5', '5.6', '5.7', '5.8'];
    const raw = get(vfs, 'Rok2.uproject');
    let parsed = null;
    try { parsed = JSON.parse(raw); } catch { /* يُرصد أدناه */ }
    const ea = parsed?.EngineAssociation ?? '';
    const isGuid = /^\{?[0-9A-Fa-f-]{36}\}?$/.test(ea);
    return [
      { name: 'Rok2.uproject قابل للتحليل كـ JSON', ok: !!parsed },
      {
        name: `EngineAssociation "${ea}" نسخة موجودة (أو GUID بناء مصدري)`,
        ok: isGuid || KNOWN.includes(ea),
        detail: isGuid || KNOWN.includes(ea) ? '' : 'ليست من نسخ UE5 المعروفة',
      },
      { name: 'الأرضية الموثقة ≥ 5.4 محفوظة', ok: isGuid || parseFloat(ea) >= 5.4 },
    ];
  },
  [
    {
      why: 'إرجاع EngineAssociation إلى نسخة غير موجودة (6.1 لم تصدر)',
      edits: {
        'Rok2.uproject': (s) => s.replace(/"EngineAssociation":\s*"[^"]*"/, '"EngineAssociation": "6.1"'),
      },
    },
  ]
);

// —— L2 (LOW مُغلقة): تبعيات خاصة في هيدرات عامة ————————————————
detector(
  'L2',
  'كل وحدة تظهر في هيدر عام مُعلَنة تبعيةً عامة في Build.cs',
  (vfs) => {
    const cs = get(vfs, 'Rok2.Build.cs');
    const block = (kind) => {
      const m = new RegExp(`${kind}DependencyModuleNames\\.AddRange\\(new string\\[\\]\\s*\\{([\\s\\S]*?)\\}\\)`, 'g');
      const mods = new Set();
      for (const g of cs.matchAll(m)) for (const q of g[1].matchAll(/"([^"]+)"/g)) mods.add(q[1]);
      return mods;
    };
    const pub = block('Public');
    const priv = block('Private');

    // بصمة كل وحدة في الهيدرات العامة
    const FINGERPRINT = {
      UMG: /#include\s+"(?:Blueprint\/UserWidget\.h|Components\/\w+\.h)"/,
      SlateCore: /#include\s+"(?:Styling\/Slate\w+\.h|Brushes\/Slate\w+\.h)"/,
      HTTP: /#include\s+"Interfaces\/IHttp\w+\.h"/,
      WebSockets: /#include\s+"IWebSocket\.h"/,
    };
    const out = [];
    for (const [mod, re] of Object.entries(FINGERPRINT)) {
      const users = headers(vfs).filter((k) => re.test(get(vfs, k)));
      if (!users.length) continue;
      out.push({
        name: `${mod} عامة (يظهر في ${users.length} هيدر عام)`,
        ok: pub.has(mod),
        detail: pub.has(mod) ? '' : `في PrivateDependency بدل Public — ${basename(users[0])}`,
      });
    }
    // Json: FJsonObject في تواقيع عامة
    const jsonUsers = headers(vfs).filter((k) => /\bFJsonObject\b/.test(code(vfs, k)));
    if (jsonUsers.length) {
      out.push({ name: `Json عامة (FJsonObject في ${jsonUsers.length} هيدر عام)`, ok: pub.has('Json') });
    }
    out.push({ name: 'لا وحدة مُعلَنة عامة وخاصة في الوقت نفسه', ok: ![...pub].some((m) => priv.has(m)) });
    out.push({ name: 'وحدات لا يذكرها هيدر عام تبقى خاصة (RHI/RenderCore)', ok: priv.has('RHI') && priv.has('RenderCore') });
    return out;
  },
  [
    {
      why: 'إرجاع UMG إلى PrivateDependencyModuleNames (نتيجة LOW الأصلية)',
      edits: {
        'Rok2.Build.cs': (s) =>
          s.replace(/(\t\t\t"UMG",\n)/, '').replace(/(PrivateDependencyModuleNames\.AddRange\(new string\[\]\s*\{\n)/, '$1\t\t\t"UMG",\n'),
      },
    },
  ]
);

// —— L3: اكتفاء الهيدرات العامة بذاتها ————————————————————————
detector(
  'L3',
  'كل نوع محرك في هيدر عام مُصرَّح أمامياً أو مُضمَّن',
  (vfs) => {
    const out = [];
    let scanned = 0;
    for (const k of headers(vfs)) {
      const raw = get(vfs, k);
      const body = stripComments(raw);
      const fwd = new Set([...body.matchAll(/^\s*class\s+(\w+)\s*;/gm)].map((m) => m[1]));
      const incs = [...raw.matchAll(/#include\s+"([^"]+)"/g)].map((m) => m[1]).join(' ');
      for (const t of ENGINE_TYPES) {
        if (!new RegExp(`\\b${t}\\b`).test(body)) continue;
        scanned++;
        if (fwd.has(t) || incs.includes(t.slice(1))) continue;
        out.push({ name: `${basename(k)}: ${t} بلا تصريح أمامي ولا تضمين`, ok: false });
      }
    }
    // العدد معروض حتى لا يمر الفحص خالياً لو انكسر الاستخراج يوماً
    if (out.length) return out;
    return [
      { name: `كل الهيدرات العامة مكتفية بذاتها (${scanned} إشارة نوع)`, ok: scanned > 0, detail: scanned ? '' : 'لم تُفحص أي إشارة — استخراج معطوب' },
    ];
  },
  [
    {
      why: 'حذف تصريح أمامي مع بقاء الاستخدام (اعتماد على تضمين عابر)',
      edits: {
        'Public/Rok2Perf.h': (s) => s.replace(/^class\s+UStaticMesh\s*;\s*$/m, ''),
      },
    },
  ]
);

// ───────────────────────────────────────────────────────────────────────────
//  التشغيل
// ───────────────────────────────────────────────────────────────────────────

const REAL = readTree();
if (REAL.size === 0) {
  console.error('❌ لم يُعثر على شجرة الوحدة — شغّل السكربت من جذر المستودع');
  process.exit(1);
}

let passed = 0;
let failed = 0;
const failures = [];

console.log('تدقيق مخاطر الترجمة — حرس بنيوي');
console.log(`الشجرة: ${REAL.size} ملفاً مقروءاً من game/client-unreal\n`);

// —— الطور 1: إثبات ————————————————————————————————————————
console.log('═'.repeat(62));
console.log('الطور 1 — إثبات: المدققون على الشجرة الحقيقية');
console.log('═'.repeat(62));

for (const d of DETECTORS) {
  console.log(`\n[${d.id}] ${d.title}`);
  let results;
  try {
    results = d.run(REAL);
  } catch (e) {
    console.error(`  ❌ المدقق نفسه رمى استثناءً — ${e.message}`);
    failed++;
    failures.push(`${d.id} threw: ${e.message}`);
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
  if (!VERBOSE) {
    const okCount = results.filter((r) => r.ok).length;
    if (okCount === results.length) console.log(`  ✅ ${okCount}/${results.length}`);
  }
}

// —— الطور 2: نفي ————————————————————————————————————————
console.log('\n' + '═'.repeat(62));
console.log('الطور 2 — نفي: كل مدقق على عطله الأصلي (يجب أن يفشل)');
console.log('═'.repeat(62));

let negOk = 0;
let negBad = 0;

for (const d of DETECTORS) {
  for (const b of d.breaks ?? []) {
    let detected;
    let note = '';
    try {
      if (b.selfCheck) {
        detected = b.selfCheck();
      } else {
        const broken = mutate(REAL, b.edits);
        const results = d.run(broken);
        detected = results.some((r) => !r.ok);
      }
    } catch (e) {
      // استثناء = رصد أيضاً (الشجرة المعطوبة لم تُحلَّل)، لكن نسجّله
      detected = true;
      note = ` (باستثناء: ${e.message})`;
    }
    if (detected) {
      negOk++;
      console.log(`  ✅ [${d.id}] يرصد: ${b.why}${note}`);
    } else {
      negBad++;
      console.error(`  ❌ [${d.id}] لا يرصد: ${b.why} — الحرس نفسه معطوب`);
      failures.push(`${d.id} negative test did not fire: ${b.why}`);
    }
  }
}

// —— الخلاصة ————————————————————————————————————————————
console.log('\n' + '═'.repeat(62));
console.log(`الطور 1 (إثبات): ${passed} ناجح، ${failed} فاشل`);
console.log(`الطور 2 (نفي):   ${negOk} مدققاً رصد عطله، ${negBad} لم يرصد`);
console.log('تحفّظ: تحقق بنيوي بلا مترجم C++ — لا ادّعاء ببناء ناجح.');
console.log('═'.repeat(62));

if (failed || negBad) {
  console.log('\n❌ FAILED');
  for (const f of failures) console.log(`   • ${f}`);
  process.exit(1);
}
console.log(`\n✅ ALL PASSED (${passed} فحصاً + ${negOk} اختبار نفي)`);
process.exit(0);
