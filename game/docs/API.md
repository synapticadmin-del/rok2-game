# ROK2 API Reference

Base local: `http://127.0.0.1:8787`

Auth header: `Authorization: Bearer <token>`

## Health / Meta
- `GET /v1/health`
- `GET /v1/meta/map`
- `GET /v1/meta/civilizations` — الحضارات الست من `data/civilizations.json` **كما هي بلا ترشيح حقول**. أضاف **P6-T5** طبقة النصّ لكل حضارة:
  - `name_ar` — الاسم العربي («روما»، «الفايكنج»).
  - `fantasy_ar` — جملة الفانتازي الواحدة (GDD §2).
  - `story` — **مصفوفة** من 3-4 أسطر: النبذة الأدبية. مصفوفة لا نصّاً واحداً لأن مواضع القطع قرارُ تأليف تقع على حدود الجُمَل؛ اللفّ الآلي يقطع حيث اتّفق.
  - `greeting` — تحية بنبرة الحضارة، يعرضها العميل بطاقةَ إشعار عند دخول المدينة.
  - `hints` — تلميحات بنبرتها. تشير إلى `bonuses` **بلا اقتباس أرقامها** (الأرقام بيانات توازن، ونصٌّ يقتبسها يصير كذبةً موثّقة لحظة تعديل قيمة).
  - العميل يفضّل هذه الحمولة على نسخته المدمجة، فتعديل نصٍّ أدبي ينزل على اللاعبين بنشر backend وحده بلا تحديث عميل.
- `GET /v1/meta/buildings`
- `GET /v1/meta/troops`
- `GET /v1/meta/commanders`
- `GET /v1/meta/techtree`
- `GET /v1/meta/all` — **بيانات التوازن الموحدة (P1-T6):** civilizations + buildings + troops + commanders + techTree + constants (productionBase, productionLevelMult, trainableUnits). يقرأها العميل مرة واحدة عند البدء بدل القيم الثابتة.

## Auth
- `POST /v1/auth/guest` `{ deviceId?, name? }` → `{ token, accountId, player? }`
- `GET /v1/me`

## City
- `POST /v1/city/init` `{ civ, name? }` → new token with playerId + `starterCommander` (قائد بداية الحضارة، P2-T1) + **`kingdom`** (المملكة المعينة، P4-T6) + **`matchmaking { strategy, fillRatio, reason }`** (سبب اختيار المملكة)
- `GET /v1/city` → player + city + buildings + troops + wounded + hospital + **`kingdom`** (مملكة اللاعب، P4-T6)
- `POST /v1/city/upgrade` `{ buildingId }`
- `POST /v1/city/train` `{ unit: infantry_t1|cavalry_t1|archer_t1, count }`
- `POST /v1/city/collect`
- `POST /v1/city/heal` `{ troops }` — شفاء الجرحى الخطيرين (P2-T2): نصف تكلفة التدريب + مدة من data/buildings.json → `{ queueId, healSeconds, cost, city }`

## Shop + VIP (P3-T4) — sandbox بدون مدفوعات حقيقية؛ القيم من data/shop.json
- `GET /v1/shop/catalog` — عناصر speedup (7: دقيقة→يوم) + مستويات VIP (7) بمزاياها
- `GET /v1/vip/status` — نقاط ومستوى VIP الحالي + مزايا المستوى
- `POST /v1/shop/buy` `{ itemId, count? }` — شراء speedup بالـ gems → مخزون + نقاط VIP
- `POST /v1/shop/use-speedup` `{ queueId, itemId? | useFreeDaily? }` — تسريع طابور من المخزون أو التسريع المجاني اليومي
- `POST /v1/shop/daily-gems` — منحة gems يومية (200)

## Battle Pass (P4-T1) — القيم من data/battlepass.json
- `GET /v1/battlepass` — حالة اللاعب (xp/level/premium) + المستويات القابلة للمطالبة لكل مسار
- `POST /v1/battlepass/unlock-premium` — فتح المسار المدفوع (500 gems)
- `POST /v1/battlepass/claim` `{ level, track: free|premium }` — مطالبة مكافأة مستوى (مرة واحدة لكل مسار)
- نقاط تُمنح تلقائياً: build 10 / train 5 / research 15 / heal 3 / march 8 / pass_attack 20

## Season (P3-T1/T2)
- `GET /v1/season/schedule` — جدول فتح المناطق الكامل (Zone unlock days + throne + core)
- `GET /v1/season/leaderboard`
- `GET /v1/season/scoreboard` — نقاط core contest (عرش + حصون + مذابح) لكل تحالف + المتصدر

## Events (P3-T3) — القيم من data/events.json
- `GET /v1/events/active` — الأحداث النشطة الآن (barbarian_invasion يومي / resource_rush يومي / war_fever جمعة) + بافاتها
- رسائل WS `event_started` / `event_ended` عند بداية/نهاية كل حدث

## Launch + Matchmaking
- `GET /v1/launch/status` (P3-T5) — ممالك الإطلاق (open/max_players) + إشغال المملكة الحالية + success_gate
- `GET /v1/matchmaking/status` (P4-T6) — إشغال كل مملكة مفتوحة (players/fill_ratio) + استراتيجية التعيين + أحدث 10 تعيينات (kingdom_id, strategy, fill_ratio, reason)
- **تعيين المملكة:** عند `city/init` يُعيَّن اللاعب الجديد للمملكة المفتوحة الأقل امتلاءً (least_fill من data/matchmaking.json) مع بوابة سعة لكل مملكة؛ الأخطاء: `kingdom_not_open_for_launch` / `kingdom_full`

## Anti-cheat (P4-T5) — الحدود من data/anticheat.json
- حدود معدل لكل لاعب (window + cooldown): march 10/د، pass_attack 5/د، help 15/د، shop_buy 20/د، use_speedup 30/د، rally 3/د
- عند التجاوز: **`429 rate_limited_cooldown | rate_limited_window_exceeded`** مع `details.retryAfterMs`
- شذوذ الحمولات يُرفض: `anticheat_single_unit_cap_exceeded` / `anticheat_total_troops_cap_exceeded` / `anticheat_max_active_marches_exceeded` / `anticheat_invalid_*`
- `GET /v1/admin/anticheat` (`x-admin-key`) — آخر 50 مخالفة مسجلة في الـ shard + عدد الجلسات المتتبعة

## Research (P2-T3) — راجع game/docs/RESEARCH.md
- `GET /v1/research` — شجرة data/research.json (economy + military × 5) مع مستويات اللاعب وتفاصيل المستوى التالي
- `POST /v1/city/research` `{ techId }` — بدء بحث (تحقق: أكاديمية + prerequisites + موارد) → `{ level, durationSec, queueId, cost }`
- البافات تُطبق فعلياً: إنتاج الموارد، سرعة التدريب، سرعة المسير، هجوم القوات في القتال

## Hospital (P2-T2) — راجع game/docs/HOSPITAL.md
- `GET /v1/city` يعيد `wounded` (الجرحى الخطيرون) + `hospital { level, capacity, used, free }`
- تقارير المعركة تحمل `hospital { admitted, died, capacity }` — الفائض فوق السعة يموت

## Commanders (P2-T1) — راجع game/docs/COMMANDERS.md
- `GET /v1/commanders` → قادة اللاعب المملوكين + الـ roster الكامل + الثوابت
- `POST /v1/commander/summon` `{ commanderId }` — استدعاء قائد (500 ذهب)
- `POST /v1/commander/levelup` `{ commanderId, tomes }` — كل تومة = 500 XP
- `POST /v1/commander/skill` `{ commanderId, skillSlot: 1..3 }` — رفع مهارة attack/defense/passive
- `POST /v1/commander/assign` `{ marchId, commanderId }` — تعيين قائد على مسيرة نشطة

## Alliance
- `POST /v1/alliance/create` `{ name, tag }`
- `POST /v1/alliance/join` `{ allianceId }`
- `GET /v1/alliance/:id` → `{ alliance, members: [{ id, name, power, region_id, rank }] }`

### Alliance — P2-T5 (رتب + helps + rally) — القواعد من data/zones.json → alliance
- **الرتب:** R1..R5 (القائد R5 عند الإنشاء، المنضم R1). الصلاحيات من rank_permissions: R4+ kick/promote، R3+ rally، الكل help
- `POST /v1/alliance/promote` `{ playerId, rank }` — ترقية/تنزيل؛ يتطلب رتبة أعلى من الهدف ومن الرتبة الجديدة
- `POST /v1/alliance/kick` `{ playerId }` — طرد عضو أدنى رتبة فقط
- `POST /v1/alliance/leave` — مغادرة طوعية (القائد R5 لا يغادر قبل نقل القيادة)
- `POST /v1/alliance/help` `{ queueId }` — كل مساعدة تخصم help.speedup_per_help_sec (60ث) من طابور عضو، بحد أقصى 10 مساعدات و30% من المدة المتبقية؛ مساعدة واحدة لكل لاعب على الطابور → `{ helpsCount, speedupSec, totalReductionSec, queue }`
- `POST /v1/alliance/rally` `{ targetType: pass|throne, targetId, troops, primaryCommanderId? }` — إطلاق حملة (R3+)؛ تجمّع 300ث (rally.prep_seconds) ثم تنطلق مسيرة واحدة بقوات كل المشاركين (حتى 5)
- `POST /v1/alliance/rally/join` `{ rallyId, troops }` — انضمام عضو بقواته أثناء التجميع
- `GET /v1/alliance/rally/:id` — حالة الحملة + المشاركون
- رسالة WS `{"type":"rally_launched","rallyId","marchId","participants"}` عند الانطلاق
- snapshot العالم يتضمن `queues` الجارية (لعرض طوابير الأعضاء القابلة للمساعدة)

## World
- `GET /v1/world/snapshot`
- `GET /v1/world/ws` (WebSocket upgrade)
- `POST /v1/world/march` `{ targetType, targetId, troops, toX?, toY?, passId?, primaryCommanderId? }`
- `POST /v1/world/pass/attack` `{ passId, troops, primaryCommanderId? }` (requires alliance)
- `POST /v1/world/scout` `{ toX, toY }` (P5-T5) — كشافة بدون قوات من مدينة اللاعب للهدف (أسرع من المسير العادي). snapshot العالم يتضمن `scouts` المتحركة، وعند الوصول يبث WS `{"type":"scout_arrived","scoutId","toX","toY"}` ليكشف العميل ضباب الحرب حول الهدف (وبث `scout_created` عند الإنشاء)

### WS client messages
```json
{"type":"hello","playerId":"..."}
{"type":"aoi_sub","x":600,"y":1000,"r":80}
{"type":"pass_attack","passId":"P_R2_R3","troops":{"infantry_t1":200}}
{"type":"march_create","targetType":"resource","targetId":"node_R2_0","troops":{"infantry_t1":50}}
{"type":"ping"}
```

## Zones (P2-T4) — مناطق مقفلة بمؤقّت زمني + موارد أعلى
- `GET /v1/meta/zones` — مواصفة data/zones.json (unlock_schedule + resource_level_range + constants)
- `GET /v1/world/zones` → `{ seasonDay, zones: [{ zoneId, regionId, unlocked, unlockDay }] }` — حالة قفل/فتح كل منطقة الآن
- snapshot العالم يتضمن `zones` بنفس البنية، ورسالة WS `{"type":"zone_unlocked","zoneId","regionId","seasonDay"}` تُبث عند بلوغ يوم الفتح
- المسيرات لأهداف داخل منطقة مقفلة (موارد/برابرة/نقاط/مدن) تُرفض بخطأ `zone_locked` حتى يوم الفتح
- عقد الموارد: مستويات ضمن resource_level_range لكل منطقة (Zone1 [1,4]، Zone2 [3,6]) وغنى Zone2 = × constants.zone2_richness_mult (1.5)
- `GET /v1/meta/all` يتضمن `zones` أيضاً

## Admin (`x-admin-key`)
- `POST /v1/admin/tick`
- `POST /v1/admin/set-time` `{ day }`
- `POST /v1/admin/grant` `{ playerId, food?, wood?, stone?, gold?, troops? }`
- `GET /v1/admin/retention` (P3-T5) — DAU + رجوع cohorts (D1/D3/D7/D14/D30) مقابل عتبات data/softlaunch.json
- `GET /v1/admin/anticheat` (P4-T5) — مخالفات anti-cheat الأخيرة في الـ shard
