# P7-T10: الأصول البصرية المتبقية على خريطة العالم — حزمة WorldMapIcons

**التاريخ:** 2026-08-12 · **الحالة:** منفّذ على الكود + استيراد جاهز للـ UE 5.4.4 · WIP حتى دورة بناء/PIE موثقة

## الدافع

كانت `ARok2WorldRenderer` ترسم عُقد الموارد والممرات والعرش ومنشآت التحالف ككرات
وأسطوانات ومواد ملونة، والمسيرات كمخروط هندسي (Cone) بلون التحالف. بند P7-T10 يستبدل
هذه الأشكال الإجرائية بـ 14 أيقونة PNG أصلية شفافة بأسلوب stylized low-poly medieval
متوافق بصريًا مع KayKit ووثيقة `07-game-design/civilizations-visual-design.md`.

## محتوى الحزمة `Content/Art/WorldMapIcons/`

| الملف | الاستخدام | معرف الاستهلاك |
|---|---|---|
| `icon_node_wheat.png` | عقدة طعام | `node_wheat` |
| `icon_node_wood.png` | عقدة خشب | `node_wood` |
| `icon_node_stone.png` | عقدة حجر | `node_stone` |
| `icon_node_gold.png` | عقدة ذهب | `node_gold` |
| `icon_node_barbarian.png` | أي عقدة برابرة | `node_barbarian` |
| `icon_node_resource_generic.png` | مورد غير مصنف/marker عام | `node_resource`، `world_marker` |
| `icon_objective_throne_crown.png` | العرش/التاج | `objective_throne_crown` |
| `icon_objective_pass_gate.png` | ممر/بوابة | `objective_pass_gate` |
| `icon_alliance_bastion.png` | برج تحالف | `alliance_bastion` |
| `icon_alliance_catapult.png` | منجنيق تحالف | `alliance_catapult` |
| `icon_march_infantry.png` | مسيرتها الأبرز مشاة | `march_infantry` |
| `icon_march_cavalry.png` | مسيرتها الأبرز فرسان | `march_cavalry` |
| `icon_march_archer.png` | مسيرتها الأبرز رماة | `march_archer` |
| `icon_march_siege.png` | مسيرتها الأبرز حصار | `march_siege` |

كل صورة 512×512 بخلفية شفافة حقيقية (alpha channel)، أقل من 340KB. الأيقونات
مصممة باتجاه +X وتُعرض على `UBillboardComponent` فتواجه الكاميرا دائمًا.

## نقاط الدمج في الكود

| الملف | التغيير |
|---|---|
| `Source/Rok2/Public/Rok2ArtAssets.h` | `GetWorldMapIconPath(IconId)` + `LoadWorldMapIcon(IconId)` (تحميل مخبأ يُرجع `nullptr` عند الغياب) |
| `Source/Rok2/Public/Rok2Types.h` | `FRok2MarchEntity.Branch` (الفرع الأبرز من `Troops`) |
| `Source/Rok2/Private/Rok2Api.cpp` | `ParseMarchEntity` يستخرج الفرع حسب العدد الأكبر بين infantry/cavalry/archer/siege |
| `Source/Rok2/Public/Rok2WorldRenderer.h` | `SpawnSpriteActor(Texture, Loc, Label, WorldScale)` |
| `Source/Rok2/Private/Rok2WorldRenderer.cpp` | عُقد + ممرات/عرش + منشآت تحالف + مسيرات تستخدم `LoadWorldMapIcon` مع fallback كامل إلى الـ mesh الهندسي السابق |
| `import_assets.py` | Job جديد `Art/WorldMapIcons -> /Game/Art/WorldMapIcons` |
| `scripts/Import-WorldMapIcons.ps1` | سكربت استيراد UE 5.4.4 جديد (نمط `Import-CityMapUIAssets.ps1`) |

التعيين من معرفات `URok2WorldIconography::Resolve` إلى ملفات الحزمة يتم في
`RefreshFromApi` (بارباريان → `node_barbarian`، `node_resource`/`world_marker` →
`node_resource_generic`، `throne`/`pass` حسب `IconId`).

## فallback والصلابة

- `LoadWorldMapIcon` يُرجع `nullptr` عند غياب الملف أو تعذر التحميل، ويُتخطى
  `SpawnSpriteActor` تلقائيًا إلى مسار `SpawnMarkerActor` السابق — لا تغيير سلوكي
  في البيئات التي لم تُستورد الحزمة فيها، ولا تأثير على البناء (لا API فوق 5.4.4).
- أيقونات المسيرات لا تحمل تلوين التحالف؛ اللون يبقى على الـ billboard غير مُلوَّن
  (Sprite) بينما يحتفظ المخروط السابق باللون — مقصود: التمييز البصري الآن بالفرع
  لا بالعمود اللوني، والحدود تُستكمل في T1–T6 عند الحاجة.

## التحقق

- معاينة لوحة مجمعة `world_icons_sheet.png` (خارج المستودع) تحققت فيها الشفافية
  والوضوح. لا يوجد UE على بيئة التنفيذ — البناء على UE 5.4.4 واختبار PIE مطلوبان
  قبل إغلاق البند في PLAN نهائيًا (بروتوكول AGENTS: لا يُغلق البند حتى تمر دورة
  بناء وPIE على الإصدار المعتمد).

## الاستيراد

```powershell
.\scripts\Import-WorldMapIcons.ps1 -EngineRoot $env:UE_ROOT
```
أو ضمن `import_assets.py` داخل `ImportAssets.bat`.
