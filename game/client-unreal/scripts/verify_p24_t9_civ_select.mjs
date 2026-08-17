#!/usr/bin/env node
/**
 * P24-T9 — حارس شاشة اختيار الحضارات.
 *
 * العطل المُبلَّغ من مالك المشروع بلقطة شاشة: البطاقة تعرض **رقعة شطرنج**
 * ضخمة بدل شعار الحضارة، والشعار يُرسم بحجمه الأصلي فيطرد النصوص وأزرار
 * التنقّل وزر «ابدأ رحلة» خارج حدود البطاقة.
 *
 * سببان مستقلان، لكل منهما فحوصه هنا:
 *
 *   (١) ثلاثة من الشعارات الستة تحمل رقعة الشطرنج **مطبوعة في البكسل** بقناة
 *       شفافية معتمة بالكامل (alpha=255 في كل بكسل). ما يعني «شفاف» في محرر
 *       الصور صار بكسلات رمادية حقيقية.
 *
 *   (٢) `SetDesiredSizeOverride` **لا يُخزَّن في `UImage`**: التنفيذ يمرّره إلى
 *       `MyImage` إن كانت صالحة ولا يحفظ شيئاً، ولا يُعاد تطبيقه في
 *       `SynchronizeProperties`. فنداؤه داخل `NativeConstruct` على ودجة بُنيت
 *       لحظتها (قبل `Super::RebuildWidget`) بلا أثر. و`SetBrushFromTexture(Tex,
 *       true)` كان يضبط `ImageSize` إلى مقاس الملف الكامل.
 *
 * الفحص بنيوي: لا محرك ولا مُصرِّف هنا. لكن (١) يُقاس **من بايتات PNG نفسها**
 * لا من نصّ الكود — فالتحقق حقيقي لا تعبيري.
 */
import { readFileSync, existsSync } from 'node:fs';
import { join, dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateSync } from 'node:zlib';
import process from 'node:process';

const scriptDir = dirname(fileURLToPath(import.meta.url));
const CLIENT = resolve(scriptDir, '..');
const REPO = resolve(CLIENT, '..', '..');
const SRC = join(CLIENT, 'Source', 'Rok2');
const ICONS = join(CLIENT, 'Content', 'Art', 'CivIcons');
const CIVS = ['rome', 'china', 'arabia', 'egypt', 'vikings', 'japan'];

const NEUTRAL_SATURATION = 8;
const NEUTRAL_MIN_VALUE = 225;

let passed = 0;
let failed = 0;

function check(name, condition, detail = '') {
  if (condition) {
    console.log(`OK  : ${name}`);
    passed += 1;
  } else {
    console.error(`FAIL: ${name}${detail ? ` — ${detail}` : ''}`);
    failed += 1;
  }
}

function read(rel) {
  return readFileSync(join(SRC, rel), 'utf8').replace(/\r\n/g, '\n');
}

/** يستخرج جسم دالة بمُوازنة الأقواس — نافذة ثابتة الطول تكذب عند إدراج سطر. */
function fnBody(code, signature) {
  const start = code.indexOf(signature);
  if (start < 0) return '';
  const open = code.indexOf('{', start);
  if (open < 0) return '';
  let depth = 0;
  for (let i = open; i < code.length; i += 1) {
    if (code[i] === '{') depth += 1;
    else if (code[i] === '}') {
      depth -= 1;
      if (depth === 0) return code.slice(open, i + 1);
    }
  }
  return '';
}

// ---------------------------------------------------------------------------
// قارئ PNG بالحدّ الأدنى: يفكّ IHDR + IDAT ويعيد بكسلات RGBA.
// (لا اعتماد على حزمة خارجية — الحارس يجب أن يعمل على مستودع نظيف.)
// ---------------------------------------------------------------------------
function decodePng(buffer) {
  if (buffer.readUInt32BE(0) !== 0x89504e47) throw new Error('ليس ملف PNG');
  let offset = 8;
  let header = null;
  const idat = [];
  while (offset < buffer.length) {
    const length = buffer.readUInt32BE(offset);
    const type = buffer.toString('ascii', offset + 4, offset + 8);
    const data = buffer.subarray(offset + 8, offset + 8 + length);
    if (type === 'IHDR') {
      header = {
        width: data.readUInt32BE(0),
        height: data.readUInt32BE(4),
        depth: data[8],
        colorType: data[9],
        interlace: data[12],
      };
    } else if (type === 'IDAT') {
      idat.push(data);
    } else if (type === 'IEND') {
      break;
    }
    offset += 12 + length;
  }
  if (!header) throw new Error('لا IHDR');
  if (header.depth !== 8) throw new Error(`عمق غير مدعوم: ${header.depth}`);
  if (header.interlace !== 0) throw new Error('PNG متشابك غير مدعوم');
  const channels = header.colorType === 6 ? 4 : header.colorType === 2 ? 3 : 0;
  if (!channels) throw new Error(`نوع لون غير مدعوم: ${header.colorType}`);

  const raw = inflateSync(Buffer.concat(idat));
  const { width, height } = header;
  const stride = width * channels;
  const out = Buffer.alloc(width * height * 4);
  let previous = Buffer.alloc(stride);

  for (let y = 0; y < height; y += 1) {
    const filter = raw[y * (stride + 1)];
    const line = Buffer.from(raw.subarray(y * (stride + 1) + 1, y * (stride + 1) + 1 + stride));
    for (let i = 0; i < stride; i += 1) {
      const a = i >= channels ? line[i - channels] : 0;
      const b = previous[i];
      const c = i >= channels ? previous[i - channels] : 0;
      switch (filter) {
        case 0: break;
        case 1: line[i] = (line[i] + a) & 0xff; break;
        case 2: line[i] = (line[i] + b) & 0xff; break;
        case 3: line[i] = (line[i] + ((a + b) >> 1)) & 0xff; break;
        case 4: {
          const p = a + b - c;
          const pa = Math.abs(p - a);
          const pb = Math.abs(p - b);
          const pc = Math.abs(p - c);
          const pred = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
          line[i] = (line[i] + pred) & 0xff;
          break;
        }
        default: throw new Error(`مرشّح غير معروف: ${filter}`);
      }
    }
    for (let x = 0; x < width; x += 1) {
      const s = x * channels;
      const d = (y * width + x) * 4;
      out[d] = line[s];
      out[d + 1] = line[s + 1];
      out[d + 2] = line[s + 2];
      out[d + 3] = channels === 4 ? line[s + 3] : 0xff;
    }
    previous = line;
  }
  return { width, height, pixels: out };
}

/** يقيس البكسلات الرمادية الفاتحة **المعتمة** — أي رقعة الشطرنج المطبوعة. */
function measurePrintedBackground(path) {
  const { width, height, pixels } = decodePng(readFileSync(path));
  let opaqueNeutralLight = 0;
  let transparent = 0;
  for (let i = 0; i < pixels.length; i += 4) {
    const r = pixels[i];
    const g = pixels[i + 1];
    const b = pixels[i + 2];
    const a = pixels[i + 3];
    if (a === 0) transparent += 1;
    if (a === 255) {
      const max = Math.max(r, g, b);
      const min = Math.min(r, g, b);
      if (max - min <= NEUTRAL_SATURATION && min >= NEUTRAL_MIN_VALUE) opaqueNeutralLight += 1;
    }
  }
  return { width, height, total: width * height, transparent, opaqueNeutralLight };
}

// ---------------------------------------------------------------------------
console.log('\n[1] الشعارات الستة: لا رقعة شطرنج مطبوعة، وشفافية حقيقية');
// ---------------------------------------------------------------------------
// السقف 2% لا صفر: الشعارات تحمل فضّة وحجراً وبياضاً حقيقياً في الرسم — إلزام
// الصفر يمنع أي رمادٍ فاتح مشروع. المقيس بعد الإصلاح 0.13%–1.10%، وقبله 46%–55%.
const PRINTED_BACKGROUND_MAX_RATIO = 0.02;
// أدنى شفافية: الشعار جسم دائري في مربع، فربع الصورة على الأقل خارجه.
const MIN_TRANSPARENT_RATIO = 0.20;

for (const civ of CIVS) {
  const path = join(ICONS, `icon_${civ}_runtime.png`);
  if (!existsSync(path)) {
    check(`icon_${civ}_runtime.png موجود`, false, path);
    continue;
  }
  let stats;
  try {
    stats = measurePrintedBackground(path);
  } catch (error) {
    check(`icon_${civ}_runtime.png مقروء`, false, error.message);
    continue;
  }
  const printedRatio = stats.opaqueNeutralLight / stats.total;
  const transparentRatio = stats.transparent / stats.total;
  check(`${civ}: بلا رقعة شطرنج مطبوعة`,
    printedRatio <= PRINTED_BACKGROUND_MAX_RATIO,
    `${(printedRatio * 100).toFixed(2)}% بكسلات رمادية فاتحة معتمة`);
  check(`${civ}: خلفية شفافة فعلاً`,
    transparentRatio >= MIN_TRANSPARENT_RATIO,
    `${(transparentRatio * 100).toFixed(2)}% شفاف`);
}

// ---------------------------------------------------------------------------
console.log('\n[2] سكربت الفصل: مصدره من المستودع ومعاييره ثوابت مسمّاة');
// ---------------------------------------------------------------------------
const stripperPath = join(REPO, 'scripts', 'strip_civ_emblem_background.py');
check('scripts/strip_civ_emblem_background.py موجود', existsSync(stripperPath));
if (existsSync(stripperPath)) {
  const py = readFileSync(stripperPath, 'utf8').replace(/\r\n/g, '\n');
  check('العتبات ثوابت مسمّاة',
    py.includes('NEUTRAL_SATURATION = 8') && py.includes('NEUTRAL_MIN_VALUE = 225'));
  check('الخلفية تُحدَّد بالاتصال بالحافة لا باللون وحده',
    py.includes('def border_connected') && py.includes('REPO_ROOT'));
  check('لا مسار محلي مؤقت',
    !/[A-Za-z]:\\\\Users\\\\/.test(py) && !py.includes('.gemini'));
  check('لون البكسل الشفاف يُسرَّب فلا هالة بيضاء عند التصغير',
    py.includes('def bleed_color_into_transparent') && py.includes('BLEED_ITERATIONS'));
  check('تشغيل مكرر آمن (ملف بشفافية يُتخطّى)', py.includes('"action": "skipped"'));
  // التقرير خارج Content: أي ملف غير أصل داخل مجلد المحتوى يفتح حوار استيراد
  // في محرر UE — وهذا ما حدث فعلاً مع alpha_report.json.
  check('تقرير القياس خارج مجلد Content',
    py.includes('"game" / "docs"') && !py.includes('ICON_DIR / "alpha_report.json"'));
}
const reportPath = join(REPO, 'game', 'docs', 'P24_T9_CIV_EMBLEM_ALPHA.json');
check('تقرير القياس مُلتزم في game/docs', existsSync(reportPath));
check('لا تقرير داخل Content/Art/CivIcons', !existsSync(join(ICONS, 'alpha_report.json')));
if (existsSync(reportPath)) {
  const report = JSON.parse(readFileSync(reportPath, 'utf8'));
  check('التقرير يسجّل الشعارات الستة', Array.isArray(report.icons) && report.icons.length === 6);
  check('التقرير يسجّل المعايير المستخدمة',
    report.criteria?.neutralSaturation === NEUTRAL_SATURATION
    && report.criteria?.neutralMinValue === NEUTRAL_MIN_VALUE);
}

// ---------------------------------------------------------------------------
console.log('\n[3] BootWidget: مقاس رسم صريح لا مقاس الملف');
// ---------------------------------------------------------------------------
const bootCpp = read('Private/Rok2BootWidget.cpp');

check('دالة واحدة تضع النسيج بمقاس صريح', bootCpp.includes('static void ApplyTextureAtSize'));
check('المقاس يُثبَّت في Brush.ImageSize (يبقى بعد البناء)',
  fnBody(bootCpp, 'static void ApplyTextureAtSize').includes('Brush.ImageSize = DrawSize'));
check('الـoverride يُستدعى بعده ليسري على ودجة معروضة',
  fnBody(bootCpp, 'static void ApplyTextureAtSize').includes('SetDesiredSizeOverride(DrawSize)'));
check('لا SetBrushFromTexture بمطابقة مقاس الملف في البطاقة المرئية',
  !bootCpp.includes('SetBrushFromTexture(Texture, true)'));

const showcaseVisuals = fnBody(bootCpp, 'void URok2BootWidget::ShowCivVisuals');
check('ShowCivVisuals موجودة', showcaseVisuals.length > 0);
check('الخلفية بمقاس البطاقة لا 2560×1440',
  showcaseVisuals.includes('ApplyTextureAtSize(CivBackdropImage')
  && showcaseVisuals.includes('ShowcaseWidth'));
check('الشعار بمقاس EmblemSize لا 1920×1920',
  showcaseVisuals.includes('ApplyTextureAtSize(CivEmblemImage')
  && showcaseVisuals.includes('EmblemSize'));
check('صورة القائد بمقاس CommanderSize',
  showcaseVisuals.includes('ApplyTextureAtSize(CivCommanderImage')
  && showcaseVisuals.includes('CommanderSize'));

// ---------------------------------------------------------------------------
console.log('\n[4] التخطيط: حدّ للبطاقة، تمديد للطبقات، ولا محتوى مقصوص');
// ---------------------------------------------------------------------------
const buildShowcase = fnBody(bootCpp, 'void URok2BootWidget::BuildCivShowcase');
check('BuildCivShowcase موجودة', buildShowcase.length > 0);
check('SizeBox يحدّ البطاقة المرئية',
  buildShowcase.includes('CivShowcaseBounds')
  && buildShowcase.includes('SetWidthOverride')
  && buildShowcase.includes('SetHeightOverride'));
check('طبقات الـOverlay تتمدد صريحاً (HAlign_Fill/VAlign_Fill)',
  (buildShowcase.match(/SetHorizontalAlignment\(HAlign_Fill\)/g) || []).length >= 3
  && (buildShowcase.match(/SetVerticalAlignment\(VAlign_Fill\)/g) || []).length >= 3);
check('عمود النصّ على Fill فيلتفّ داخل البطاقة',
  buildShowcase.includes('TextColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill))'));
check('صف التفاصيل يمتد بعرض البطاقة',
  buildShowcase.includes('DetailsSlot->SetHorizontalAlignment(HAlign_Fill)'));

const nativeConstruct = fnBody(bootCpp, 'void URok2BootWidget::NativeConstruct');
check('ScrollBox داخل البطاقة فلا يُقصّ زر البدء',
  nativeConstruct.includes('UScrollBox')
  && nativeConstruct.includes('CardScroll')
  && nativeConstruct.includes('Orient_Vertical'));
check('ScrollBox.h مضمَّن', bootCpp.includes('#include "Components/ScrollBox.h"'));
check('OverlaySlot.h مضمَّن', bootCpp.includes('#include "Components/OverlaySlot.h"'));

// ---------------------------------------------------------------------------
console.log('\n[5] نظام التصميم: الأسطح والخطوط من طبقاتها');
// ---------------------------------------------------------------------------
check('سطح البطاقة من Rok2Surface', buildShowcase.includes('Rok2Surface::Card()'));
check('الحجاب من Rok2Surface', buildShowcase.includes('Rok2Surface::Scrim()'));
check('أزرار التنقّل بنمط ثانوي مشترك',
  (buildShowcase.match(/Rok2Surface::SecondaryButton\(\)/g) || []).length === 2);
check('كل نصّ بدور من URok2Typography',
  (buildShowcase.match(/URok2Typography::ApplyFont/g) || []).length >= 6);
check('لا SetBrushColor في الملف', !bootCpp.includes('SetBrushColor'));

// ---------------------------------------------------------------------------
console.log('\n[6] الوظيفة داخل بوابة check');
// ---------------------------------------------------------------------------
const { chainRuns } = await import('../../../scripts/lib/npm_script_chain.mjs');
const pkg = JSON.parse(readFileSync(join(REPO, 'game', 'backend', 'package.json'), 'utf8'));
check('test:p24-t9-civ-select في سلسلة check',
  chainRuns(pkg.scripts, 'test:p24-t9-civ-select'));

console.log('\n==== RESULT ====');
console.log(`${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
