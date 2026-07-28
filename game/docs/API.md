# ROK2 API Reference

Base local: `http://127.0.0.1:8787`

Auth header: `Authorization: Bearer <token>`

## Health / Meta
- `GET /v1/health`
- `GET /v1/meta/map`
- `GET /v1/meta/civilizations`
- `GET /v1/meta/buildings`
- `GET /v1/meta/troops`
- `GET /v1/meta/commanders`
- `GET /v1/meta/techtree`
- `GET /v1/meta/all` — **بيانات التوازن الموحدة (P1-T6):** civilizations + buildings + troops + commanders + techTree + constants (productionBase, productionLevelMult, trainableUnits). يقرأها العميل مرة واحدة عند البدء بدل القيم الثابتة.

## Auth
- `POST /v1/auth/guest` `{ deviceId?, name? }` → `{ token, accountId, player? }`
- `GET /v1/me`

## City
- `POST /v1/city/init` `{ civ, name? }` → new token with playerId + `starterCommander` (قائد بداية الحضارة، P2-T1)
- `GET /v1/city`
- `POST /v1/city/upgrade` `{ buildingId }`
- `POST /v1/city/train` `{ unit: infantry_t1|cavalry_t1|archer_t1, count }`
- `POST /v1/city/collect`
- `POST /v1/city/heal` `{ troops }` — شفاء الجرحى الخطيرين (P2-T2): نصف تكلفة التدريب + مدة من data/buildings.json → `{ queueId, healSeconds, cost, city }`

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
