# P9-T2: أراضي التحالف ومراكز الموارد

**التاريخ:** 2026-08-13 · **الحالة:** مكتمل (85 فحصًا + E2E كامل) · **commit:** P9-T2

## الملخص التنفيذي

هذا البند يبني طبقة **الأراضي الإقليمية للتحالفات** فوق نظام الرايات (flags) القائم: التحالفات التي تنشر رايات كافية حول هدفٍ استراتيجي تحصل على **أرضٍ إقليمية** تمنح أعضاؤها باف جمع موارد **+25%**، وتخفّض الأضرار التي تسبّبها المسيرات المعادية عند عبور تلك الأرض بنسبة **25%** (تأثير الحراسة/patrol). كما يضيف **قلاع التحالف (Outposts)** القابلة للبناء من رتبة R4/R5 بشروط تحالف ناضج، و**مراكز موارد محايدة** (14 مركزًا) تُجمع منها الموارد في مسيرات جمع آمنة، مع قفل تحالف المؤقت عند أول جمع، وقبول إعادة الظهور بعد استنفاد الاحتياطي.

## لماذا هذا البند

في ألعاب 4X/MMO الاستراتيجية تكون الأراضي أحد أقوى محركات الصراع والتحالف، لأنها تحوّل الخريطة من فضاءٍ مفتوح إلى مناطق نفوذ متنافسة. بدونها يبقى التنافس محصورًا في الممرات، ومعها يصبح للتحالف هدفٌ جغرافي يحميه ويتوسع فيه، ويظهر سببٌ عملي لتجنيد الأعضاء وبناء القوات.

## المكونات المنفّذة

### 1. بيانات الأراضي (`src/data/alliance_territory.json`)

كل الثوابت مقروءة من JSON لا hard-coded:

| الثابت | القيمة | الدلالة |
|---|---|---|
| `flag_radius` | 500 | نصف قطر تأثير الراية |
| `outpost_radius` | 300 | نصف قطر تأثير القلعة |
| `gather_multiplier` | 1.25 | باف الجمع داخل الأرض |
| `patrol_reduction` | 0.25 | تخفيض ضرر الممرات العابرة لأرض التحالف |
| `center.gather_capacity` | 5000 | احتياطي كل مركز موارد |
| `center.lock_minutes` | 15 | مدة قفل المركز لتحالف الجمع |
| `outpost.max_outposts_per_alliance` | 12 | سقف القلاع للتحالف الواحد |
| `outpost.min_player_count` | 10 | الحد الأدنى للأعضاء |
| `outpost.min_total_power` | 250000 | الحد الأدنى للقوة الكلية |
| `outpost.min_hall_level` | 10 | الحد الأدنى لمستوى قاعة المدينة |

الأنواع الأربعة لمراكز الموارد: `granary` و`wood_lot` و`stone_pit` و`mother_lode` (14 مركزًا عبر الخريطة عبر `seedCenters`).

### 2. المنطق النقي (`src/do/sim/territory.ts`)

دوال نقية بلا حالة، تختبرها وحدةُ الجودة المستقلة:

| الدالة | السلوك |
|---|---|
| `flagRadius()` / `outpostRadius()` | أنصاف أقطار التأثير من JSON |
| `insideTerritory(x, y, castles, allianceId)` | هل النقطة داخل أي دائرة قلعة/راية للتحالف |
| `marchCrossesTerritory(x1,y1,x2,y2, castles, allianceId)` | 8 عينات على خط الممر — هل يعبر أرض التحالف |
| `gatherMultiplier()` / `patrolReduction()` | قيم البافات |
| `canBuildOutpost(cfg, castles, members, power, hallLevels)` | شروط بناء القلعة: سقف 12، عضو ≥10، قوة ≥250K، قاعة ≥10 |
| `validPosition(x, y, cfg, castles)` | داخل الخريطة وبُعد كافٍ عن القلاع الأخرى |
| `seedCenters(day)` / `respawnDueCenters(now)` | توزيع البذور وعودة المراكز المستنفدة |
| `centerGatherAmount(troops, reserve)` | `max(1, floor(troops/100))` مقيدًا بالاحتياطي |
| `centerResource(kind)` / `centerKinds()` | تحويل النوع إلى مورد وقائمة الأنواع |

### 3. Durable Object (`src/do/KingdomShard.ts`)

- جدولا `alliance_centers` و`alliance_outposts` في `ensureCoreWorldTables` وهجرة `ver<13`.
- حقلا الحالة `allianceOutposts` و`resourceCenters` مع `persistCenter`/`persistOutpost` (INSERT OR REPLACE).
- `seedAllianceCenters()` بعد `seedHolySites` في إقلاع العالم.
- `castlesList()` تدمج الرايات والقلاع في قائمة موحدة لكل حسابات الأرض.
- **snapshot/worldDelta**: تُصدَر `allianceOutposts` و`resourceCenters` و`territoryCfg`.
- **tick**: `respawnDueCenters` يعيد ملء المراكز المستنفدة؛ ومسيرات الجمع داخل أراضي التحالف تحصل على `gather_multiplier` (1.25) فوق باف الفعاليات.
- **resolveMarchArrival — فرع `center`**: المسيرة تصبح `gathering` وتستخرج الموارد من احتياطي المركز دفعةً دفعة في tick حتى الاستنفاد، ثم تُعيد مسيرتها العائدة عبر `payload{kind, amount}` — المسارات القائمة للجمع العائد تغطيها دون تعديل.
- **أمن المراكز**: المركز يُقفل للتحالف الأول الذي يجمع منه (`lock_minutes=15`)؛ المسيرات الأجنبية تُرَدّ بمسيرة عائدة دون قتال — مراكز الموارد **غير قابلة للهجوم** (جمع آمن لا قتال).
- **patrol mods**: في كل فروع القتال الخمسة (ممر، عرش، هدف رئيسي، موقع مقدس، بربر)، إذا عبر مسار المسير أرضَ تحالفٍ مدافع، يُخفَّض ضرر المهاجم بـ `patrol_reduction`.
- **createMarch/redirectMarch**: دعم `targetType: "center"` مع رفض `center_locked` ومسافة/هدف صحيحين.
- **endpoints داخلية**: `/territory-state` و`/territory-centers` و`/build-outpost` (بترخيص رتبة ومعدل حد).

### 4. الراوتر (`src/http/router.ts`)

| المسار | الصلاحية | السلوك |
|---|---|---|
| `GET /v1/territory/state` | لاعب مسجَّل | لقطة الأرض الكاملة: قلاع + مراكز + إعدادات |
| `GET /v1/territory/centers` | لاعب مسجَّل | مراكز الموارد وحالة احتياطيها وقفلها |
| `POST /v1/alliance/outpost` | R4/R5 بصلاحية `territory` | بناء قلعة جديدة بشروط `canBuildOutpost` وحد معدل `alliance_outpost` |
| `set-alliance` | — | يمرّر `allianceName`/`rank` لرفع عضوية التحالف في الذاكرة |

### 5. الصلاحيات ومعدل الحدود

- `src/data/zones.json`: صلاحية `territory` أُضيفت لرتبتي **R4 وR5** في `rank_permissions`.
- `src/data/anticheat.json` + `sim/anticheat.ts`: حد معدل `alliance_outpost` (نافذة ساعة، 10 طلبات، تهدئة 30 ثانية).

## الجودة

- **حارس الوحدة** `scripts/alliance_territory_offline_test.mjs`: **85 فحصًا، 0 فشل** — كل دالة من `sim/territory.ts` معاد تنفيذها محليًا ومقارنة مخرجاتها بالـ JSON.
- **E2E كامل** `e2e_p7_t3` (rally + combat + season story): **P7-T3 FULL E2E PASSED** — جرى إصلاحه خلال هذا البند: wrangler 4 لم يعد يحترم متغيري `WRANGLER_D1_STATE_PATH`/`WRANGLER_DO_STATE_PATH`، فاستُبدلا بـ `--persist-to sandboxDb` في إقلاع `wrangler dev` وتطبيق الهجرات.
- **TypeScript**: `npx tsc --noEmit` نظيف.
- **npm run check**: سلسلة الجودة كاملة EXIT 0 (تشمل `test:p9-t2-alliance-territory` و`test:pie-android-acceptance`).

## تكامل مع البند السابق

P9-T2 يبني فوق P9-T1 (تكنولوجيا التحالف): قلاع outpost تستخدم آلية `allianceStructures` القائمة، وصلاحيات الرتب تمتد بنفس نمط `rankHas`، ولقطة العالم تُصدِر الآن البنيات الثلاث (تقنية/رايات/قلاع/مراكز) في دفعة واحدة للعميل.

## ملاحظات للعميل (UE5)

- `territoryCfg` ظهر في snapshot — على العميل تخزين أنصاف الأقطار ورسم حلقات التأثير للرايات والقلاع.
- `resourceCenters` جديدة: نوع عقدة `center` بمسيرة جمع آمنة (لا قتال) — تحتاج حالة UI لاحتياطي المركز وقفل التحالف.
- مسيرات الجمع داخل أراضي التحالف تُعرض بباف جمع +25% (نفس نمط باف الفعاليات).
