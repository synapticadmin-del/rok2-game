# تحليل التثبيت المحلي — Rise of Kingdoms

**المسار المفحوص:**  
`C:\Program Files (x86)\Rise of Kingdoms`

**تاريخ الفحص:** 2026-07-22

---

## 1) بنية المجلد

```
Rise of Kingdoms/
├── launcher.exe                 # المشغّل
├── launcher_version_map.txt
├── cef/                         # Chromium Embedded (ويب داخل المشغّل)
├── dependence/                  # خطوط (عربي/صيني…)
├── update/
├── save/
└── Rise of Kingdoms Game/
    ├── MASS.exe                 # العميل الفعلي
    ├── UnityPlayer.dll
    ├── GameAssembly.dll         # IL2CPP code
    ├── NEP2.dll
    ├── eng.out / ez.out
    ├── version_map.txt          # manifest ملفات + md5
    └── MASS_Data/
        ├── boot.config
        ├── globalgamemanagers*
        ├── level0 / level1
        ├── il2cpp_data/Metadata/global-metadata.dat
        ├── Plugins/
        └── StreamingAssets/     # بيانات اللعبة الثقيلة
```

---

## 2) استنتاجات تقنية

| البند | الاستنتاج |
|-------|-----------|
| المحرك | **Unity** |
| Backend اللغة على العميل | **IL2CPP** (C++ متولد، ليس سورس C# قابل للقراءة مباشرة) |
| الشبكة/الحسابات | سيرفرات Lilith (المنطق الحساس ليس محليًا بالكامل) |
| التوطين | حزم `lc_*.dat` = ZIP فيها `*_en.bin` مشفّرة XOR بمفتاح بايت واحد مختلف لكل حزمة |
| الأصول | ملفات `.lb` / bundles / فيديوهات / سكنات |
| خرائط الموسم | أسماء `LostLand_Map_*` كثيرة (سيناريوهات KvK متعددة) |

### مفاتيح XOR التي نجحت على حزم EN (مرجع بحثي)
| الحزمة | المفتاح |
|--------|--------:|
| map | 0xDA |
| civpedia | 0x15 |
| hero | 0x1D |
| common | 0xAA |
| kingdomwar | 0x42 |
| build | 0xF5 |
| research | 0xF6 |
| event | 0xDA |
| alliance | 0x42 |
| store | 0xAE |
| quest | 0x82 |

النصوص المفكوكة محفوظة في: `rok2/_extracted/*_en.strings.txt`

---

## 3) أنظمة ظاهرة من أسماء الملفات/الكلاسات (`ls.dat`)

### الخريطة والحرب
- `WorldObjPass`, `MapUIPass`, `PassUnlockPopup`
- `WorldObjCanyonAltar`, `MapUIAltar*`, `MapUITemple*`
- `TempleData`, `TempleHandler`, `LostTemple*`
- `LostLandData`, `LostLandHandler`, `KVKTheLostLandMainpage`
- `LostLand_Map_1` … `LostLand_Map_20` (+ نسخ `_v2`, `_2_v2`)
- `StrategicViewProvinceTittle`, `ProvinceListBar`

### التحالف/القتال
- `AggressiveRally*`, `AllianceRallyMain`
- `GarrisonHeroReplaceMail`

### الحضارة
- `AllUISelectCivilization*`, `CivilizationFlagList*`
- `EventCivilizationFiveDay*`, `CivilizationThemeCN`

### الموسم/KVK
- `KvkPolicy*`, `KvkPreview*`, `KvkDuel*`, `KvkEarthFort*`
- `KvkS4Crystal*`, `KVKS6CampAndSubBase*`, `KvkS15Outpost*`
- `KVKSelectSide*`, `ConquerKVKTeam*`
- `KvkSeasonReview*`

### اقتصاد/تقدم
- `BattlePass*` (عدة أنواع: Newbie/Museum/Crystal/GVG/Pioneer…)
- مباني Crystal في بعض السيناريوهات: `CityBuildObjectCrystalFactory`

---

## 4) ماذا يعني هذا لتصميم ROK2؟

1. **الخريطة ليست flat مفتوح بالكامل** — فيها Provinces/Passes/Holy sites/LostLand scenarios.
2. **الموسم = منتج كامل** (خرائط متعددة، سياسات، فرق، كريستال، outposts…).
3. **الحضارة نظام UI + flags + أحداث ثيم** وليس مجرد رقم باف.
4. **العميل عرض + إدخال**؛ التحقق الحقيقي سيرفري (متوقع في هذا النوع).
5. تعقيد RoK النهائي **أكبر بكثير** من MVP — لذلك Zone1/2/3 الذي صممناه اختصار ذكي وواضح.

---

## 5) حدود التفكيك

- لا يوجد سورس كود C# مقروء (IL2CPP).
- جداول التوازن الرقمية النهائية (ATK values…) ليست نصوصًا صريحة في locale؛ غالبًا في binaries/config مشفرة أو سيرفر.
- ما استخرجناه قوي لـ **taxonomy الأنظمة + UX strings + map vocabulary**، وليس نسخ معادلات القتال رقمًا برقم.

---

## 6) ملفات StreamingAssets المفيدة بحثيًا

| ملف | دلالة |
|-----|-------|
| `lc_map.dat` | نصوص الخريطة/المواقع/الممرات |
| `lc_kingdomwar.dat` | Lost Kingdom / مواسم |
| `lc_hero.dat` | قادة |
| `lc_civpedia.dat` | موسوعة حضارات |
| `lc_build.dat` | مباني |
| `lc_research.dat` | أبحاث |
| `lc_alliance.dat` | تحالف |
| `lc_troop.dat` | جنود (حزمة صغيرة نسبيًا) |
| `lc_event.dat` | أحداث |
| `ezRes_ground_*.bytes` | بيانات أرض/خريطة بصرية |
| `ls.dat` | فهرس/حزم أنظمة ضخمة |

---

## 7) توصية هندسية لمشروعك (لا استنساخ ملفات)

ابنِ من الصفر:
- Unity client نظيف
- Backend services
- JSON/ScriptableObjects لأنظمتك
- Map generator لـ 8 regions + rings

لا تُضمّن ملفات `.dat` / `.dll` من RoK في مشروعك.
