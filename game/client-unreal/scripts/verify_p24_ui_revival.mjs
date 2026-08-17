#!/usr/bin/env node
/**
 * verify_p24_ui_revival.mjs — حرس مرحلة إحياء الواجهة (P24).
 *
 * ما يحرسه — كل بند مربوط بالعطل الذي أُصلح لا بوجود كود عام:
 *   T1  لا انحدار إلى لوح مبني ثم مخفي بلا مسار إظهار، وأوامر المدينة الثلاثة
 *       لها مستدعٍ في ودجة مرئية.
 *   T2  الأيقونات المستوردة تطابق ما هو على القرص فعلاً (لا قائمة تتعفّن).
 *   T3  الأسطح نسيجية مع سقوط إلى اللون، وجلود الأزرار مركزية.
 *   T4  الأوجه تُبنى من أصول FontFace بأوزان مسمّاة، والتدهور اللطيف قائم.
 *   T5  الأصول المستوردة لها قارئ فعلي (لا أصل يُحزَّم بلا مستهلك).
 *
 * تحفّظ: تحقق بنيوي وفحص وجود ملفات — لا مترجم C++ هنا ولا حكم على الشكل على
 * الشاشة (ذلك بند P24-T7).
 */

import { existsSync, readFileSync, readdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const CLIENT = resolve(__dirname, '..');
const REPO = resolve(CLIENT, '../..');
const SRC = join(CLIENT, 'Source', 'Rok2');
const CONTENT = join(CLIENT, 'Content');

let passed = 0;
const failures = [];

function check(name, ok, detail = '') {
  if (ok) {
    passed += 1;
    return;
  }
  failures.push(detail ? `${name} (${detail})` : name);
}

/** يجرّد التعليقات: السطر أولاً ثم الكتل — العكس يفتح كتلة وهمية تمحو الكود. */
function code(rel) {
  const p = join(SRC, rel);
  if (!existsSync(p)) return '';
  return readFileSync(p, 'utf8')
    .replace(/\/\/[^\n]*/g, '')
    .replace(/\/\*[\s\S]*?\*\//g, '');
}

const raw = (rel) => (existsSync(join(SRC, rel)) ? readFileSync(join(SRC, rel), 'utf8') : '');

// ---------------------------------------------------------------------------
// T1 — لا لوح مبني ثم مخفي بلا مسار إظهار
// ---------------------------------------------------------------------------
check('Rok2CityWidget.cpp محذوفة', !existsSync(join(SRC, 'Private', 'Rok2CityWidget.cpp')));
check('Rok2CityWidget.h محذوفة', !existsSync(join(SRC, 'Public', 'Rok2CityWidget.h')));

{
  // العطل الأصلي: `X->SetVisibility(Collapsed)` **غير مشروط** على متغيّر محلي
  // داخل دالة البناء، فلا حقل يحمله ولا مسار يعيد إظهاره — الودجة تُبنى ثم
  // تُخفى أبداً. مستثنيان:
  //   • الحقول (this->) — إخفاؤها حالة يُعاد ضبطها من دالة تحديث.
  //   • الإخفاء المشروط (`if (!Texture) … else`) — سقوط صادق عند غياب أصل:
  //     لا نرسم مستطيلاً غريباً بدل الصورة.
  const widgetFiles = readdirSync(join(SRC, 'Private')).filter((f) => f.endsWith('Widget.cpp'));
  for (const file of widgetFiles) {
    const c = code(join('Private', file));
    const offenders = [];

    for (const m of c.matchAll(/\b(\w+)->SetVisibility\(ESlateVisibility::Collapsed\)/g)) {
      const name = m[1];
      // متغيّر محلي؟ (يُعلن بـ`UBorder* X =` في نفس الملف)
      if (!new RegExp(`\\bU\\w+\\*\\s+${name}\\s*=`).test(c)) continue;
      // له مسار إظهار؟
      if (new RegExp(`${name}->SetVisibility\\(ESlateVisibility::(Visible|HitTestInvisible)`).test(c)) continue;
      // مشروط بغياب أصل؟ نفحص السياق القريب قبل الإخفاء.
      const context = c.slice(Math.max(0, m.index - 200), m.index);
      if (/\belse\b\s*\{?\s*$|\bif\s*\([^)]*\)\s*\{?\s*$/.test(context)) continue;
      offenders.push(name);
    }

    check(`${file}: لا لوح محلي يُخفى بلا شرط ولا مسار إظهار`, offenders.length === 0,
      offenders.join(','));
  }
}

{
  const hud = code('Private/Rok2HudWidget.cpp');
  const hudH = code('Public/Rok2HudWidget.h');
  const gm = code('Private/Rok2GameMode.cpp');
  const roster = code('Private/Rok2AllianceRosterWidget.cpp');

  check('الـHUD يبثّ فعل التحصيل', hudH.includes('OnCollectAction') && hud.includes('OnCollectAction.Broadcast'));
  check('الـHUD يبثّ فعل التدريب', hudH.includes('OnTrainAction') && hud.includes('OnTrainAction.Broadcast'));
  check('GameMode يشترك على الفعلين',
    gm.includes('OnCollectAction.AddDynamic') && gm.includes('OnTrainAction.AddDynamic'));
  check('التحصيل يستدعي الأمر السلطوي', gm.includes('CollectCityProduction'));
  check('التدريب يفتح ورقة الوحدات الموجودة', gm.includes("HandleBuildingAction(TEXT(\"barracks\"), TEXT(\"train\"))"));
  check('تسريع الطابور له مستدعٍ في الـHUD',
    hud.includes('SpeedupQueue') && hud.includes('URok2HudQueueAction'));
  check('إنشاء التحالف في شاشة التحالف',
    roster.includes('CreateAlliance(Name, Tag)') && roster.includes('RefreshMembershipState'));
  check('قسم الإنشاء يُطوى لمن له تحالف', roster.includes('AllianceId.IsEmpty()'));

  // GameMode لم يبق يحمل مرجعاً لودجة محذوفة
  check('GameMode بلا مرجع لـ CityWidget', !gm.includes('CityWidget') && !code('Public/Rok2GameMode.h').includes('URok2CityWidget'));

  // أسماء الطوابير عربية لا معرّف خادمي خام
  check('أسماء الطوابير مترجمة من بيانات الخادم', hud.includes('QueueSubjectName'));
  check('لا `RefId Lv%d` خام في سطر الطابور', !/\*Q\.RefId, Q\.Level/.test(hud));

  // مرساة التدريب على عنصر مرئي في الـHUD
  check('مرساة FTUE للتدريب في الـHUD', hud.includes('Rok2FtueSpec::AnchorTrain'));
}

// ---------------------------------------------------------------------------
// T2 — قائمة الأيقونات المستوردة تطابق القرص
// ---------------------------------------------------------------------------
{
  const artCpp = code('Private/Rok2ArtAssets.cpp');
  const block = (/static const TSet<FString> ImportedIds = \{([\s\S]*?)\};/.exec(artCpp) || [, ''])[1];
  const declared = new Set([...block.matchAll(/TEXT\("([a-z_]+)"\)/g)].map((m) => m[1]));

  const iconDir = join(CONTENT, 'Art', 'UIIcons');
  const onDisk = new Set(
    existsSync(iconDir)
      ? readdirSync(iconDir).filter((f) => f.endsWith('.png')).map((f) => f.replace(/^icon_/, '').replace(/\.png$/, ''))
      : [],
  );

  check('قائمة ImportedIds غير فارغة', declared.size > 0);
  check(`عدد الأيقونات المستوردة ≥ 60 (${onDisk.size})`, onDisk.size >= 60);

  const declaredMissingOnDisk = [...declared].filter((id) => !onDisk.has(id));
  check('كل معرّف مُعلن له صورة على القرص', declaredMissingOnDisk.length === 0,
    declaredMissingOnDisk.slice(0, 5).join(','));

  // العكس ليس خطأً بالضرورة (صورة بلا معرّف في KnownIds)، لكن أي معرّف في
  // KnownIds له صورة ولا يُعلن = سقوط صامت إلى الراسم الإجرائي وهو العطل نفسه.
  const iconLib = code('Private/Rok2IconLibrary.cpp');
  const knownBlock = (/KnownIds = \{([\s\S]*?)\};/.exec(iconLib) || [, ''])[1];
  const known = [...knownBlock.matchAll(/TEXT\("([a-z_]+)"\)/g)].map((m) => m[1]);
  const silentFallback = known.filter((id) => onDisk.has(id) && !declared.has(id));
  check('لا معرّف له صورة ويسقط للراسم الإجرائي', silentFallback.length === 0,
    silentFallback.slice(0, 5).join(','));

  check('مولّد الأيقونات موجود', existsSync(join(REPO, 'scripts', 'generate_ui_icon_set.py')));
}

// ---------------------------------------------------------------------------
// T3 — أسطح نسيجية مع سقوط، وجلود أزرار مركزية
// ---------------------------------------------------------------------------
{
  const surf = code('Private/Rok2Surface.cpp');
  const surfH = code('Public/Rok2Surface.h');

  check('الأسطح تُحمّل نسيجاً من UISurfaces', surf.includes('/Game/Art/UISurfaces/%s.%s'));
  check('النسيج يُرسم 9-slice', surf.includes('ESlateBrushDrawType::Box') && surf.includes('Brush.Margin'));
  check('السقوط إلى اللون المسطّح قائم',
    /if \(!Texture\)\s*\{\s*return Fallback;/.test(surf) && surf.includes('FSlateRoundedBoxBrush'));

  for (const [fn, asset] of [['Panel', 'panel_parchment'], ['Sheet', 'panel_leather'],
    ['Card', 'card_stone'], ['TopBar', 'bar_wood'], ['Pill', 'pill_bronze']]) {
    check(`Rok2Surface::${fn} نسيجية (${asset})`,
      new RegExp(`Rok2Surface::${fn}\\([^)]*\\)\\s*\\{[\\s\\S]*?${asset}`).test(surf));
  }

  for (const skin of ['button_primary_gold', 'button_secondary_blue', 'button_danger_red', 'button_success_green']) {
    check(`جلد ${skin} مستهلك في مصنع الأسطح`, surf.includes(skin));
  }
  check('جلد الزر يُحمّل من UIButtons', surf.includes('/Game/Art/UIButtons/%s.%s'));
  check('الإطار المزخرف والفاصل معلنان',
    surfH.includes('OrnateFrame') && surfH.includes('GoldDivider'));

  // الأصول السبعة على القرص
  const surfDir = join(CONTENT, 'Art', 'UISurfaces');
  for (const asset of ['panel_parchment', 'panel_leather', 'card_stone', 'bar_wood',
    'pill_bronze', 'frame_ornate', 'divider_gold']) {
    check(`أصل السطح ${asset}.png موجود`, existsSync(join(surfDir, `${asset}.png`)));
  }
  check('مولّد الأسطح موجود', existsSync(join(REPO, 'scripts', 'generate_ui_surface_assets.py')));

  // رمز التعتيم بدل رقم مخترع في كل ودجة
  check('رمز ArtVeil معلن', code('Public/Rok2VisualTheme.h').includes('ArtVeil'));
}

// ---------------------------------------------------------------------------
// T4 — الأوجه من أصول FontFace بأوزان مسمّاة
// ---------------------------------------------------------------------------
{
  const typo = code('Private/Rok2Typography.cpp');
  const typoH = code('Public/Rok2Typography.h');

  check('الوجه يُبنى خطاً مركّباً', typo.includes('FStandaloneCompositeFont') && typo.includes('DefaultTypeface'));
  check('الأوزان الثلاثة تُطلب بالاسم',
    typo.includes('Rok2TypeWeight::Regular') && typo.includes('Rok2TypeWeight::Bold') && typo.includes('Rok2TypeWeight::Black'));
  check('أصول الأوجه تُحمّل من Fonts/Faces', typo.includes('/Game/Fonts/Faces/%s.%s'));
  check('التدهور اللطيف باقٍ', typo.includes('FCoreStyle::GetDefaultFontStyle'));
  check('أصول الأوجه محمية من الـGC', typoH.includes('TSet<UObject*>') && typoH.includes('FaceAssets'));
  check('وجه احتياط للمحارف خارج التغطية', typo.includes('FallbackTypeface'));

  const facesDir = join(CONTENT, 'Fonts', 'Faces');
  for (const face of ['ArefRuqaa-Regular', 'ArefRuqaa-Bold', 'Cairo-Regular',
    'Cairo-Bold', 'Cairo-Black', 'Cinzel-Regular', 'Cinzel-Bold']) {
    check(`ملف الوجه ${face}.ttf موجود`, existsSync(join(facesDir, `${face}.ttf`)));
  }
  check('سكربت استيراد الخطوط موجود', existsSync(join(CLIENT, 'import_fonts.py')));

  // الاستيراد يحتاج رمز المحرر لا commandlet — الوثيقة تسجّل السبب
  const importer = existsSync(join(CLIENT, 'import_fonts.py'))
    ? readFileSync(join(CLIENT, 'import_fonts.py'), 'utf8') : '';
  check('سكربت الخطوط يوثّق شرط ExecutePythonScript', importer.includes('ExecutePythonScript'));
}

// ---------------------------------------------------------------------------
// T5 — كل أصل مستورد له قارئ فعلي
// ---------------------------------------------------------------------------
{
  const art = code('Private/Rok2ArtAssets.cpp');
  const artH = code('Public/Rok2ArtAssets.h');
  const detail = code('Private/Rok2BuildingDetailWidget.cpp');
  const boot = code('Private/Rok2BootWidget.cpp');
  const story = code('Private/Rok2SeasonStoryWidget.cpp');
  const cmdr = code('Private/Rok2CommanderWidget.cpp');

  check('صور المباني لها قارئ',
    artH.includes('LoadCityBuildingPortrait') && art.includes('GetCityBuildingPortraitId'));
  check('بطاقة المبنى تعرض الصورة',
    detail.includes('LoadCityBuildingPortrait') && detail.includes('PortraitImage'));
  check('قاعة الحضارة عند مستوى متقدم', art.includes('civ_%s_hall_tier4'));
  check('Splash موصولة بشاشة الدخول', boot.includes('TEXT("Splash")') && boot.includes('splash_title'));
  check('لوحة فصل الموسم موصولة',
    story.includes('/Game/Art/SeasonStory/%s.%s') && story.includes('BackdropForEvents'));
  check('جلد القائد الأسطوري موصول',
    cmdr.includes('/Game/Art/CommanderSkins/%s.%s') && cmdr.includes('legendary'));

  // مجلدات الأصول التي أُضيفت لخط الاستيراد
  const importer = readFileSync(join(CLIENT, 'import_assets.py'), 'utf8');
  for (const dir of ['Art/UISurfaces', 'Art/HumanUnits', 'Art/KenneyCastleKit',
    'Art/Splash', 'Art/SeasonStory', 'Art/CommanderSkins']) {
    check(`${dir} داخل خط الاستيراد`, importer.includes(dir));
  }

  // الوحدات البشرية: `LoadHumanUnitMesh` كانت تعيد nullptr دائماً لغياب الأصول
  const unitDir = join(CONTENT, 'Art', 'HumanUnits');
  const unitAssets = existsSync(unitDir) ? readdirSync(unitDir).filter((f) => f.endsWith('.uasset')).length : 0;
  check(`وحدات بشرية مستوردة (${unitAssets})`, unitAssets >= 17);
}

// ---------------------------------------------------------------------------
// الوثيقة والخطة
// ---------------------------------------------------------------------------
{
  const docPath = join(REPO, 'game', 'docs', 'P24_UI_REVIVAL.md');
  check('وثيقة P24 موجودة', existsSync(docPath));
  if (existsSync(docPath)) {
    const doc = readFileSync(docPath, 'utf8');
    check('الوثيقة تسجّل التشخيص بالدليل', doc.includes('ESlateVisibility::Collapsed'));
    check('الوثيقة تسجّل ما لم يُوصَل', doc.includes('ما لم يُوصَل'));
    check('الوثيقة تحمل تحفّظ الشكل على الشاشة', doc.includes('لا حكم على الشكل النهائي'));
  }

  const plan = readFileSync(join(REPO, 'PLAN.md'), 'utf8');
  check('PLAN يحمل قسم P24', plan.includes('المرحلة P24'));
  check('PLAN يبقي بنود التحقق البصري مفتوحة',
    plan.includes('P24-T7') && plan.includes('P24-T8'));
}

// ---------------------------------------------------------------------------
console.log(`\nverify_p24_ui_revival: ${passed} PASSED, ${failures.length} FAILED`);
for (const f of failures) console.error(`  FAIL — ${f}`);
console.log('تحفّظ: تحقق بنيوي بلا مترجم C++ — ولا حكم على الشكل على الشاشة (P24-T7).');
if (failures.length) process.exit(1);
