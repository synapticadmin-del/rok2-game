# Art Assets — ROK2 (P2-T7)

أصول فنية أولية CC0 من **KayKit Medieval Hexagon Pack 1.0**
(<https://github.com/KayKit-Game-Assets/KayKit-Medieval-Hexagon-Pack-1.0>).
الترخيص الكامل في `LICENSE.txt` (CC0 1.0 — لا يتطلب نسبة).

## التحويل

الحزمة الأصلية تأتي بصيغة `.gltf + .bin + .png` مشتركة. حُوّلت الموديلات
المختارة إلى **GLB مضغوط ذاتي الاكتفاء** (الهندسة + الأطلس مضمّنة) عبر
سكربت التحويل في جلسة P2-T7 — كل ملف هنا صالح كـ glTF 2.0 Binary.

## الفهرس (URok2ArtAssets)

| Id (لعبة) | الملف | الاستخدام |
|---|---|---|
| city_hall | building_castle.glb | قاعة المدينة / قلعة زرقاء |
| city_enemy | building_castle_red.glb | مدن الأعداء على الخريطة |
| farm | building_windmill.glb | مزرعة |
| lumber_mill | building_lumbermill.glb | منشرة |
| quarry / goldmine | building_mine.glb | محجر / منجم |
| barracks | building_barracks.glb | ثكنة |
| stable | building_tavern.glb | إسطبل |
| archery_range | building_archeryrange.glb | ميدان رماية |
| hospital | building_market.glb | مستشفى |
| wall | building_tower_A.glb | سور/برج |
| storehouse | building_blacksmith.glb | مخزن |
| flag_blue/red/green/yellow | prop_flag_*.glb | أعلام تحالف |
| mountain / hills | nature_*.glb | مرتفعات حدودية |

## كيف تُستخدم

- `URok2ArtAssets::LoadMesh(Id)` يحاول تحميل `UStaticMesh` من الأصل المستورد
  (في المحرر بعد الاستيراد)، ويعيد `nullptr` إن لم يوجد — فيبقى الراسم على
  الأشكال الهندسية الافتراضية (لا يُكسر البناء بدون الاستيراد).
- للاستيراد في المحرر: شغّل `setup_level.py` (يستورد كل GLB إلى
  `/Game/Art/kaykit` كـ uasset) أو اسحب الملفات إلى Content Browser.

## ملاحظة تخزين (base64)

ملفات `.glb` على الفرع مخزّنة حالياً كنص base64 (يفك ترميزها إلى GLB صالح).
`setup_level.py` يفك ترميزها تلقائياً إلى binary قبل الاستيراد (خطوة
`_ensure_binary_glb`)، فلا يُكسر البناء. لإعادة توليد binary محلياً دفعةً واحدة:

```python
import base64, glob
for p in glob.glob("Content/Art/kaykit/*.glb"):
    raw = open(p, "rb").read()
    if raw[:4] != b"glTF":                       # نص base64؟
        open(p, "wb").write(base64.b64decode(raw))
```

## الوحدات (units)

ملفات الوحدات البشرية غير مضمّنة في النسخة المجانية من الحزمة (تأتي في نسخة
Extra المدفوعة). مسيرات الجيوش حالياً تستخدم Cone الافتراضي، وعند إضافة نسخة
Extra تُسجَّل في الفهرس بنفس النمط (`unit_infantry` …) دون تغيير كود.
