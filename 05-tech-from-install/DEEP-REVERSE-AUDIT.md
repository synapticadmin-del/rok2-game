# تدقيق تفكيك عميق — Rise of Kingdoms (ختام قدرات التحليل)

**تاريخ:** 2026-07-22  
**هدف المرحلة:** ختم ما يمكن (وما لا يمكن) استخراجه من التثبيت المحلي قبل الانتقال لتصميم ROK2 الخاص.

**مسار التثبيت المفحوص:**  
`C:\Program Files (x86)\Rise of Kingdoms`

---

## 0) الخلاصة التنفيذية (Executive Verdict)

| السؤال | الجواب |
|--------|--------|
| هل يوجد سورس كود C# مقروء؟ | **لا** — العميل IL2CPP (`GameAssembly.dll`) |
| هل يمكن تفكيك “كل اللعبة” كمنطق سيرفر؟ | **لا** — المنطق الحساس (اقتصاد/قتال/Anti-cheat) **سيرفري** |
| هل يمكن استخراج taxonomy كامل للأنظمة؟ | **نعم بدرجة عالية** عبر `ls.dat` + locale + assemblies |
| هل يمكن استخراج معادلات القتال الرقمية النهائية؟ | **لا بشكل موثوق من العميل وحده** |
| هل يمكن فهم معمارية الخريطة؟ | **نعم على مستوى أنظمة/مفردات/طبقات أصول** + استنتاجات من نصوص |
| هل ظهر ما يؤكد 8 مناطق بداية؟ | **نعم** — نصوص Lost Kingdom: *“8 crusader fortresses… eight starting provinces”* و *“eight kingdoms of a region”* |
| هل ننسخ باينري/أصول RoK داخل ROK2؟ | **لا** — قانونيًا وهندسيًا نبني من الصفر |

**الحكم:** اكتمل **تقطير الأنظمة + حدود التفكيك**.  
ما تبقى من “كود السيرفر الحقيقي” غير موجود على الجهاز. الانتقال لتصميمنا أصبح مبررًا.

---

## 1) بصمة البناء (Build Fingerprint)

| البند | القيمة |
|-------|--------|
| اسم التطبيق | Rise of Kingdoms |
| شركة في app.info | LegouTech |
| التنفيذي | `MASS.exe` |
| المحرك | **Unity** |
| إصدار Unity (من AssetBundles) | **2022.3.62f3** |
| كود اللعب | **IL2CPP** → `GameAssembly.dll` (~53.3 MB) |
| build-guid | `1f8f36c96c3847528c317c3e4b6aed60` |
| عدد ملفات version_map | **1704** |
| StreamingAssets | ~**4.3 GB** / **1245** ملف |
| MASS_Data إجمالي ملفات | ~**1654** |

### مكتبات أصلية مهمة (Native)
| DLL | دلالة |
|-----|--------|
| `Ez.dll` | محرك/نظام Map EZ (terrain pack) |
| `EngineDll.dll` | طبقة محرك مخصصة |
| `limpc.dll` | كبير (~9.8MB) — مرشح شبكة/منصة Lim |
| `rail_api*.dll` | منصة توزيع (Rail) |
| `nertc-c-sdk.dll` / NetEase RTC | صوت/RTC |
| `AVProVideo*.dll` | فيديو |
| `Vuplex` / `ZFBrowser` | WebView داخل العميل |

### Assemblies المدارة (من ScriptingAssemblies.json)
- `Assembly-CSharp.dll` / `firstpass` — قلب اللعب (متحول IL2CPP)
- `mapGT.Scripts.dll` — سكربتات الخريطة
- `LG.MapEditor.PrefabLibrary.Runtime.dll` — مكتبة محرر خرائط
- `LG.LGUI.*` — UI framework داخلي
- `sub_gameplay_base.dll` — أنظمة sub-gameplay (مثل DeerHunter)
- `Sirenix.*` (Odin) — تسلسل/أدوات
- `Newtonsoft.Json` / `System.Text.Json`
- `Sentry*` — crash/telemetry
- `Cinemachine` — كاميرا
- `Aliyun.OSS` — تخزين سحابي محتمل للأصول/رفع
- `IFix.Core` — hot fix / patch runtime محتمل

---

## 2) طبقات العميل (Client Layer Cake)

```
[Launcher + CEF web]
        ↓
[MASS.exe / Unity Player]
        ↓
[IL2CPP GameAssembly + Managed facade]
        ↓
┌──────────────────┬────────────────────┬────────────────────┐
│  City / UI / IAP │  Map EZ Runtime    │  Net + Platform    │
│  LGUI, Prefabs   │  Ez.dll + ezRes*   │  limpc / sockets   │
│  lc_*.dat texts  │  .btil tiles/mats  │  auth / sessions   │
└──────────────────┴────────────────────┴────────────────────┘
        ↓
[Authoritative Game Servers — NOT in install]
```

**استنتاج:** العميل غني بالعرض والـ state المحلي المؤقت، لكن **مصدر الحقيقة** للحرب/الموارد/النتائج = السيرفر.

---

## 3) ما تم تفكيكه بنجاح (Achieved)

### 3.1 فهرس الأنظمة من `ls.dat`
- **9622** اسم كلاس/نظام بنمط `Something.lsUT`
- ملف كامل: `ls_classes_all.txt`
- تصنيف تقريبي (تداخل متوقع لأن الاسم قد يطابق أكثر من فئة):

| المجال | عدد تقريبي |
|--------|------------:|
| UI | 8473 |
| IAP/Event/Quest/Mail/Chat | 2020 |
| Map/World/Pass/Temple | 1184 |
| KVK/Season/LostLand | 1087 |
| Combat/March/Troop | 914 |
| Alliance/Territory | 663 |
| Hero/Commander | 637 |
| City/Build | 472 |
| Net/Msg | 52 |
| Civilization | 31 |

> UI ضخم جدًا = اللعبة LiveOps/Product-heavy وليست فقط combat sim.

### 3.2 التوطين (Locale pipeline)
```
lc_*.dat  = ZIP
  └── *_en.bin = XOR single-byte key (مفتاح يختلف لكل حزمة)
```
تم فك: map, hero, civpedia, build, research, kingdomwar, alliance, common, event, store, quest, troop, tutorial...

المخرجات: `rok2/_extracted/*`

### 3.3 مفردات الخريطة المؤكدة من النصوص
- **Province / Region / King's Land (center province)**
- **Pass levels** تتدرج (1→2→3 في المملكة، وفي Lost Kingdom حتى 4…9+)
- **Altars / Shrines / Sanctums / Lost Temple**
- **Alliance Flags / Fortresses**
- **Barbarian / Fort / Guardians**
- **Lost Kingdom starting provinces = 8**
- فتح الممرات مربوط بـ **Chapters** + تأخير ساعات بعد الفصل
- قيود: لا عبور Pass غير محتل؛ لا teleport قرب Pass؛ بعض الممرات حسب home province/camp

### 3.4 محرك الخريطة البصري (Map EZ)
صيغة الحزم:
- `ezResPack001` + أجزاء `ezfastbin`
- ملفات:
  - `ezRes_ground_map.bytes` (~2.2MB) — فهرس/بيانات خريطة
  - `ezRes_ground_tile.bytes` (~15.8MB) — **273** بلاطة `Tile_*.btil`
  - `ezRes_ground_umesh.bytes` (~53MB) — meshes
  - `ezRes_ground_items.bytes` — عناصر أرض
  - `ezRes_ground_mtls.bytes` — **851** مادة `mapez_*`
- مواد تضاريس واضحة:
  - mountain_grass / mountain_clay (+ edge)
  - river01/02 / water_river
  - PURE_TYPE_GRASS / CLAY
  - bridge_grass
  - LODs للأرض (`lod1`, `lod3`…)

AssetBundles UnityFS للخريطة:
- `map_ez_assets.lb` (~114–120MB)
- `auto_share_new_map.lb` (~94–98MB)
- `auto_share_map.lb`
- `ez_data_tiles.lb` / `ez_data_mesh.lb` / `ez_data_map.lb`
- `map_building*.lb` (holy/lost_land/rebel/egypt…)

### 3.5 مدينة/بحث/جنود (من locale)
مبانٍ كاملة تقريبًا (City Hall… Tavern… Hospital…)  
شجرة بحث Economic/Military مع أسماء T1–T4  
قادة وندرة ومسارات مواهب  
(انظر ملفات `03-systems` و`02-civilizations`)

### 3.6 مواسم Lost Kingdom (تقطير قواعد)
من `kingdomwar` strings:
1. دخول Lost Kingdom → spawn في **starting province** للمملكة/camp
2. City Hall ≥ 16 للـ teleport داخل المقاطعة المقابلة
3. بعد الدخول: Territorial/Targeted teleports داخل territory
4. **8 crusader fortresses** — واحد لكل starting province
5. Pass levels تُفعّل تدريجيًا بنهاية الـ chapters
6. قيود بناء Alliance Fortress داخل center province / قرب بعض الأهداف
7. Seasons 1/2/3 + Season of Conquest + immigration rules حسب region

---

## 4) ما لم يُفك / حدوده (Hard Limits)

| الهدف | الحالة | السبب |
|-------|--------|-------|
| سورس C# كامل | متعذر مباشرة | IL2CPP |
| global-metadata strings الواضحة | ضعيفة/مشفرة أو غير نافعة كما هي | metadata magic `AF1BB1FA` لكن المحتوى غير readable كـ plain API map بسهولة |
| جداول ATK/DEF/HP النهائية | غير موثوقة من locale | أرقام التوازن binary/server |
| Combat formula exact | غير متاح | Server-side resolution |
| بروتوكول الشبكة كامل | غير موثق هنا | يحتاج runtime capture (mitm) — خارج نطاق الملفات الساكنة وقد يخالف ToS |
| إحداثيات RoK الحية لكل object | غير مستخرجة رقمًا | تُحمّل من سيرفر/حزم map runtime |
| نسخ LostLand_Map geometry | غير مستحسن وغير مكتمل | أصول محمية + صيغ مخصصة |

### حدود أخلاقية/قانونية/تشغيلية
- تفكيك للتعلّم المعماري **مسموح كتحليل أنظمة** في إطار بحثك المحلي.
- **إعادة توزيع** DLL/LB/نصوص/أصول RoK = انتهاك.
- **عكس بروتوكول السيرفر للغش/بوتات** = مرفوض.
- هدفنا: **تقطير تصميم** ثم بناء محركنا.

---

## 5) خريطة الأنظمة المقطّرة (System Distillation)

### A) Meta / Account
Auth, device bind, server list, immigration, kingdom assignment, VIP, settings, Sentry crashes.

### B) City Economy
Build queues, production, storehouse protection, training, healing, research, scavenger/speedups, shop.

### C) Commanders
Rarity ladder, skills 1–5, stars/sculptures, talent trees, pair primary/secondary, tavern chests.

### D) Army & March
Troop tiers/branches, load/speed, march states, scout, AP, reports.

### E) Alliance
Roles, helps, tech, territory graph (flags/forts), rallies, garrisons, chat.

### F) World Map
Provinces, mountains/terrain mats, passes, holy sites, barbarians, AOI UI (`MapUI*`), path constraints.

### G) Seasonal War (LostLand/KVK)
Multiple map scenarios (`LostLand_Map_*`), chapter-gated pass unlocks, camps/sides, crystals/outposts variants, scoring, rewards.

### H) LiveOps
Events, battle passes (عديدة), mail, quests, IAP bundles, civ theme events.

### I) Presentation
LGUI, massive prefab UI, video, audio banks, civilization skins, particles.

---

## 6) نموذج بيانات الخريطة كما يُفهم من العميل

```
Static (client assets):
  terrain packs (ezRes*) → visual mesh/tile/material/LOD
  map buildings prefabs (holy/pass/fort...)
  locale names/rules text

Dynamic (server → client):
  city positions
  march paths/eta
  pass ownership & state
  holy ownership & timers
  resource node remain
  alliance territory
  fog/intel from scouts
  season phase flags
```

**لا تفترض أن ملف `.bytes` المحلي = كامل لعبة الحرب.**  
هو طبقة عرض + قوالب؛ السيادة للسيرفر.

---

## 7) أدوات ومسارات تحليل مستقبلية (اختياري — خارج الحالي)

إذا احتجتم طبقة أعمق لاحقًا (بحذر قانوني):
1. Il2CppDumper + metadata (قد يفشل جزئيًا مع الحماية)
2. AssetStudio/AssetRipper على بعض `.lb` غير المحمية
3. تحليل ديناميكي للذاكرة أثناء اللعب (إحداثيات حية)
4. توثيق UX باليد من داخل اللعبة (أدق للـ GDD)

**للمشروع ROK2: غير ضروري الآن.** لدينا ما يكفي لـ Map Spec والتنفيذ.

---

## 8) ملفات المخرجات في هذا المجلد

| ملف | محتوى |
|-----|--------|
| `DEEP-REVERSE-AUDIT.md` | هذا التقرير |
| `install-analysis.md` | الجولة الأولى |
| `ls_classes_all.txt` | 9622 نظام |
| `ls_taxonomy_counts.json` | إحصاء التصنيف |
| `map_materials.txt` | 851 مادة |
| `map_btil_list.txt` | 273 بلاطة |
| `../_extracted/*` | نصوص مفكوكة |

---

## 9) قرار الإغلاق (Audit Closed)

### يمكن اعتبار تفكيك العميل **مختومًا** للأغراض التالية:
- فهم الفئة والأنظمة
- إثبات بنية 8 مقاطعات بداية في Lost Kingdom
- فهم Pass progression / Holy / Temple / Alliance territory
- فهم stack التقني (Unity 2022 LTS + EZ map + IL2CPP + LiveOps)

### لا ننتظر من التثبيت:
- معادلات سرية
- كود سيرفر
- خريطة إحداثيات جاهزة 1:1 للنسخ

### التالي الصحيح:
- **Map Spec ROK2 الدقيق** (إحداثياتنا)
- ثم التصميم الخاص (هوية + GDD + Prototype)

---

*نهاية تقرير التدقيق العميق.*
