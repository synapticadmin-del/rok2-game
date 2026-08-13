# P9-T6: صناديق هدايا التحالف (Alliance Gift Boxes)

## الهدف

صناديق جماعية تظهر لكل أعضاء التحالف عند مصدرها (بائعة الصناديق، صندوق النصر، تبرع التكنولوجيا، الفوز بـ rally). كل عضو يفتح الصندوق **مرة واحدة** خلال نافذة 24 ساعة ويحصل على **مكافأة عشوائية مرجحة** من pool النوع.

## الثوابت (من `src/data/alliance_gifts.json` — لا hard-coded)

| ثابت | قيمة | معنى |
|---|---|---|
| `max_active_gifts_per_alliance` | 10 | سقف الصناديق النشطة لكل تحالف |
| `gift_open_window_ms` | 86,400,000 (24 ساعة) | نافذة فتح الصندوق |
| `max_opens_per_member_per_gift` | 1 | فتحة واحدة لكل عضو لكل صندوق |
| `max_daily_opens_per_member` | 30 | فتحات يومية لكل عضو (1/ساعة × 30) |
| `min_members_for_gift` | 1 | الحد الأدنى من الأعضاء لإنشاء صندوق |

## أنواع الصناديق (4 أنواع في JSON)

| id | source | min_hall_level |
|---|---|---|
| `quest_weekly_gift` | weekly_chest | 8 |
| `victory_gift` | victory | 10 |
| `tech_gift` | tech_donation | 9 |
| `rally_gift` | rally_win | 10 |

كل نوع له `pool` عناصر مرجحة: مورد (food/wood/stone/gold) أو تسريع (speedup_1h) أو جواهر (gems).

## المنطق النقي `src/do/sim/alliance_gifts.ts`

- `createGift()` — يتحقق: نوع معروف، مستوى قاعة المدينة ≥ `min_hall_level`، أعضاء ≥ `min_members_for_gift`، سقف نشط غير ممتلئ. يختار عنصر pool بوزن مرجح.
- `claimGift()` — يتحقق: غير منتهٍ، عضو فعلي، لم يفتح من قبل، فتحات متبقية (≤ maxOpeners)، تحت السقف اليومي. يُرجع المكافأة المصنفة (resource/speedup/gems).
- `expiredGifts()` / `giftOpenSlotsRemaining()` / `pickPoolItem()` / `giftPoolTotalWeight()` — دوال مساعدة نقية.
- `buildGiftForPersistence()` — يجهّز الصندوق للحفظ (قائمة الفاتحين).

## التخزين السلطوي (KingdomShard — migration 16)

- جدول `alliance_gifts` (id, alliance_id, gift_type_id, items_json, created_ms, expires_ms, openers_json, max_openers) في D1 للشارد.
- جدول `alliance_gift_claims` (player_id, day, gift_id, reward_json) — PK ثلاثي يضمن idempotency لكل فتحة يومية.
- `loadAllianceGifts()` عند تهيئة الشارد — يستبعد المنتهي فورًا.
- `expireAllianceGiftsFor()` قبل كل قراءة/مطالبة — تنظيف + حذف سجلات المطالبات اليتيمة.

## endpoints

| المسار | الطريقة | الوظيفة |
|---|---|---|
| `GET /v1/alliance/gifts/list` | GET | قائمة الصناديق النشطة للتحالف + slotsRemaining |
| `POST /v1/alliance/gifts/create` | POST | إنشاء صندوق (R3+ فقط) — النوع يُتحقق منه من JSON |
| `POST /v1/alliance/gifts/claim` | POST | فتح صندوق — تسليم المكافأة سلطوي في D1 (موارد/gems في cities + تسريع في player_inventory) |

مسارات الشارد الداخلية: `alliance-gift-list` / `alliance-gift-create` / `alliance-gift-claim`.

## التسليم الفوري للمكافأة (router)

- مورد: `UPDATE cities SET {resource}={resource}+?` بعد `refreshCity`.
- جواهر: `gems=gems+?`.
- تسريع: `INSERT INTO player_inventory ... ON CONFLICT DO UPDATE SET count = count + ?`.

## rate limits (anticheat.json)

- `gift_claim`: 30/دقيقة، cooldown 500ms.
- `gift_create`: 10/دقيقة، cooldown 2000ms.
- مع fallback في `anticheat.ts` إن غاب الفعل عن JSON.

## حارس الجودة

`scripts/alliance_gifts_offline_test.mjs` — يعيد تنفيذ القواعد محليًا من JSON:
- 254 فحصًا: صحة البيانات، إنشاء/مطالبة/انتهاء/امتلاء/سقف يومي/توزيع مرجح/أنواع المكافآت.
- `node scripts/alliance_gifts_offline_test.mjs` → `ALL PASSED`.

## تسلسل E2E

1. لاعب R3+ (قاعة ≥ مستوى النوع) يفتح `POST /v1/alliance/gifts/create {giftTypeId}`.
2. الصندوق يظهر لكل الأعضاء عبر `GET /v1/alliance/gifts/list`.
3. كل عضو يستدعي `POST /v1/alliance/gifts/claim {giftId}` مرة واحدة خلال 24 ساعة.
4. المكافأة تُسلَّم فورًا في D1 وتُعاد في الاستجابة.
5. انتهاء النافذة → تنظيف تلقائي (DO tick + عند القراءة).
