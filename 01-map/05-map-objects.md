# كائنات الخريطة (Map Objects)

> **[مرجعي قديم]** هذه قائمة تصنيفية مفيدة لسجل الكائنات. مواصفة الخريطة التفاعلية وقواعد الملكية والمرور ونطاقات منشآت التحالف المعتمدة موجودة في [`design/04-world-map/MAP_AND_ALLIANCE_INTERACTION.md`](../design/04-world-map/MAP_AND_ALLIANCE_INTERACTION.md)، بينما يبقى كتالوج المباني في [`design/04-world-map/BUILDING_CATALOG.md`](../design/04-world-map/BUILDING_CATALOG.md).


## تصنيف سريع

| الفئة | أمثلة | تفاعل |
|-------|-------|--------|
| تضاريس | جبال، مياه، غابات بصرية | path block / decor |
| بنية تحتية حرب | Pass, Gate, Fortress | occupy / garrison |
| مواقع مقدسة | Altar, Shrine, Sanctum, Temple | occupy + buff |
| موارد | Food/Wood/Stone/Gold/Gem nodes | gather march |
| PvE | Barbarian, Fort, Guardian | attack for loot/XP/AP sink |
| لاعبين | City, March, Scout | core entities |
| تحالف | Flag, Alliance Fortress, Resource center | territory graph |
| موسمي | Crystal nodes, Outposts, Cores | season scoring |

---

## 1) مدينة اللاعب (City)
- نقطة على الخريطة + داخلها scene منفصل أو mode
- خصائص: owner, civilization skin, wall status, shield, coords
- Scouting يكشف تقريبيًا fortification/troops (حسب مستوى scout tech)

## 2) المسيرة (March)
- army snapshot: commanders + troops + buffs
- path, speed, eta, state: moving/gathering/combat/returning
- Action Points تُستهلك لأهداف PvE/استكشاف حسب التصميم

## 3) نقاط الموارد البرية
مستويات 1..N  
كل مستوى: inventory size، gather rate، defense guard اختياري، respawn rule

## 4) Barbarians / PvE
- Barbarian عادي: AP sink + XP قادة + موارد
- Barbarian Fort: rally target
- Guardians حول Holy Sites: لازم تُكسر قبل/أثناء الاحتلال

## 5) Holy Sites (من تفكيك النصوص)

### Altars (أضعف نسبيًا)
أمثلة أسماء من اللعبة: Flame / Surge / Storm / Earth / Harvest / Wisdom  
قواعد شائعة:
- فتح كل عدة أيام
- Hold لمدة ساعات لتثبيت الملكية
- severely wounded → hospital غالبًا

### Shrines / Sanctums (أقوى)
- بافات أعلى
- قواعد wounded أقسى أحيانًا (جزء يموت)
- rally cap أعلى

### Lost Temple / Core
- مركز الخريطة/Zone3
- أعلى قيمة سياسية وعسكرية
- حراس أقوياء + believers/reinforcement NPC في بعض السيناريوهات

## 6) Alliance Territory Objects
- **Flag**: يمد الأرض
- **Fortress**: مركز إقليم التحالف
- لازم اتصال (connected territory) لتمدد قانوني
- قيود بناء قرب Pass/Holy في بعض الفصول

## 7) Pass / Gate
انظر `04-passes-mountains.md`

---

## كثافة مقترحة (Zone1 Region واحد)

| الكائن | عدد تقريبي |
|--------|-------------|
| Resource nodes | 80–200 |
| Barbarians spawn slots | dynamic |
| Altar | 1 |
| Border passes shared | 1–2 |
| Recommended fortress spots | 2–4 |

Zone2: أقل nodes لكن أعلى level  
Zone3: موارد موسمية أقل عددًا + objectives أكبر

---

## تحديثات العميل (AOI)
السيرفر يرسل فقط الكائنات في نافذة اللاعب + الاشتراكات (مسيراتة، تحالفه، أهدافه).  
Tick حركة المسيرات: 1s أو event-based كافٍ لاستراتيجية.
