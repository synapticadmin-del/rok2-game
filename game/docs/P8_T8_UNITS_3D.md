# P8-T8: الوحدات البشرية 3D (Human Units 3D)

**الحالة:** مكتمل — تحقق بنيوي (لا PIE). القبول النهائي على Unreal Engine 5.4.4 / Windows بعد استيراد الأصول عبر `Import-HumanUnits.ps1`.

هذا البند ينقل تمثيل المسيرات على خريطة العالم من المخروط الهندسي الافتراضي إلى **موديلات وحدات بشرية 3D** متدرجة: 5 مراحل تدريب × 4 فروع قتالية (مشاة/رماة/فرسان/حصار) = 20 شبكة عرض، إضافة إلى 6 وحدات خاصة حضارية تُفتح عند المرحلة الرابعة.

## الخريطة الكاملة لشبكات الوحدات

| الفرع | T1 | T2 | T3 | T4 | T5 |
|-------|----|----|----|----|----|
| مشاة infantry | Swordsman | Spearman | Long Swordsman | Royal Guard | Imperial Guard |
| رماة archer | Bowman | Composite Bowman | Crossbowman | Royal Crossbowman | Imperial Crossbowman |
| فرسان cavalry | Light Cavalry | Heavy Cavalry | Knight | Royal Knight | Imperial Knight |
| حصار siege | Arcuballista | Mangonel | Ballista | Trebuchet | Catapult |

المصادر الفنية موزعة على مستودعين موثوقين بترخيص مفتوح. المشاة والرماة والفرسان (15 شبكة) مولّدة إجرائيًا low-poly في `game/client-unreal/Content/Art/HumanUnits/` بأسلوب متسق مع KayKit (أحجام سنتيمتر UE، تدرج لوني من أخضر التجنيد t1 إلى ذهبي إمبراطوري t5). حصار المرحلتين الأولى والثانية (Arcuballista/Mangonel) مولّد أيضًا في نفس المجلد، بينما حصار T3–T5 يعاد استخدام موديلات Kenney Castle Kit الموجودة أصلًا في `Content/Art/KenneyCastleKit/` (ترخيص CC0 — انظر `game/client-unreal/Content/Art/KenneyCastleKit/LICENSE.txt`).

| الشبكة | الملف | المجلد | المقياس |
|--------|-------|--------|---------|
| infantry_t1..t5 | infantry_t{1..5}.glb | Content/Art/HumanUnits | 1.75 |
| archer_t1..t5 | archer_t{1..5}.glb | Content/Art/HumanUnits | 1.75 |
| cavalry_t1..t5 | cavalry_t{1..5}.glb | Content/Art/HumanUnits | 1.90 (حصان) |
| siege_t1 | siege_arcuballista.glb | Content/Art/HumanUnits | 1.00 |
| siege_t2 | siege_mangonel.glb | Content/Art/HumanUnits | 1.00 |
| siege_t3..t5 | siege-ballista/trebuchet/catapult.glb | Content/Art/KenneyCastleKit | 1.00 |

## الوحدات الخاصة الحضارية

الوحدات الخاصة الست من `game/backend/src/data/civilizations.json` (unlock_tier = 4) ترث حاليًا شبكة فرعها عند المرحلة الرابعة حتى توفر جلود مخصصة (textures/جلود حضارية) عند الاستيراد النهائي على Windows. هذا القرار موثق لأن لقطة العالم السلطوية لا تحمل هوية حضارة مالك المسيرة، والموديلات الإجرائية المؤقتة لا تميز بين وحدات الحضارات.

| الحضارة | الوحدة الخاصة | الفرع | الشبكة الموروثة |
|---------|--------------|-------|------------------|
| Rome | Legionary | مشاة | infantry_t4 |
| Egypt | Khopesh Guard | مشاة | infantry_t4 |
| Vikings | Huskarl | مشاة | infantry_t4 |
| Japan | Samurai | مشاة | infantry_t4 |
| China | Chu-Ko-Nu | رماة | archer_t4 |
| Arabia | Desert Rider | فرسان | cavalry_t4 |

## التنفيذ C++

**`URok2ArtAssets` (Rok2ArtAssets.h/cpp)** أُضيف إليها نظام كتالوج ثانٍ للوحدات البشرية يحاكي كتالوج المباني القائم:

| الدالة | الدور |
|--------|-------|
| `GetHumanUnitId(Branch, Tier, CivId)` | اسم الشبكة من الفرع والمرحلة؛ يستبدل الوحدة الخاصة عند Tier ≥ 4 ومعرّف حضارة مطابق |
| `GetHumanUnitAssetPath(UnitId)` | مسار أصل المحرر `/Game/Art/{Folder}/{File}.{File}` |
| `LoadHumanUnitMesh(UnitId)` | تحميل UStaticMesh مع تخزين مؤقت؛ nullptr عند عدم الاستيراد (fallback) |
| `HasHumanUnit(UnitId)` | تحقق معرف |
| `BuildHumanUnitCatalog()` | بناء كتالوج 26 وحدة مرة واحدة (15 بشرية + 5 حصار + 6 خاصة) |

`FRok2ArtEntry` اكتسب حقل `Folder` ليدعم مجلدات متعددة (kaykit/HumanUnits/KenneyCastleKit)، و`LoadHumanUnitMesh` يقرأ المسار من الكتالوج بدل المسار الثابت `/Game/Art/kaykit/`.

**`ARok2WorldRenderer` (Rok2WorldRenderer.h/cpp):** مسار عرض المسيرات أصبح ثلاثي التدرج:

1. **P8-T8:** `DeriveMarchTier(M.Troops)` يقرأ أقصى مرحلة من أسماء المفاتيح `branch_tN` داخل خريطة Troops، ثم `GetHumanUnitId` + `LoadHumanUnitMesh` — عند نجاح التحميل تُرسم المسيرة بموديل الوحدة الملون بلون التحالف.
2. **P7-T10 (fallback):** أيقونة sprite `march_{branch}` من حزمة WorldMapIcons.
3. **الافتراضي:** مخروط Engine BasicShapes ملون.

`DeriveMarchTier` ثابتة بلا اعتماد على بيانات خادم جديدة — تستخدم خريطة Troops الموجودة أصلًا في snapshot (مفاتيح مثل `infantry_t3=4000`)، وتتجاهل المفاتيح غير المرتبة tier.

## التوليد والاستيراد

```bash
# توليد 17 GLB إجرائيًا (لا مكتبات خارجية — GLB 2.0 خام)
python3 scripts/generate_human_units_glb.py
```

```powershell
# الاستيراد في Unreal 5.4 (Windows)
.\game\client-unreal\scripts\Import-HumanUnits.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
```

السكربت يتحقق من اكتمال الحزمة (17 ملفًا) ثم يستدعي `UnrealEditor-Cmd -run=ImportAssets` على `StaticMeshFactory` إلى `/Game/Art/HumanUnits/`. بعد الاستيراد يحمّل `LoadHumanUnitMesh` الأصول مباشرة من حزمة المحرر.

## التحفظات والقبول النهائي

التحقق في هذه البيئة بنيوي فقط (سكربت الحارس `game/client-unreal/scripts/verify_p8_human_units.mjs` — 143 فحصًا)؛ لا تتوفر أدوات Unreal للفحص داخل المحرر (PIE). عند القبول النهائي على Windows يتحقق: استيراد الموديلات دون أخطاء glTF، ظهور موديل الوحدة الأبرز على المسيرات بدل المخروط عند جميع المراحل، وألوان التحالف مصبوغة على المواد، ثم استبدال الموديلات الإجرائية بالأصول الفنية النهائية في `game/docs/07-game-design/assets/` عند توفرها.

## الترخيص

الموديلات الإجرائية كود مشروع داخلي (لا ترخيص خارجي). موديلات Kenney Castle Kit تحت ترخيص CC0. الأصول الفنية النهائية المستوردة لاحقًا يجب أن تكون CC0 أو مرخصة تجاريًا وفق متطلبات P7-T9.

## حارس الجودة

```bash
node game/client-unreal/scripts/verify_p8_human_units.mjs   # من game/backend: npm run test:p8-t8-human-units
```
