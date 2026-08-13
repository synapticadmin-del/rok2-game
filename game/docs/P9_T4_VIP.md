# P9-T4: نظام VIP الكامل

## النطاق

توسيع نظام VIP من سبعة مستويات إلى **15 مستوى كاملًا** مع ثلاث ركائز جديدة: نقاط VIP يومية بمكافأة اتصال وسقف يومي، بافات حرجة على المستوى 15، ومتجر VIP بأسعار مخفّضة حسب المستوى.

## الملفات المعدّلة أو المضافة

| الملف | الوصف |
| --- | --- |
| `src/data/shop.json` | توسيع `vip_tiers` إلى 16 إدخالًا (المستوى 0–15) مع حقول جديدة `research_speed_mult` و`heal_speed_mult` و`gather_mult` و`extra_build_queue`، وإضافة `constants.vip_daily` و`constants.vip_store` |
| `src/do/sim/shop.ts` | دوال VIP اليومية النقية (`applyVipDailyPoints`, `markVipConnected`, `vipDailyFullGrant`) ودوال متجر VIP (`vipStorePrice`, `vipStoreHallRequired`, `vipStoreDiscountForLevel`) |
| `src/env.ts` | توسيع `VipRow` بحقلي `last_daily_points_day` و`last_login_day` |
| `migrations/0013_vip_daily.sql` | مهاجرة D1: `ALTER TABLE player_vip ADD COLUMN last_daily_points_day INTEGER DEFAULT -1, last_login_day INTEGER DEFAULT -1` |
| `src/http/router.ts` | تحديث `getOrCreateVip` و`vip/status` (منح يومي + حالة المتجر)، `claimVipDailyPoints`، طابور البناء الثاني VIP6، بافات البحث/التدريب/الشفاء، خصومات متجر VIP، وقفل CH5 |
| `src/do/KingdomShard.ts` | مزامنة مستوى VIP من الراوتر عبر header موثوق `x-rok2-vip-level`، طابور البناء الثاني في `queue/add`، باف جمع VIP في tick (مراكز + عقد) |
| `scripts/vip_offline_test.mjs` | حارس جودة: 26 فحصًا (15 مستوى متصاعد، منح يومي، بافات، متجر، خصومات) |
| `package.json` | job جديد `test:p9-t4-vip` مضاف إلى `check` chain |

## تفاصيل التنفيذ

### 1. المستويات الخمسة عشر

| المستوى | النقاط المطلوبة | إنتاج | بناء | بحث | تدريب | شفاء | جمع | طابور ثانٍ | تسريع مجاني/يوم |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | لا | 0 |
| 1 | 100 | 1.02 | 1.02 | 1.00 | 1.00 | 1.00 | 1.00 | لا | 60s |
| 2 | 300 | 1.04 | 1.04 | 1.00 | 1.02 | 1.00 | 1.00 | لا | 180s |
| 3 | 700 | 1.06 | 1.06 | 1.05 | 1.04 | 1.00 | 1.00 | لا | 300s |
| 4 | 1500 | 1.08 | 1.08 | 1.08 | 1.06 | 1.00 | 1.00 | لا | 600s |
| 5 | 3000 | 1.10 | 1.10 | 1.10 | 1.08 | 1.05 | 1.05 | لا | 900s |
| 6 | 6000 | 1.12 | 1.12 | 1.12 | 1.10 | 1.08 | 1.08 | **نعم** | 1200s |
| 7 | 10000 | 1.13 | 1.13 | 1.13 | 1.11 | 1.10 | 1.10 | نعم | 1500s |
| 8 | 15000 | 1.14 | 1.14 | 1.14 | 1.12 | 1.15 | 1.12 | نعم | 1800s |
| 9 | 21000 | 1.15 | 1.15 | 1.15 | 1.13 | 1.20 | 1.14 | نعم | 2400s |
| 10 | 28000 | 1.16 | 1.16 | 1.16 | 1.14 | 1.25 | 1.16 | نعم | 3000s |
| 11 | 36000 | 1.17 | 1.17 | 1.17 | 1.15 | 1.30 | 1.18 | نعم | 3600s |
| 12 | 45000 | 1.18 | 1.18 | 1.18 | 1.16 | 1.35 | 1.20 | نعم | 4200s |
| 13 | 56000 | 1.19 | 1.19 | 1.19 | 1.18 | 1.40 | 1.24 | نعم | 4800s |
| 14 | 70000 | 1.195 | 1.195 | 1.195 | 1.19 | 1.45 | 1.27 | نعم | 5400s |
| 15 | 99000 | 1.20 | 1.20 | 1.20 | 1.20 | **1.50** | **1.30** | نعم | 7200s |

البافات تتصاعد بشكل صارم (monotonic) مع المستوى وتُقرأ كلها من JSON ولا تُعرَّف hard-coded في الكود.

### 2. نقاط VIP اليومية (P9-T4)

> 40 نقطة/يوم + 20 نقطة لمن يسجّل نشاطًا في اليوم ذاته، بسقف تراكم يومي 200 نقطة (من `constants.vip_daily`).

تُمنح النقاط تلقائيًا عند قراءة `GET /v1/vip/status` (idempotent — مرة واحدة يوميًا، `last_daily_points_day`). نقاط الشراء من شراء gems تبقى كما هي (`vip_points_per_gem = 1`).

### 3. بافات حرجة

يُطبَّق التوزيع الجديد في المواضع التالية، جميعها server-authoritative في الراوتر (قناة موثوقة إلى الـ Durable Object):

- **VIP 6+**: طابور بناء ثانٍ دائم (`extra_build_queue`) — يتحقق الراوتر قبل البدء ويتحقق `queue/add` في `KingdomShard` (فحص `x-rok2-vip-level`).
- **البحث**: `research_speed_mult` تراكمي مع باف `research_speed` البحثي، في `POST /v1/city/research`.
- **التدريب**: `train_speed_mult` تراكمي مع `training_speed` البحثي، في `POST /v1/city/train`.
- **الشفاء**: `heal_speed_mult` يطبَّق على `healDurationSec` في `POST /v1/city/heal` (حد أدنى ثانية واحدة).
- **الجمع**: `gather_mult` يطبَّق في tick العالمي بعد باف الأرض الإقليمية + لقب التحالف، لكل من مراكز الموارد وعقد الموارد.
- **البناء**: `build_speed_mult` موجود مسبقًا ويُستخدم في `POST /v1/city/upgrade`.

### 4. مزامنة مستوى VIP إلى الـ Shard (قرار معماري)

الـ Durable Object لا يصل إلى D1 مباشرة بمرحلة tick، لذا يقرأ الراوتر مستوى VIP من D1 ويوقّعه داخليًا عبر header `x-rok2-vip-level` مع رأس الهوية `x-rok2-player`. يثبت `KingdomShard` المستوى في `playerVipLevels` map من ذاكرة الشارد (يُحدَّث مع كل طلب موقّع — مقبول لأن مستوى VIP يتغير نادرًا: شراء أو منح يومي). الدالة `syncVipLevel` ترفض أي قيمة خارج النطاق 0–15 أو أكبر من أعلى مستوى في `shop.json` (دفاع ضد تعديل headers خارجي).

### 5. متجر VIP

يفتح عند **قاعة مستوى 5 (CH5)**، وخصومات متصاعدة حسب المستوى من `constants.vip_store.discount_by_tier` (5% عند L1 حتى 25% عند L15)، تُطبَّق في `POST /v1/shop/buy`. الرد على `vip/status` يعرض `vip_store_open` و`vip_store_discount` الحالي.

### 6. ملاحظات أمان وتوافق

- مهاجرة D1 0013 اختيارية: الراوتر يعمل مع `catch` على أعمدة غائبة (حالة جداول قديمة غير مرحّلة).
- كل الثوابت من `data/shop.json` — لا hard-coded في الكود.
- حارس الجودة `vip_offline_test.mjs` يعيد تنفيذ المنطق النقي محليًا (26 فحصًا) ولا يستورد TypeScript.

## التحقق

- `npm run test:p9-t4-vip` → 26/26، ALL PASSED.
- `npm run check` كاملة → EXIT=0 (تشمل E2E رحلة لاعبين، PIE/Android، كل حراس P7–P9).
- TypeScript typecheck → بدون أخطاء.
