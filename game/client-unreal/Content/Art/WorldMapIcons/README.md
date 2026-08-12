# WorldMapIcons — حزمة أيقونات خريطة العالم (P7-T10)

أصول بصرية أصلية (PNG شفاف 512×512) تحل محل الأشكال الهندسية الافتراضية
(Cone/Cylinder/Sphere) لعُقد الخريطة والمسيرات في `ARok2WorldRenderer`.

## الجدول

| الملف | الاستخدام | المستهلك |
|---|---|---|
| `icon_node_wheat.png` | عقدة مورد طعام | `Rok2WorldIconography` → `node_wheat` |
| `icon_node_wood.png` | عقدة مورد خشب | `node_wood` |
| `icon_node_stone.png` | عقدة مورد حجر | `node_stone` |
| `icon_node_gold.png` | عقدة مورد ذهب | `node_gold` |
| `icon_node_barbarian.png` | أي عقدة برابرة (كل التراتب) | `node_barbarian` |
| `icon_node_resource_generic.png` | مورد غير مصنف أو marker عام | `node_resource` / `world_marker` |
| `icon_objective_throne_crown.png` | العرش/التاج (الهدف النهائي) | `objective_throne_crown` |
| `icon_objective_pass_gate.png` | الممرات والبوابات | `objective_pass_gate` |
| `icon_alliance_bastion.png` | منشأة تحالف: برج الحراسة | `alliance_bastion` |
| `icon_alliance_catapult.png` | منشأة تحالف: منجنيق | `alliance_catapult` |
| `icon_march_infantry.png` | مسيرتها الأبرز مشاة | `march_infantry` |
| `icon_march_cavalry.png` | مسيرتها الأبرز فرسان | `march_cavalry` |
| `icon_march_archer.png` | مسيرتها الأبرز رماة | `march_archer` |
| `icon_march_siege.png` | مسيرتها الأبرز حصار | `march_siege` |

## ملاحظات

- الأسلوب البصري: stylized low-poly medieval (KayKit-compatible) مطابق
  لوثيقة التصميم في `07-game-design/civilizations-visual-design.md`.
- الأيقونات مصممة باتجاه +X؛ يُعرضها `SpawnSpriteActor` على
  `UBillboardComponent` فتواجه الكاميرا دائماً (لا حاجة لتدوير حسب الاتجاه).
- في `URok2ArtAssets::LoadWorldMapIcon` تحميل مخبأ (cache)؛ الفشل في
  التحميل يجعل الراسم يعود تلقائياً إلى الشكل الهندسي الافتراضي (fallback
  كامل — لا انهيار عند غياب الحزمة).
- الاستيراد إلى `/Game/Art/WorldMapIcons` عبر:
  `scripts\Import-WorldMapIcons.ps1` أو `import_assets.py` (Job مضاف).
- أيقونات المسيرات لا تحمل تلوين التحالف؛ الفرع يتحدد من `FRok2MarchEntity.Branch`
  (الفرع الأبرز في `Troops` map حسب العدد).
