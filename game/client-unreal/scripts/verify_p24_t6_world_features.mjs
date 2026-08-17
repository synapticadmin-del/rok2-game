#!/usr/bin/env node
/**
 * verify_p24_t6_world_features.mjs — P24-T6: تقطيع صفائح معالم العالم ووصلها.
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. الـ24 ملفاً `Content/Art/WorldMapIcons/T_world_*.png` **صفائح لا
 *      sprites**: `T_world_resource_nodes_quad` يحمل أربع عقد موارد في شبكة
 *      2×2 وتحت كل واحدة **نص عربي مطبوع داخل الصورة**، و
 *      `T_world_stone_gold_quarry_mine` يحمل منشأتين، وخمس صفائح أخرى تحمل
 *      جسماً رئيسياً زائد قصاصة من الجسم المجاور.
 *   2. `URok2ArtAssets::LoadWorldFeatureTexture` معرّفة **بلا أي مستدعٍ** في
 *      المشروع، ومسارها يشير إلى تلك الصفائح — فلو نُوديت لرسمت عقدةً تحمل
 *      ثلاث عقد أخرى ونصاً معكوساً.
 *   3. `slice_and_process_all_assets.py` (سكربت التقطيع القائم) يقرأ من
 *      `C:\Users\kayf\.gemini\antigravity\brain\...` — مسار محلي مؤقت لا يوجد
 *      على أي جهاز آخر، فلا يمكن إعادة توليد الأصول.
 *
 * فحص بنيوي: لا يحتاج بناء UE5 ولا محرراً يعمل.
 *
 * Usage: node scripts/verify_p24_t6_world_features.mjs
 */

import { readFileSync, existsSync, readdirSync } from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';

const here = fileURLToPath(new URL('.', import.meta.url));
const CLIENT = join(here, '..');
const REPO = join(CLIENT, '..', '..');
const ROOT = join(CLIENT, 'Source', 'Rok2');
const FEATURES = join(CLIENT, 'Content', 'Art', 'WorldFeatures');

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

/** المعالم الأحد عشر — القائمة مصدرها بيان التقطيع لا افتراض. */
const EXPECTED = [
  'farm_field', 'lumber_camp', 'stone_quarry', 'gold_mine',
  'gold_mine_large', 'barbarian_camp', 'barbarian_keep',
  'pass_fortress', 'throne_temple', 'holy_shrine', 'mountain_ridge',
];

// ---------------------------------------------------------------------------
console.log('\n[1] سكربت التقطيع — مصادره داخل المستودع');
// ---------------------------------------------------------------------------
const scriptPath = join(REPO, 'scripts', 'slice_world_feature_sprites.py');
const slicer = readOr(scriptPath);

check('scripts/slice_world_feature_sprites.py موجود', slicer.length > 0);
check('المصادر مشتقّة من جذر المستودع لا مسار محلي',
  slicer.includes('os.path.dirname(os.path.dirname(os.path.abspath(__file__)))')
  && !/[A-Za-z]:\\\\Users/.test(slicer),
  'slice_and_process_all_assets.py يقرأ من .gemini/antigravity/brain فلا يعمل على جهاز آخر');
check('يقرأ صفائح WorldMapIcons', slicer.includes('"WorldMapIcons"'));
check('يكتب إلى WorldFeatures', slicer.includes('"WorldFeatures"'));
check('وضع --dry-run للتقرير بلا كتابة', slicer.includes('--dry-run'));

// ---------------------------------------------------------------------------
console.log('\n[2] معايير القصّ قابلة للفحص لا حكم بصري');
// ---------------------------------------------------------------------------
check('عتبة شفافية ثابتٌ مُسمّى', /ALPHA_THRESHOLD\s*=\s*\d+/.test(slicer));
check('العتبة > 0 (الحواف المصفّاة تصل المركّبات)',
  /ALPHA_THRESHOLD\s*=\s*(\d+)/.exec(slicer)?.[1] > 0);
check('حد أدنى للمساحة يستبعد النص العربي المطبوع',
  /MIN_SPRITE_AREA\s*=\s*\d+/.test(slicer));
check('نسبة القصاصة تفصل الأجسام الحقيقية عن قصاصات الجيران',
  /FRAGMENT_AREA_RATIO\s*=\s*0?\.\d+/.test(slicer));
check('معيار القصاصة مزدوج (حافة + حجم نسبي)',
  slicer.includes('touches_edge(') && slicer.includes('FRAGMENT_AREA_RATIO'),
  'لمس الحافة وحده يحذف عقدتين حقيقيتين تصلان الحدّ الأيمن');
check('عدد المركّبات المتوقّع مُعلن لكل صفيحة', /"expect":\s*\d+/.test(slicer));
check('التقطيع يفشل عند اختلاف العدد المتوقّع',
  slicer.includes('مركّبات مقبولة') && slicer.includes('return 1'),
  'صمتٌ عند تغيّر الصفيحة يُنتج أصلاً خاطئاً بلا إنذار');

// ---------------------------------------------------------------------------
console.log('\n[3] النص المطبوع والقصاصات لا تدخل الـsprite');
// ---------------------------------------------------------------------------
check('القصّ يُطمس بقناع المركّب لا بمستطيل وحده',
  slicer.includes('labels[y0:y1, x0:x1] == comp["label"]'),
  'النص يقع 2 بكسل تحت قاعدة العقدة — أي هامش مستطيل يعيده');
check('الشفافية تُضرب بالقناع (تحفظ الحواف المصفّاة)',
  /pixels\[:, :, 3\] \* \(np\.array\(mask\) \/ 255\.0\)/.test(slicer),
  'استبدال الشفافية يُنتج حدوداً مسنّنة');
check('خرائط N/E تُقصّ بنفس الإحداثيات لا تُعاد توليداً',
  slicer.includes('for m in MAPS') && slicer.includes('images[m].crop('));
check('تطابق أبعاد D/N/E محروس',
  slicer.includes('أبعاد') && slicer.includes('images[m].size != (width, height)'));
check('ترتيب الأسماء مكاني بالمركز لا بالحدّ الأعلى',
  slicer.includes('c["y"] + c["h"] / 2.0'),
  'الترتيب بالحدّ الأعلى يقلب القمح والأخشاب لأن الأشجار أعلى من السنابل');

// ---------------------------------------------------------------------------
console.log('\n[4] الأصول الناتجة على القرص');
// ---------------------------------------------------------------------------
check('مجلد WorldFeatures موجود', existsSync(FEATURES));
const files = existsSync(FEATURES) ? readdirSync(FEATURES) : [];

for (const id of EXPECTED) {
  for (const map of ['D', 'N', 'E']) {
    check(`T_feat_${id}_${map}.png`, files.includes(`T_feat_${id}_${map}.png`));
  }
}
check('33 صورة (11 معلماً × 3 خرائط)',
  files.filter((f) => /^T_feat_.*\.png$/.test(f)).length === 33);
check('33 أصلاً مستورداً (.uasset)',
  files.filter((f) => /^T_feat_.*\.uasset$/.test(f)).length === 33,
  'الاستيراد ناقص — شغّل scripts/Import-WorldFeatureSprites.ps1');

// بيان الإحداثيات: يجعل القصّ قابلاً للمراجعة بلا إعادة تشغيل.
const manifestPath = join(FEATURES, 'sprites.json');
check('بيان sprites.json موجود', existsSync(manifestPath));
if (existsSync(manifestPath)) {
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  check('البيان يسجّل معايير التقطيع',
    typeof manifest.alphaThreshold === 'number'
    && typeof manifest.minSpriteArea === 'number'
    && typeof manifest.padding === 'number');
  check('البيان يغطي المعالم الأحد عشر',
    EXPECTED.every((id) => manifest.sprites && manifest.sprites[id]));
  check('كل مدخل يذكر صفيحته الأصلية',
    EXPECTED.every((id) => typeof manifest.sprites?.[id]?.sheet === 'string'));
  check('عقد الموارد الأربع من صفيحة الشبكة نفسها',
    ['farm_field', 'lumber_camp', 'stone_quarry', 'gold_mine']
      .every((id) => manifest.sprites?.[id]?.sheet === 'T_world_resource_nodes_quad'),
    'أربع عقد كانت في صورة واحدة');
  // النوافذ **تتقاطع بمقدار الهامش** عن قصد: `PAD` يوسّع كل نافذة 6 بكسل
  // فتتلامس نوافذ العقد المتجاورة. وهذا غير ضار لأن القصّ يُطمس بقناع المركّب،
  // فالبكسل الغريب يخرج شفافاً. ما يجب ألّا يتقاطع هو **حدود الأجسام نفسها**
  // (النافذة منقوصة الهامش) — تقاطعها يعني جسمين متصلين قُطعا خطأً.
  const padding = manifest.padding ?? 0;
  check('حدود الأجسام (بلا الهامش) لا تتقاطع',
    (() => {
      const boxes = ['farm_field', 'lumber_camp', 'stone_quarry', 'gold_mine']
        .map((id) => manifest.sprites[id].crop)
        .map(([x0, y0, x1, y1]) => [x0 + padding, y0 + padding, x1 - padding, y1 - padding]);
      for (let i = 0; i < boxes.length; i++) {
        for (let j = i + 1; j < boxes.length; j++) {
          const [ax0, ay0, ax1, ay1] = boxes[i];
          const [bx0, by0, bx1, by1] = boxes[j];
          if (ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1) return false;
        }
      }
      return true;
    })(),
    'تقاطع جسمين يعني مركّباً واحداً قُطع خطأً إلى اثنين');
  check('التقاطع لا يتجاوز الهامش على أي محور',
    (() => {
      const boxes = ['farm_field', 'lumber_camp', 'stone_quarry', 'gold_mine']
        .map((id) => manifest.sprites[id].crop);
      for (let i = 0; i < boxes.length; i++) {
        for (let j = i + 1; j < boxes.length; j++) {
          const [ax0, ay0, ax1, ay1] = boxes[i];
          const [bx0, by0, bx1, by1] = boxes[j];
          const overlapX = Math.min(ax1, bx1) - Math.max(ax0, bx0);
          const overlapY = Math.min(ay1, by1) - Math.max(ay0, by0);
          if (overlapX > 0 && overlapY > 0
            && overlapX > 2 * padding && overlapY > 2 * padding) {
            return false;
          }
        }
      }
      return true;
    })(),
    'تقاطع أكبر من الهامش على المحورين معاً يعني قصّاً خاطئاً لا توسيعاً');
}

// ---------------------------------------------------------------------------
console.log('\n[5] القارئ في URok2ArtAssets');
// ---------------------------------------------------------------------------
const artH = readOr(join(ROOT, 'Public', 'Rok2ArtAssets.h'));
const artC = strip(readOr(join(ROOT, 'Private', 'Rok2ArtAssets.cpp')));

check('المسار يشير إلى WorldFeatures لا WorldMapIcons',
  artC.includes('/Game/Art/WorldFeatures/T_feat_'),
  'كان يشير إلى الصفائح نفسها');
check('لا مسار T_world_ باقٍ في قارئ المعالم',
  !fnBody(artC, 'FString URok2ArtAssets::GetWorldFeatureTextureAssetPath').includes('T_%s_%s'));
check('مجموعة معرّفات مغلقة (لا تحميل مسار مخترع)',
  artC.includes('WorldFeatureSpriteIds()'));
check('المعرّفات الأحد عشر كلها مسجّلة',
  EXPECTED.every((id) => artC.includes(`TEXT("${id}")`)),
  EXPECTED.filter((id) => !artC.includes(`TEXT("${id}")`)).join(', '));
check('معرّف غير معروف يعيد سلسلة فارغة',
  fnBody(artC, 'FString URok2ArtAssets::GetWorldFeatureTextureAssetPath').includes('return FString();'));
check('التحميل لا يجرّب مساراً فارغاً',
  fnBody(artC, 'UTexture2D* URok2ArtAssets::LoadWorldFeatureTexture').includes('Path.IsEmpty() ? nullptr'));
check('HasWorldFeatureSprite معلنة للتشخيص',
  artH.includes('static bool HasWorldFeatureSprite('));

// الخريطة من نوع العقدة إلى المعلم.
check('WorldFeatureIdForNode معلنة', artH.includes('static FString WorldFeatureIdForNode('));
const mapper = fnBody(artC, 'FString URok2ArtAssets::WorldFeatureIdForNode');
check('يفهم أنواع الموارد الأربعة من الخادم',
  ['food', 'wood', 'stone', 'gold'].every((k) => mapper.includes(`TEXT("${k}")`)));
check('يفهم البرابرة', mapper.includes('TEXT("barb")'));
check('المستوى يرفع الرسم إلى نسخته الأوفى',
  mapper.includes('Level >= 4') && mapper.includes('gold_mine_large') && mapper.includes('barbarian_keep'),
  'التراتب يُقرأ من الشكل لا من رقم وحده');
check('نوع غير معروف يعيد فارغاً (يبقى الراسم على أيقونته)',
  mapper.includes('return FString();'));

// ---------------------------------------------------------------------------
console.log('\n[6] الوصل في ARok2WorldRenderer');
// ---------------------------------------------------------------------------
const rendH = readOr(join(ROOT, 'Public', 'Rok2WorldRenderer.h'));
const rendC = strip(readOr(join(ROOT, 'Private', 'Rok2WorldRenderer.cpp')));

check('LoadWorldFeatureTexture لها مستدعٍ',
  rendC.includes('URok2ArtAssets::LoadWorldFeatureTexture('),
  'كانت معرّفة بلا مستدعٍ واحد في المشروع');
check('العقد تستخدم WorldFeatureIdForNode',
  rendC.includes('URok2ArtAssets::WorldFeatureIdForNode(N.Kind, N.Level)'));
check('الممر والعرش لهما معلماهما',
  rendC.includes('TEXT("throne_temple")') && rendC.includes('TEXT("pass_fortress")'));

// ثلاث طبقات احتياط: الرسم المقطّع ← الأيقونة المسطّحة ← الشكل الهندسي.
const nodeSection = rendC.slice(rendC.indexOf('for (const FRok2NodeEntity& N : W.Nodes)'));
const iFeature = nodeSection.indexOf('LoadWorldFeatureTexture');
const iIcon = nodeSection.indexOf('LoadWorldMapIcon');
const iMesh = nodeSection.indexOf('NodeMesh');
check('ترتيب الاحتياط: معلم ← أيقونة ← شكل هندسي',
  iFeature >= 0 && iFeature < iIcon && iIcon < iMesh,
  'الرسم الأوفى أولاً، والانحدار عند غيابه لا انهيار');
check('الأيقونة المسطّحة تبقى احتياطاً (else if)',
  /else if \(UTexture2D\* Icon = URok2ArtAssets::LoadWorldMapIcon/.test(rendC));

check('مضاعف الحجم ثابتٌ قابل للتعديل من المحرر',
  /UPROPERTY\(EditAnywhere, Category = "Rok2\|World Features"\)\s*\n\s*float WorldFeatureSpriteScale/.test(rendH),
  'الحكم على الحجم بصري — لا يُثبَّت من غير رؤية (P24-T7)');
check('المضاعف يُطبَّق على مقياس الأيقونة',
  rendC.includes('Style.WorldScale * WorldFeatureSpriteScale'));

// ---------------------------------------------------------------------------
console.log('\n[7] سكربت الاستيراد');
// ---------------------------------------------------------------------------
const importPs = readOr(join(CLIENT, 'scripts', 'Import-WorldFeatureSprites.ps1'));
check('Import-WorldFeatureSprites.ps1 موجود', importPs.length > 0);
check('يتحقق من UE 5.4.4', importPs.includes("-ne '5.4.4'"));
check('يشغّل التقطيع قبل الاستيراد',
  importPs.includes('slice_world_feature_sprites.py'),
  'وإلا استُورد أصلٌ قديم بعد تعديل معايير التقطيع');
check('يرفض حزمة ناقصة قبل الاستيراد', importPs.includes('-ne 33'));
check('البوابة على الناتج لا على شفرة الخروج',
  importPs.includes("Filter 'T_feat_*.uasset'") && importPs.includes('$Assets.Count -ne $Sprites.Count'),
  '-run=ImportAssets يعيد 1 مع Invalid Destination Path حتى عند نجاح كل الملفات');
check('الوجهة /Game/Art/WorldFeatures', importPs.includes("'/Game/Art/WorldFeatures'"));

// ---------------------------------------------------------------------------
console.log('\n' + '='.repeat(60));
console.log(`P24-T6 structural verification: ${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
