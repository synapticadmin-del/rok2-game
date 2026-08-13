# P9-T3: متجر التحالف والألقاب (Alliance Shop & Titles)

**التاريخ:** 2026-08-13 · **الحالة:** مكتمل (31 فحصًا + حارس جودة + E2E ضمن `npm run check`) · **commit:** P9-T3

## الملخص التنفيذي

يبني هذا البند **نظامًا اقتصاديًا وتحفيزيًا داخل التحالف**: رصيد تحالف مشترك (Alliance Credits) يُكسبه الأعضاء تلقائيًا من نشاطاتهم اليومية — كل مساعدة لطابور عضو تكسب **25 رصيدًا**، وكل مطالبة جائزة مهمة (quest claim) تكسب **10 أرصدة** — بحد يومي للكسب (1000) وحد أقصى للرصيد (20000) وحماية rate-limit للشراء (30 عملية/ساعة). يُنفق الرصيد في **متجر التحالف** لشراء عناصر catalog، ويُكسب القائد (R5) القدرة على منح **ألقاب تحالف (Titles)** لأعضائه — خمسة ألقاب لكل تحالف (القائد/الاستراتيجي/الحارس/الجامع/مهندس الحصار)، كل لقب يحمل بافات server-authoritative (هجوم/دفاع/HP/جمع/حصار حتى 25%) تُطبَّق فقط على مسيرات حامل اللقب نفسه.

## لماذا هذا البند

الأنظمة الجماعية وحدها لا تكفي لربط الأعضاء النشطين بعضوية التحالف: الألقاب تمنح الاعتراف الاجتماعي القابل للقياس (رؤية اسم حامل اللقب وقواته المعززة في الخريطة والتقارير)، والرصيد المشترك يجعل نشاط كل عضو — ولو بسيطًا — مساهمة ملموسة في خزينة الجماعة. هذا يحل مشكلة الاحتفاظ بالأعضاء في منتصف الموسم: الدافع اليومي ليس "بناء مدينتي" فقط بل "تحالفي يكسب مني".

## المكونات المنفذة

### 1. البيانات (`src/data/alliance_shop.json`)

كل الثوابت مقروءة من JSON لا hard-coded:

| الثابت | القيمة | الدلالة |
|---|---|---|
| `earn.help_credit.per_help` | 25 | رصيد لكل مساعدة طابور عضو |
| `earn.gift_claims.per_claim` | 10 | رصيد لكل مطالبة جائزة مهمة |
| `earn.daily_cap.amount` | 1000 | حد الكسب اليومي لكل تحالف |
| `earn.balance_cap.amount` | 20000 | حد الرصيد الأقصى |
| `rate_limits.window_seconds` | 3600 | نافذة rate-limit الشراء |
| `rate_limits.max_purchases_per_hour` | 30 | حد الشراء في الساعة |
| `titles.max_granted_per_alliance` | 5 | سقف الألقاب الممنوحة |
| `titles.max_holders_per_title` | 1 | لقب واحد لكل نوع |

الكاتالوج يحتوي عناصر قابلة للشراء (كل عنصر: `id`, `name`, `price`, `max_per_alliance`, `grant{type, amount}`)، والألقاب الخمسة بهجمات/دفاع/HP/جمع/حصار مقيدة بسقف 0.25 للباف الواحد.

### 2. المنطق النقي (`src/do/sim/alliance_shop.ts`)

| الدالة | السلوك |
|---|---|
| `applyHelpCredit(state, seasonDay)` | كسب من مساعدة — يفرض daily_cap وbalance_cap ويُعيد ضبط العداد اليومي بتغيير اليوم |
| `applyGiftClaimCredit(state, seasonDay)` | كسب من مطالبة هدية |
| `validatePurchase(state, itemId)` | {ok, reason}: سعر/سقف عنصر/عنصر مجهول |
| `purchase(state, itemId)` | خصم الرصيد وزيادة الكمية المشتراة |
| `validateTitleGrant(state, titleId)` | لقب موجود وسقف الألقاب غير ممتلئ |
| `grantTitle(state, titleId, playerId)` / `revokeTitle(...)` / `revokeTitlesForPlayer(...)` | منح/سحب/تنظيف ألقاب |
| `titleBuffsForPlayer(state, playerId)` | تجميع بافات الألقاب التي يحملها اللاعب (سلطوي — playerId فقط) |

### 3. Durable Object (`src/do/KingdomShard.ts`)

- **هجرة `ver<14`**: جدول `alliance_shop` (نص JSON للحالة الكاملة: balance/dailyEarned/items/titles) — نفس نمط `alliance_tech`.
- `loadAllianceShop` في `loadState` + `persistAllianceShop` (INSERT OR REPLACE) عند كل تغيير.
- **`allianceTitleBuffs(playerId, allianceId)`** و`marchTitleMod(ownerId, allianceId, kind)` — بافات سلطوية (سقف 0.25 لكل باف) تُطبَّق في خمسة مواضع قتال (ممر/عرش/هدف رئيسي/موقع مقدس/بربر) لكل مسيرة حامل اللقب من تحالفها، وفي مسارَي الجمع (ratePerSec لمركز الموارد + دفعة tick لمركز الموارد وnodes).
- **`allianceShopStateFor(allianceId)`**: الحالة العامة للتحالف (رصيد + عناصر + حَمَلة الألقاب بدون playerId حساس غير الأعضاء) — تُصدَر في snapshot/worldDelta.
- **إدارة الألقاب**: في `grant-title` الداخلي يضبط `city.titleId` على بطاقة المدينة ليصل للعميل عبر `city_upsert`؛ في `set-alliance` (مغادرة/طرد) مع `previousAllianceId` يُسحب اللقب من حالة التحالف ويُبَثَّ `alliance_title_revoked` وتُمسح `titleId` من بطاقة المدينة.
- **`title_definitions` و`shop_state` endpoints داخلية** للقراءة العامة.

### 4. الراوتر (`src/http/router.ts`)

| المسار | الوصول | السلوك |
|---|---|---|
| `GET /v1/alliance/shop-state` | عضو تحالف | رصيد + عناصر + تعريفات الألقاب والحَمَلة |
| `POST /v1/alliance/shop/purchase` | عضو تحالف | شراء عنصر مع rate-limit `alliance_shop_purchase` |
| `POST /v1/alliance/shop/grant-title` | قائد R5 فقط | منح لقب لعضو |
| (داخلي) `/alliance-shop-earn-help` | يستدعيه `alliance/help` | كسب 25 رصيدًا لكل مساعدة |
| (داخلي) `/alliance-shop-earn-gift` | يستدعيه `quests/claim` | كسب 10 أرصدة لكل مطالبة جائزة |

### 5. الجودة

- **الحارس** `scripts/alliance_shop_offline_test.mjs`: 31 فحصًا يغطي JSON والمنطق المحلي حرفيًا — ALL PASSED، مضاف إلى `check` chain بعد `test:p9-t2-alliance-territory`.
- **rate-limit** `alliance_shop_purchase` في `anticheat.json` (3600s / 30 عملية).
- **tsc** نظيف و`npm run check` كامل (EXIT=0) بما فيه E2E رحلة اللاعب.

## قواعد gameplay السلطوية

يُفرض كل شيء server-side ولا ثقة في العميل: الألقاب تُمنح من R5 فقط، والباڤات تُحسب من قائمة `titles` في حالة DO لا من طلبات العميل، والمغادرة/الطرد يسحب اللقب فورًا حتى لو ظل العميل يعرضه. الرصيد يُكسب من فعلين واقعيين (مساعدة + مطالبة جائزة) ولا يُضاف من أي endpoint آخر.
