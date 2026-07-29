#!/usr/bin/env node
/**
 * verify_delegate_bind.mjs — إصلاح AddDynamic: تحقق بنيوي
 *
 * الخطأ الذي يحرسه هذا السكربت:
 *   Btn->OnClicked.AddDynamic(this, Handler);   // Handler من نوع FName
 *
 * `AddDynamic` ماكرو يتوسّع إلى __Internal_AddDynamic(Obj, FuncName, FName)
 * ومعامله الثاني مؤشر دالة عضو — فتمرير متغيّر FName لا يُترجم أصلاً. وحتى لو
 * تُرجم، `#FuncName` يحوّل *اسم المعامل* نصّياً فيصير الاسم المربوط الحرفي
 * "Handler" لا قيمة المتغيّر.
 *
 * البديل الصحيح: Rok2BindClickByName (Public/Rok2DelegateBind.h) الذي يستخدم
 * FScriptDelegate::BindUFunction — ربط بالاسم يُحلّ في وقت التشغيل.
 *
 * ولأن الربط بالاسم يفشل **بصمت** حين لا توجد الدالة، أهم فحص هنا هو الفحص [4]:
 * كل اسم مُعالج يُمرَّر في الكود لا بد أن يقابله UFUNCTION() بتوقيع void بلا
 * معاملات في هيدر الصنف المالك. هذا يمنع «زراً ميتاً» يعود مستقبلاً.
 *
 * Static/structural test — does not require a running UE5 build or backend.
 *
 * Usage: node scripts/verify_delegate_bind.mjs
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
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

/** يزيل تعليقات C++ حتى لا تُحسب أمثلة التوثيق كأنها كود حقيقي */
function stripComments(src) {
  return src
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/^[ \t]*\/\/.*$/gm, '')
    .replace(/\/\/.*$/gm, '');
}

// المواضع الخمسة التي كانت معطوبة، والصنف المالك لكل ملف
const SITES = [
  { cpp: 'Rok2HudWidget.cpp', h: 'Rok2HudWidget.h', cls: 'URok2HudWidget', expected: 2 },
  { cpp: 'Rok2BuildMenuWidget.cpp', h: 'Rok2BuildMenuWidget.h', cls: 'URok2BuildMenuWidget', expected: 1 },
  { cpp: 'Rok2BuildingDetailWidget.cpp', h: 'Rok2BuildingDetailWidget.h', cls: 'URok2BuildingDetailWidget', expected: 1 },
  { cpp: 'Rok2CommanderWidget.cpp', h: 'Rok2CommanderWidget.h', cls: 'URok2CommanderWidget', expected: 1 },
];

// ---------------------------------------------------------------------------
// 1. الدالة المساعدة موجودة وبالشكل الصحيح
// ---------------------------------------------------------------------------
console.log('\n[1] Rok2DelegateBind.h — الدالة المساعدة للربط بالاسم');

const bindH = join(ROOT, 'Public', 'Rok2DelegateBind.h');
check('Rok2DelegateBind.h exists', existsSync(bindH));

const bh = readOr(bindH);
const bhCode = stripComments(bh);

if (bh) {
  check('has #pragma once', bh.includes('#pragma once'));
  check('declares inline Rok2BindClickByName', /inline\s+bool\s+Rok2BindClickByName\s*\(/.test(bhCode));
  check('takes (UButton*, UObject*, FName)',
    /Rok2BindClickByName\s*\(\s*UButton\*\s*\w+\s*,\s*UObject\*\s*\w+\s*,\s*const\s+FName\s+\w+\s*\)/.test(bhCode));
  check('includes Components/Button.h (brings FScriptDelegate machinery)',
    bh.includes('#include "Components/Button.h"'));

  // جوهر الإصلاح: الربط بالاسم عبر FScriptDelegate
  check('uses FScriptDelegate', /\bFScriptDelegate\b/.test(bhCode));
  check('uses BindUFunction (name-based binding)', /\.BindUFunction\s*\(/.test(bhCode));
  check('adds via OnClicked.Add (not AddDynamic)', /OnClicked\.Add\s*\(/.test(bhCode));
  check('does NOT use AddDynamic itself', !/AddDynamic/.test(bhCode));

  // الحرس ضد الفشل الصامت
  check('guards with FindFunction', /FindFunction\s*\(/.test(bhCode));
  check('guard is an ensure (loud in development)', /ensureMsgf\s*\(/.test(bhCode));

  // صلابة المعاملات
  check('null-guards its parameters', /!Button\s*\|\|\s*!Target/.test(bhCode));
  check('rejects NAME_None', /IsNone\s*\(\s*\)/.test(bhCode));

  // لا حاجة لـ generated.h — دالة حرة لا UCLASS
  check('is a plain free function (no generated.h needed)',
    !bhCode.includes('.generated.h') && !/\bUCLASS\b/.test(bhCode));
}

// ---------------------------------------------------------------------------
// 2. صنف الخطأ اختفى من كل الموديول
// ---------------------------------------------------------------------------
console.log('\n[2] لا AddDynamic بمعامل ليس مؤشر دالة عضو (صنف الخطأ)');

const allSources = [];
for (const dir of ['Private', 'Public']) {
  const d = join(ROOT, dir);
  if (!existsSync(d)) continue;
  for (const f of readdirSync(d)) {
    if (f.endsWith('.cpp') || f.endsWith('.h')) {
      allSources.push({ file: `${dir}/${f}`, src: stripComments(readOr(join(d, f))) });
    }
  }
}
check('module sources found', allSources.length > 0, `${allSources.length} files`);

// كل AddDynamic صحيح شكله AddDynamic(Obj, &Class::Method)
const badSites = [];
for (const { file, src } of allSources) {
  const re = /\.Add(?:Unique)?Dynamic\s*\(([^;]*?)\)\s*;/g;
  let m;
  while ((m = re.exec(src)) !== null) {
    const args = m[1];
    const comma = args.indexOf(',');
    if (comma === -1) continue;
    const second = args.slice(comma + 1).trim();
    if (!second.startsWith('&')) {
      const line = src.slice(0, m.index).split('\n').length;
      badSites.push(`${file}:${line} → ${second}`);
    }
  }
}
check('zero AddDynamic/AddUniqueDynamic with non-member-pointer 2nd arg',
  badSites.length === 0, badSites.join(' | '));

// وتحديداً: النمط الحرفي القديم لم يعد موجوداً
const oldPattern = allSources.filter(({ src }) => /AddDynamic\s*\(\s*this\s*,\s*Handler\s*\)/.test(src));
check('old `AddDynamic(this, Handler)` pattern fully gone',
  oldPattern.length === 0, oldPattern.map((s) => s.file).join(', '));

// حراسة عكسية: المواضع السليمة لم تُلمس
const goodCount = allSources.reduce(
  (n, { src }) => n + (src.match(/\.Add(?:Unique)?Dynamic\s*\(\s*[^,]+,\s*&/g) || []).length, 0);
check('correct member-pointer AddDynamic sites still intact', goodCount >= 50, `${goodCount} sites`);

// ---------------------------------------------------------------------------
// 3. كل موضع كان معطوباً يستخدم الدالة المساعدة الآن
// ---------------------------------------------------------------------------
console.log('\n[3] المواضع الخمسة تستخدم Rok2BindClickByName');

let totalHelperCalls = 0;
for (const site of SITES) {
  const src = readOr(join(ROOT, 'Private', site.cpp));
  const code = stripComments(src);
  check(`${site.cpp} exists`, src.length > 0);
  if (!src) continue;

  check(`${site.cpp} includes Rok2DelegateBind.h`,
    src.includes('#include "Rok2DelegateBind.h"'));

  const calls = (code.match(/Rok2BindClickByName\s*\(/g) || []).length;
  totalHelperCalls += calls;
  check(`${site.cpp} calls helper ×${site.expected}`, calls === site.expected, `found ${calls}`);

  // المعالج ما زال يُمرَّر كـ FName إلى lambda مشتركة (شكل الاستدعاء سليم)
  check(`${site.cpp} passes (Btn, this, Handler)`,
    /Rok2BindClickByName\s*\(\s*\w+\s*,\s*this\s*,\s*Handler\s*\)/.test(code));
}
check('helper called exactly 5 times across module', totalHelperCalls === 5, `found ${totalHelperCalls}`);

// ---------------------------------------------------------------------------
// 4. الفحص الأهم — كل اسم مُعالج يُحلّ فعلاً إلى UFUNCTION
//    (الربط بالاسم يفشل بصمت، فهذا ما يمنع «زراً ميتاً»)
// ---------------------------------------------------------------------------
console.log('\n[4] كل اسم مُعالج يقابله UFUNCTION() بتوقيع void بلا معاملات');

let handlerCount = 0;
for (const site of SITES) {
  const code = stripComments(readOr(join(ROOT, 'Private', site.cpp)));
  const header = readOr(join(ROOT, 'Public', site.h));
  check(`${site.h} exists`, header.length > 0);
  if (!header) continue;

  const names = [...code.matchAll(/FName\(TEXT\("([A-Za-z_]\w*)"\)\)/g)].map((m) => m[1]);
  check(`${site.cpp} passes handler names`, names.length > 0, `${names.length} found`);

  for (const name of new Set(names)) {
    handlerCount++;
    // UFUNCTION() إما على نفس السطر أو في الأسطر الثلاثة السابقة
    const decl = new RegExp(
      `(?:UFUNCTION\\s*\\([^)]*\\)[\\s\\S]{0,120}?)\\bvoid\\s+${name}\\s*\\(\\s*\\)\\s*;`
    );
    check(`${site.cls}::${name} is a UFUNCTION() void()`, decl.test(header));
  }
}
check('all handler names accounted for', handlerCount === 16, `${handlerCount} names`);

// ---------------------------------------------------------------------------
// 5. سلامة عامة
// ---------------------------------------------------------------------------
console.log('\n[5] سلامة البناء');

// الدالة المساعدة inline في هيدر — لا cpp مطلوب ولا تعديل Build.cs
check('no Rok2DelegateBind.cpp needed (header-only)',
  !existsSync(join(ROOT, 'Private', 'Rok2DelegateBind.cpp')));

const buildCs = readOr(join(ROOT, 'Rok2.Build.cs'));
check('Rok2.Build.cs depends on UMG (UButton)', buildCs.includes('"UMG"'));
check('Rok2.Build.cs unchanged by this fix (no new module needed)',
  !buildCs.includes('Rok2DelegateBind'));

// توازن الأقواس في الهيدر الجديد
if (bh) {
  const braces = (bh.match(/\{/g) || []).length - (bh.match(/\}/g) || []).length;
  check('Rok2DelegateBind.h braces balanced', braces === 0, `delta ${braces}`);
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(50));
console.log(`AddDynamic fix structural verification: ${passed} passed, ${failed} failed`);

if (failed > 0) {
  console.log('\n❌ FAILED');
  process.exit(1);
} else {
  console.log('\n✅ ALL PASSED');
  process.exit(0);
}
