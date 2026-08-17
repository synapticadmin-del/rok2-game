# WorldFeatures — رسوم معالم الخريطة المقطّعة (P24-T6)

sprites مفردة مقطّعة من صفائح `Content/Art/WorldMapIcons/T_world_*.png` بواسطة
`scripts/slice_world_feature_sprites.py`.

## لماذا مقطّعة

الأصول في `WorldMapIcons` باسم `T_world_*` **ليست sprites بل صفائح**:

| الصفيحة | ما تحمله فعلاً |
|---|---|
| `T_world_resource_nodes_quad` | أربع عقد موارد في شبكة 2×2، **وتحت كل واحدة نص عربي مطبوع داخل الصورة** («حقل قمح»، «معسكر أخشاب»، «مقلع حجارة»، «منجم ذهب») |
| `T_world_stone_gold_quarry_mine` | منجم كبير + برج صغير مقصوص على الحافة اليسرى |
| `T_world_holy_shrine_altar` | معبد + مسلّة مقصوصة |
| `T_world_mountain_pass_fortress` | حصن ممر + قصاصة جبل |
| `T_world_barbarian_fort_camp` | معسكر + شريحة سياج |

فرسمها كما هي على الخريطة يعني عقدة قمح تحمل ثلاث عقد أخرى ونصاً عربياً
معكوساً (البِلبورد يواجه الكاميرا، والنص مرسوم في البكسل).

## الجدول

| المعرّف | ما يمثّله | يُختار متى |
|---|---|---|
| `farm_field` | حقل قمح | عقدة `food`/`wheat` |
| `lumber_camp` | معسكر أخشاب | عقدة `wood` |
| `stone_quarry` | مقلع حجارة | عقدة `stone` |
| `gold_mine` | منجم ذهب | عقدة `gold` مستوى ≤ 3 |
| `gold_mine_large` | منجم ذهب كبير | عقدة `gold` مستوى ≥ 4 |
| `barbarian_camp` | معسكر برابرة | عقدة `barb` مستوى ≤ 3 |
| `barbarian_keep` | حصن برابرة | عقدة `barb` مستوى ≥ 4 |
| `pass_fortress` | حصن الممر | ممر (`pass`) |
| `throne_temple` | معبد العرش | العرش (`throne`) |
| `holy_shrine` | معبد مقدّس | متاح — لم يوصل بعد (لا مواقع مقدسة في لقطة العميل) |
| `mountain_ridge` | سلسلة جبال | متاح — لم يوصل بعد (زخرفة تضاريس) |

كل معرّف بثلاث خرائط: `_D` ألبيدو، `_N` عمق، `_E` إضاءة. الأسماء الكاملة
`T_feat_<id>_<D|N|E>.png`.

## إعادة التوليد

```bash
python scripts/slice_world_feature_sprites.py --dry-run   # تقرير بلا كتابة
python scripts/slice_world_feature_sprites.py             # يكتب 33 ملفاً
```

ثم الاستيراد إلى `/Game/Art/WorldFeatures` (يشغّل التقطيع بنفسه أولاً):

```powershell
.\scripts\Import-WorldFeatureSprites.ps1 -EngineRoot $env:UE_ROOT -ReplaceExisting
```

`sprites.json` يسجّل إحداثيات كل قصّ ومعايير التقطيع، فالمراجعة لا تحتاج إعادة
تشغيل.

## ملاحظات

- المستهلك: `URok2ArtAssets::LoadWorldFeatureTexture` (كانت بلا مستدعٍ قبل
  P24-T6) عبر `WorldFeatureIdForNode`، ويرسمها `ARok2WorldRenderer` على
  `UBillboardComponent`.
- ثلاث طبقات احتياط: الرسم المقطّع ← أيقونة `icon_node_*` المسطّحة ← الشكل
  الهندسي. غياب الحزمة لا يُعطّل الخريطة.
- الحجم يُضبط بـ`WorldFeatureSpriteScale` على الراسم (افتراضي 1.8): الرسم
  المفصّل يحتاج مساحة أوسع من الرمز المسطّح. القيمة النهائية تُقرّر بصرياً في
  قبول PIE (P24-T7).
- الأصول مشتقّة من صفائح المشروع نفسها، فترخيصها ترخيصها.
