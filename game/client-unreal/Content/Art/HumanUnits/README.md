# HumanUnits — موديلات الوحدات البشرية (P8-T8)

هذه الحزمة توفر **17 موديلًا إجرائيًا low-poly** للوحدات البشرية تُستخدم على خريطة العالم حتى يُستورد بديل فني نهائي على بيئة Unreal (Windows). النمط المستهدف: stylized low-poly متسق مع KayKit (CC0) الموجود في `Content/Art/kaykit/` وKenney Castle Kit في `Content/Art/KenneyCastleKit/`.

## الخريطة الكاملة (20 شبكة × 4 فروع × 5 مراحل)

| المعرف | الملف | المرحلة | المصدر |
|--------|-------|---------|--------|
| `infantry_t1`–`infantry_t5` | `infantry_t{1..5}.glb` | Swordsman ← Imperial Guard | حزمة HumanUnits |
| `archer_t1`–`archer_t5` | `archer_t{1..5}.glb` | Bowman ← Imperial Crossbowman | حزمة HumanUnits |
| `cavalry_t1`–`cavalry_t5` | `cavalry_t{1..5}.glb` | Light Cavalry ← Imperial Knight | حزمة HumanUnits |
| `siege_t1` | `siege_arcuballista.glb` | Arcuballista | حزمة HumanUnits |
| `siege_t2` | `siege_mangonel.glb` | Mangonel | حزمة HumanUnits |
| `siege_t3` | Kenney `siege-ballista.glb` | Ballista | Kenney Castle Kit (CC0) |
| `siege_t4` | Kenney `siege-trebuchet.glb` | Trebuchet | Kenney Castle Kit (CC0) |
| `siege_t5` | Kenney `siege-catapult.glb` | Catapult | Kenney Castle Kit (CC0) |

## الوحدات الخاصة الحضارية (unlock tier 4)

| الحضارة | الوحدة | تستخدم |
|---------|--------|---------|
| Rome | Legionary | `infantry_t4` (جلد لاحقًا) |
| China | Chu-Ko-Nu | `archer_t4` (جلد لاحقًا) |
| Arabia | Desert Rider | `cavalry_t4` (جلد لاحقًا) |
| Egypt | Khopesh Guard | `infantry_t4` (جلد لاحقًا) |
| Vikings | Huskarl | `infantry_t4` (جلد لاحقًا) |
| Japan | Samurai | `infantry_t4` (جلد لاحقًا) |

الجلود الحضارية (civ-specific overrides) تُستورد لاحقًا كمواد/جلود مخصصة بعد توفر بيئة UE — حاليًا كل وحدة خاصة ترث شبكة فرعها عند `unlock_tier` (4) عبر `URok2ArtAssets::GetHumanUnitId`.

## التوليد

```bash
python3 scripts/generate_human_units_glb.py
```

المولّد (`scripts/generate_human_units_glb.py`) يبني GLB 2.0 خامًا (بدون مكتبات خارجية): مشاة ورماة بارتفاع 175cm وفرسان 190cm، بتدرج لوني tier (t1 أخضر فاتح → t5 ذهبي إمبراطوري). التدرج اللوني مطابق لـ `TIER_COLORS` في `troop_tiers.json` concept.

## الاستيراد في Unreal

```powershell
.\scripts\Import-HumanUnits.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
```

ينقل الملفات إلى `/Game/Art/HumanUnits/` ويثبّت `Conv_GLB`. بعد الاستيراد يحمّلها `URok2ArtAssets::LoadHumanUnitMesh` مباشرة من `EditorPackagePath`.

## الترخيص

الموديلات في هذه الحزمة **مولّدة إجرائيًا** بـ `scripts/generate_human_units_glb.py` (كود المشروع نفسه — لا ترخيص خارجي). موديلات الحصار T3–T5 من Kenney Castle Kit (CC0 — انظر `Content/Art/KenneyCastleKit/LICENSE.txt`). الأصول النهائية عالية الدقة المستوردة لاحقًا على Windows يجب أن تكون CC0 أو مرخصة تجاريًا (متطلبات P7-T9).
