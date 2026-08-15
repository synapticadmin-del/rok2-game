# P16-T1 — صيانة العرض: الكاميرا، الألوان، والواجهة

> **الشكوى الأصلية:** «زاوية الكاميرا مش كويسة في APK، القلعة والسور بلا ألوان،
> الواجهات سيئة». هذه الوثيقة تسجّل السبب الجذري لكل بند وما تغيّر فعلاً.

الحارس الآلي: `npm run test:p16-render-ui` (58 فحصاً) داخل `game/backend`،
ومضاف إلى سلسلة `npm run check`.

---

## 1. الألوان — القلعة والسور والمباني رمادية

### السبب الجذري

لم تكن المشكلة في الإضاءة ولا في إعدادات أندرويد. كل التلوين في اللعبة كان
يمرّ على هذا النمط:

```cpp
if (UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(0))
{
    Dyn->SetVectorParameterValue(TEXT("Color"), Theme.Primary);
}
```

`CreateAndSetMaterialInstanceDynamic` يبني النسخة الديناميكية **فوق المادة
الموجودة على الميش**. كل أشكال اللعبة placeholder مأخوذة من
`/Engine/BasicShapes`، فتحمل `WorldGridMaterial` (المكعب والمستوى) أو
`DefaultMaterial` (الأسطوانة والمخروط والكرة). وكلتا المادتين **لا تملك أي
`MaterialExpressionVectorParameter`** — تحقّقتُ من جدول التعبيرات داخل
الـ`.uasset` مباشرة. أي `SetVectorParameterValue` عليهما يُهمَل بصمت بلا
تحذير، فيبقى الجسم على شبكة WorldGrid أو رمادي مسطّح.

المشروع أيضاً لم يملك أي مادة `.uasset` من صنعه: 105 ملف `.uasset` في
`Content/` بلا مادة أساس واحدة. و`URok2ProceduralAssets::Init` كان يحاول
تحميل `/Engine/EngineMaterials/DefaultLitMaterial` — **وهذا الأصل غير موجود
في UE 5.4** (السجل يوثّق فشل التحميل)، فيسقط إلى `DefaultMaterial` بلا
بارامترات.

### ما تغيّر

1. **مادتا مشروع حقيقيتان** يولّدهما `create_materials.py` (headless، بلا
   واجهة رسومية):
   - `/Game/Art/Materials/M_Rok2Base` — مضاءة، بارامترات `Color` و
     `EmissiveColor` و `Roughness` و `Metallic`.
   - `/Game/Art/Materials/M_Rok2Unlit` — اللون على Emissive، تظهر بلا إضاءة.

   كلتاهما مُعلَّمة `used_with_instanced_static_meshes` — بدون هذا العلم يستبدل
   المحرك المادة بـ`DefaultMaterial` عند استخدامها مع ISM/HISM في بناء مُطبَّق،
   والسور والأرض والخريطة كلها HISM.

2. **`URok2ProceduralAssets::MakeTintedMaterialOn(Component, Element, Color)`**
   تفرض والداً نملك بارامترَه فعلاً بدل الاعتماد على مادة الميش. استُبدلت بها
   كل مواضع التلوين في `Rok2BuildingActor` و`Rok2HexWallActor` و
   `Rok2WorldRenderer` و`Rok2CityLayoutActor`.

3. **`TintExistingMaterialOn`** للأصول الفنية المستوردة: مجسمات KayKit تحمل
   مادة glTF بنسيج حقيقي، واستبدالها بلون مسطّح كان سيمحو التفاصيل. فنبحث عن
   بارامتر لون داخلها (`BaseColorFactor` هو اسم مستورد glTF) ونصبغ فيه، وإن لم
   تملك بارامتراً تُترك كما هي.

   ولأن البارامتر يُضرب في النسيج، يمرّ لون الحضارة على
   `SoftenTintForTexturedAsset`: إيماءة 30% نحو لون الحضارة مع حدّ أدنى للسطوع
   0.72. بدونها كان أساس اليابان `#111111` يطفئ المجسم كله إلى أسود.

4. **محو التجاوزات عند تبديل الميش**: تجاوزات المواد لا تُمحى مع
   `SetStaticMesh`. فالمبنى يمرّ أولاً بمسار placeholder ويحمل لوناً مسطّحاً،
   ثم يأتي الأصل الفني فتبقى المادة القديمة فوقه. أُضيف
   `EmptyOverrideMaterials()` في `MarkUsingArtAsset` وفي `SpawnMarkerActor`
   (الأخير يعيد استخدام ممثلين من مسبح الأداء).

5. **أرضية المدينة**: `GroundMaterial` خاصية بلا قيمة افتراضية ولم تُضبط قط،
   فكان `if (GroundMaterial)` كاذباً دائماً وتبقى الأسطوانات على مادتها
   الافتراضية. الآن تُلوَّن الأرضية وخلايا الإبراز عبر مادة المشروع.

6. **مؤشر حالة المبنى** كان يفرّق الحالات بالشكل فقط. أضفتُ لوناً لكل حالة
   (أزرق للبناء، ذهبي للجمع، أخضر للتدريب، أحمر للجرحى) — الشكل واللون معاً.

7. **متانة السور مرئية**: `ApplyTierMaterials` كانت تكتب وسماً فقط بتعليق
   «تلوين فعلي يحتاج مادة بـ Parameter». صار لون السور يميل إلى الأحمر الداكن
   بنسبة الضرر، فيرى اللاعب حالة سوره قبل فتح أي لوحة.

### التحقق

`UnrealEditor-Cmd -run=pythonscript` يزرع `Rok2BuildingActor` و
`Rok2HexWallActor` ويقرأ المادة الفعلية على كل مكوّن:

```
bld:Mesh         mat=MID_M_Rok2Base_0  parent=M_Rok2Base  Color=(0.006,0.006,0.006)
wall:WallSegments mat=MID_M_Rok2Base_0  parent=M_Rok2Base  Color=(0.167,0.054,0.042)  ← أثر الضرر
art_body         mat=MID_hexagons_medieval1_1  parent=hexagons_medieval1  BaseColorFactor=(0.72,0.72,0.72)
```

قبل الإصلاح كانت كلها `parent=WorldGridMaterial` أو `NO MATERIAL`.

---

## 2. زاوية الكاميرا في APK

### السبب الجذري

`FieldOfView` في UE **أفقي دائماً**، ويُفسَّر على النسبة المخزّنة في
`UCameraComponent::AspectRatio` (افتراضها 1.7778). القيد الافتراضي في
`BaseEngine.ini` هو `AspectRatio_MaintainYFOV`، لكن `UCameraComponent` يحمل
افتراضاً مختلفاً (`MaintainXFOV`) ولا يُطبَّق إلا مع
`bOverrideAspectRatioAxisConstraint`. لم يكن أيٌّ من الاثنين مضبوطاً في
المشروع، فأي اختلاف في نسبة الشاشة بين نافذة المحرر 16:9 وهاتف 19.5:9 أو 21:9
يغيّر عمق المشهد المرئي — وهذا ما يُقرأ كـ«زاوية سيئة».

`Config/DefaultEngine.ini` أيضاً لم يحتوِ قسم `[/Script/Engine.LocalPlayer]`.

### ما تغيّر

`ARok2IsometricCamera::ApplyProjectionSettings()` (تُنادى في المُنشئ وفي
`BeginPlay`) تضبط:

- `AspectRatio = ReferenceAspectRatio` (16:9) — النسبة التي ضُبطت عليها زاوية
  الميل `-42°` والمسافة.
- `bOverrideAspectRatioAxisConstraint = true` + `MaintainYFOV`.
- `bConstrainAspectRatio = false` — ملء الشاشة بلا أشرطة سوداء.

مع هذين، يحسب المحرك (`CameraStackTypes.cpp:281`) المجال الرأسي
`atan(tan(FOV/2) / AspectRatio)` ويثبّته، ويمدّ الأفقي بعرض النافذة الفعلي.
النتيجة: نفس العمق الرأسي على كل جهاز، والشاشة الأعرض تكسب مدى جانبياً فقط.

القيد نفسه مُثبَّت في `[/Script/Engine.LocalPlayer]` كي لا يتسرّب اختلاف من
ملف تهيئة آخر.

**ملاحظة على محاولة أولى مرفوضة:** جرّبتُ حساب FOV يدوياً من نسبة النافذة كل
إطار. هذا خطأ مزدوج — المحرك يفعله أصلاً مع MaintainYFOV، فالتصحيح الإضافي
يضاعف الأثر ويوسّع المجال الرأسي على الشاشات العريضة، أي عين المشكلة.

### ما لم يُتحقَّق منه هنا

الشاشة الفعلية على جهاز أندرويد. الرياضيات مؤكَّدة من كود المحرك والبناء ينجح،
لكن القياس البصري يحتاج APK على جهاز مستهدف.

---

## 3. الواجهة

### 3.1 سلم أحجام النص كان معطلاً بالكامل

`Rok2Typography.cpp` — `SizeOf()` كانت بلا `break` في أي حالة:

```cpp
case ERok2TextRole::Display:  Raw = Rok2TypeScale::Display;   // ← لا break
case ERok2TextRole::Title:    Raw = Rok2TypeScale::Title;
...
default:                      Raw = Rok2TypeScale::Body;
```

كل حالة تسقط إلى التالية حتى `default`، فكانت **كل** الأدوار الاثنا عشر تعود
بحجم `Body` = 15px. العنوان الكبير (24) والحاشية الصغرى (11) بنفس الحجم. سلم
الأحجام في الهيدر و`static_assert` والحارس النصّي كلها كانت خضراء والسلوك
مسطّح. أُضيف `break` لكل حالة، والحارس الجديد يفحص السلوك لا وجود الأسماء.

### 3.2 أصول الواجهة المؤلَّفة لم تكن موجودة للعبة

`import_assets.py` كان يستورد `kaykit` و`Commanders` و`WorldMapIcons` و
`Audio` فقط. أي أن **155 ملف PNG** في `UIIcons` و`UIButtons` و
`CityBuildingIcons` و`CivIcons` و`CivBackgrounds` و`Tavern` بقيت صوراً على
القرص بلا `.uasset` — غير موجودة للمحرك أصلاً. النتيجة:

- `GetIconBrush` يفشل في `LoadObject` ويسقط إلى الراسم الإجرائي 32×32، فكل
  أيقونة في اللعبة رسم متجهي مكتوب بالكود لا الأصل المؤلَّف.
- `ApplyButtonSkin` يعود بلا جلد فتظهر أزرار Slate الرمادية الافتراضية.

أُضيفت المجلدات الستة إلى `JOBS`، واستُوردت كلها (155 أصلاً)، وأُضيفت
مجلداتها إلى `DirectoriesToAlwaysCook`.

### 3.3 مقياس الواجهة كان مضاعفاً على الحاسوب ومعدوماً على الهاتف

`URok2Accessibility` كان يبذر `UiScale` من
`FPlatformApplicationMisc::GetDPIScaleFactorAtPoint`. خطأ من طرفين:

- المحرك يطبّق مقياس DPI أصلاً عبر `UIScaleRule`/`UIScaleCurve`، فكان ضرب
  الأحجام مرة ثانية يكبّرها على شاشات ويندوز 125%/150% بلا داعٍ.
- على أندرويد تعيد الدالة `1.0` دائماً (تنفيذ GenericPlatform)، فلم تكن تفعل
  شيئاً حيث تلزم.

الآن `UiScale = 1.0` كتفضيل للاعب، وأُضيف قسم
`[/Script/Engine.UserInterfaceSettings]` — كان غائباً تماماً فورث المشروع منحنى
المحرك حيث يحصل هاتف 720p على مقياس 0.666 فتصغر كل الأحجام المطلقة. المنحنى
الجديد يثبّت 1.0 من 720 ارتفاعاً وفوق.

### 3.4 لا حواف آمنة للنتوء

أُضيفت `URok2Accessibility::GetSafeAreaPadding()` من `FDisplayMetrics`، مع حدّ
أدنى 16px على الجانبين في الهاتف (الزوايا المنحنية تقص أي عنصر ملتصق بالحد).
مطبَّقة على شريط الموارد ومجموعة أزرار العمل والمجموعة اليسرى.

### 3.5 نظافة الملفات

سكربتات التصحيح الآلية (`fix_all_typography_and_hud.js`) استعملت
`String.replace` بأنماط تتطابق في مواضع متعددة، فأنتجت **76 سطراً مكرراً
متتالياً** (48 في `Rok2CityWidget`، 27 في `Rok2HudWidget`) مثل تسع نداءات
متتالية لـ`SetVisibility(Collapsed)`. حُذفت، ويمنع الحارس عودتها.

---

## 4. التحزيم — لماذا لم يكن أي من هذا يظهر في APK

`Config/DefaultGame.ini` **لم يكن موجوداً**، وقسم
`[/Script/UnrealEd.ProjectPackagingSettings]` كان في `DefaultEngine.ini`.
لكن `UProjectPackagingSettings` معلَّنة `UCLASS(config=Game)` فتقرأ
`DefaultGame.ini` فقط — أي أن `UsePakFile` و`bUseIoStore` و`MapsToCook`
**لم تُقرأ يوماً**، والكوك كان يعتمد كلياً على `-cookall` في
`Build-Rok2.ps1`. أي سكربت لا يمرّره (`BuildAndroid.bat`) ينتج APK ناقص الأصول.

أُنشئ `Config/DefaultGame.ini` بالقسم في مكانه الصحيح، مع
`DirectoriesToAlwaysCook` لكل مجلد يُحمَّل بالمسار النصّي في وقت التشغيل
(`LoadObject` لا يُرى كمرجع صريح للكوك) — بما فيها `/Game/Art/Materials` و
`/Engine/BasicShapes`.

### حاجز `WITH_EDITOR`

`URok2ArtAssets::LoadMesh` كان يحمّل الميش داخل `#if WITH_EDITOR`، فيعيد
`nullptr` **دائماً** في بناء مُطبَّق ويبقى العميل على مكعبات placeholder أبد
الدهر حتى لو كانت الأصول محزَّمة. الحاجز أُزيل.

### تسمية مجسمات KayKit

ملفات KayKit متعددة العقد لا يُسمّي المستورد ميشها الرئيسي باسم الملف:
`building_windmill.glb` ينتج `building_windmill_blue`، و`building_tower_A`
ينتج `building_tower_A_blue`. فأصبح `MeshPackageCandidates` يبحث في قائمة
مرتبة (الاسم المسطّح، شجرة `StaticMeshes/`، ثم لواحق الألوان). التحقق: كل
معرّفات الكتالوج الثمانية عشر تُحلّ الآن إلى ميش حقيقي (`MISSING_COUNT=0`).

### إعادة استيراد أصول بصيغة أحدث من المحرك

102 أصلاً (`kaykit` و`Commanders` و`Audio`) كانت بنسخة حزمة `-9` أي UE 5.5+،
والمحرك المعتمد 5.4.4 يرفضها بـ`Package is too old. Min Version: 214`.
مصدرها `build_android.ps1` الذي يشير إلى `UE_5.8`. حُذفت وأُعيد استيرادها
بـ5.4.4. الأصول الخام (`.glb`/`.wav`/`.png`) هي المُلتزمة في git —
`.gitignore` يستثني `*.uasset` — فإعادة الاستيراد بلا خسارة.

### تفعيل Interchange

`DisableEnginePluginsByDefault: true` كان يعطّل إضافة **Interchange**، وهي
مستورد glTF في UE 5.4. أي محاولة استيراد `.glb` كانت تُسقط المحرك بـ
`Assertion failed: IsValid()` داخل `UnrealEditor-AssetTools`. أُضيفت مع
`TargetAllowList: ["Editor"]` وفي `DisabledPlugins`، فتعمل في المحرر ولا
تدخل بناء اللعبة أو الأندرويد.

كذلك أُصلح المستورد نفسه: كان يستورد دفعة واحدة فيضيع كل شيء عند أول تعثر،
وكان فحص «هل استُورد؟» يسأل عن مسار مسطّح لا ينتجه Interchange فيعيد استيراد
كل شيء في كل تشغيل. الآن ملف-ملف مع التقاط الاستثناء، وفحص يعرف شجرة glTF،
و`ROK2_JOB` يقصر التشغيل على مجلد واحد لأن استيراد كل شيء في جلسة واحدة
يُسقط المحرك بعد عدة مئات من الملفات.

---

## 5. سلطة الخادم

`Rok2WorldRenderer::RefreshFromApi` كانت تضيف 6 عُقد وممرين **وهميين** عند
فراغ اللقطة («Local preview entities for immediate map vibrancy»). هذا يخالف
`AGENTS.md §3` ويُنتج أهدافاً لا وجود لها على الخادم — يضغطها اللاعب فلا يحدث
شيء. أُزيلت؛ لقطة فارغة تعني «لم تصل بعد» والصواب أن تبقى الخريطة فارغة.

---

## 6. ما بقي غير متحقَّق

- **قياس بصري على جهاز أندرويد**: لا أدوات أندرويد مثبَّتة في هذه الجلسة.
  دورة `ImportAssets.bat` ← `BuildAndroid.bat` ← تشغيل على جهاز مستهدف، مع
  تسجيل `stat unit` و`stat memory`، شرطٌ لازم قبل تعليم أي بند APK.
- **دورة PIE كاملة**: بُنِي الهدفان (`Rok2Editor` و`Rok2`) بنجاح على UE 5.4.4،
  وشُغّل Standalone وثبت تحميل `Rok2Main` بلا بصمة خطأ قاتل. لكن
  `/v1/auth/guest` يعود 500 من الخادم الحيّ (`AUTH_SECRET` غير مضبوط)، فلم
  تُختبر واجهة ما بعد الدخول ضد بيانات حقيقية.
- **الخطوط**: `Content/Fonts/` فيه `README.md` فقط. النص العربي يُرسم على
  Roboto المحرك، وهو بلا محارف عربية. استيراد الخطوط الموثقة في ذلك الملف
  (Cairo / Noto Kufi، OFL) عمل مستقل لم يُنفَّذ هنا.
- **توحيد رموز التصميم**: لا تزال 130 قيمة `FLinearColor` مكتوبة داخل
  الودجات، وست لوحات ألوان متوازية، ولا نظام `spacing`/`radius`/`FSlateButtonStyle`
  مشترك. `Rok2Visual` مستعملة في ملفَي ودجات فقط. هذا إعادة هيكلة واجهة كاملة
  لا صيانة.
