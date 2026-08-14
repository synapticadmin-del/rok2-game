# P7-T15 — تشغيل الممالك وإدارة الحوادث (Ops Runbook)

هذا المستند يرافق بند **P7-T15** من `PLAN.md`. يغطي مراقبة صحة الـ Durable Object الخاص بالمملكة (`KingdomShard`)، ومؤشرات أخطاء أوامر اللاعبين، وزمن دورة `tick`، والطوابير المتعثرة، مع إجراءات الاستجابة لكل تنبيه.

## 1. البنية

| المكوّن | المسار | الدور |
|---|---|---|
| ثوابت المؤشرات (عتبات قابلة للضبط) | `game/backend/src/data/ops.json` | نوافذ زمن التنبيه والعتبات — تُقرأ بـ `OPS_CONSTANTS` |
| جمع المؤشرات | `game/backend/src/do/KingdomShard.ts` | `recordCommandError()` + `lastTickMs` + مدة `tick` + `opsSnapshot()` |
| نقطة قراءة داخلية | `KingdomShard.fetch` → `/ops` | لقطه snapshot + آخر 10 انتهاكات anti-cheat (لا تُعرَّض خارجيًا إلا عبر الإداري) |
| نقطة إدارية محمية | `router.ts` → `GET /v1/admin/ops` | `requireAdmin` (مفتاح `x-admin-key`) |
| حارس الجودة | `game/client-unreal/scripts/verify_p7_t15_ops.mjs` | فحص بنيوي: وجود الثوابت والـ endpoints وعدم وجود عتبات hard-coded |
| الاختبار | `npm run test:p7-t15-ops-monitoring` | مدرج في `npm run check` |

### الثوابت الافتراضية (`ops.json`)

| الثابت | القيمة الافتراضية | المعنى |
|---|---|---|
| `enabled` | `true` | تفعيل/تعطيل تسجيل أخطاء الأوامر ككل |
| `command_error_window_ms` | `3600000` | نافذة الساعة المنزلقة لحصر أخطاء كل رمز خطأ |
| `tick_stale_threshold_ms` | `30000` | متى يعتبر tick متعثرًا (30 ثانية بدون تحديث) |
| `queue_stuck_threshold` | `40` | عدد الطوابير النشطة الذي يُعتبر ضغطًا |
| `command_alert_threshold` | `5` | عدد التكرارات داخل النافذة الذي يولّد تنبيه `command_error_X` |
| `error_log_limit` | `200` | سقف سجل أخطاء الأوامر في ذاكرة الـ shard (اُستبعد الأقدم) |

## 2. المؤشرات التي يرصدها `opsSnapshot()`

1. **`seasonDay` / `lastTickMs` / `tickStaleMs`** — يوم الموسم وزمن آخر دورة tick ناجحة (محدَّث بعد INSERT `world_meta` في نهاية كل tick).
2. **`commandErrors`** — أخطاء أوامر اللاعبين خلال نافذة الساعة المنزلقة، مجمعة بالرمز، مرتبة تنازليًا، أعلى 10. تشمل: `city_not_found`, `bad_flag_coords`, `bad_structure_coords`, `unknown_structure_kind`, `alliance_structure_cap_reached`, `structure_kind_cap_reached`, `structure_too_close`, `structure_requires_alliance_territory`, `not_your_alliance`, `invalid_seconds`, `queue_not_found`, `not_your_queue`, `player_identity_mismatch`, `alliance_and_builder_required`.
3. **`lastTickDurationMs` / `avgTickDurationMs` / `maxTickDurationMs` / `tickCount`** — زمن تنفيذ آخر دورة، ومتوسطها، وأطولها، وعدد الدورات المقاسة منذ إنشاء الشارد. تقيس هذه الحقول جسم الدورة، بينما يقيس `tickStaleMs` الزمن منذ اكتمال آخر دورة.
4. **`queuesTotal` / `queuesByKind`** — عدد الطوابير النشطة (build/train/heal/research) موزعة بالنوع.
5. **`marchesActive`** — المسيرات المتحركة/الراجعة النشطة.
6. **`violationsTotal` + آخر 10 انتهاكات anti-cheat** — للفحص الإداري.
7. **`alerts`** — قائمة التنبيهات المشتقة (انظر القسم 3).

لا تُسجل أخطاء البنية الإدارية (`auth_required`, `admin_unauthorized`, `unknown_action`, `not_found`) كأخطاء أوامر لاعب — فهي ليست مؤشرًا على سلوك اللاعبين أو على خلل في منطق اللعبة.

## 3. التنبيهات ومعانيها وإجراءات الاستجابة

### 3.1 `tick_stale`

**المعنى:** لم يُحدَّث `lastTickMs` منذ أكثر من `tick_stale_threshold_ms` (افتراضيًا 30 ثانية). يعني أن دورة المحاكاة اليومية لا تعمل: لا تُنتج المسارات، لا تكتمل الطوابير، لا يُعالج القتال، ولا يُحفظ `world_meta`.

**الأسباب المحتملة:**
- `alarm` مفقود أو لم يعد يعمل بعد انقطاع طويل.
- Exception غير ممسوك داخل `tick()` (تحقق من سجلات `worker` في Cloudflare Dashboard).
- ضغط CPU على الـ isolate أدى إلى إطالة الدورة الواحدة.

**الإجراء:**
1. افتح `GET /v1/admin/ops` وتحقق من `tickStaleMs`. إذا كانت موجبة وكبيرة فالمشكلة قائمة.
2. استدعِ `POST /v1/admin/tick` مع `{ force: true }` لإجبار دورة يدوية فورية.
3. افحص سجلات الـ worker للـ exception؛ إن وُجد أصلحه وادفعه (نمط AGENTS commit).
4. إن لم يستجب الـ shard، أعِد نشر الـ worker عبر `wrangler deploy` ثم أعد استدعاء `/v1/admin/tick`.

### 3.2 مؤشرات زمن `tick`

**المعنى:** `lastTickDurationMs` هو زمن آخر دورة مكتملة، و`avgTickDurationMs` متوسط الدورات، و`maxTickDurationMs` أطول دورة، و`tickCount` عدد العينات. ارتفاع مدة tick مع `tickStaleMs` منخفض يعني ضغطاً داخل الدورة؛ ارتفاعهما معاً يعني تعثراً في الجدولة أو فشل الدورة.

**الإجراء:** عند ظهور بطء، سجّل الحقول الأربعة ثم قارنها بـ`queuesTotal` و`marchesActive`. راجع عمليات D1 داخل معالجة الطوابير والمسيرات إذا ارتفع المتوسط أو الحد الأقصى بصورة مستمرة.

### 3.3 `queue_pressure`

**المعنى:** عدد الطوابير النشطة (`build`/`train`/`research`/`heal`) تجاوز `queue_stuck_threshold` (افتراضيًا 40).

**الأسباب المحتملة:**
- تسارع غير طبيعي في أفعال بناء/تدريب اللاعبين (حدث في اللعبة أو استغلال).
- tick متعثر لا يكمل الطوابير المنتهية (`state=completed` لا يُطبق).

**الإجراء:**
1. افحص `queuesByKind` — إن كان نوع واحد مهيمنًا (مثل `train` ضخم) فهذا نمط استغلال محتمل.
2. قارن مع `tick_stale`: إن كان التنبيهان معًا فالسبب غالبًا tick متعثر؛ عالج `tick_stale` أولًا.
3. إن كان الأمر نمط استغلال، راجع سجلات anti-cheat (`/v1/admin/anticheat`) واعمل على حد معدل إضافي أو مراجعة المنطق.

### 3.4 `command_error_<code>`

**المعنى:** رمز خطأ محدد تكرر ≥ `command_alert_threshold` (افتراضيًا 5) مرات خلال نافذة `command_error_window_ms` (افتراضيًا ساعة).

**التشخيص حسب الرمز:**

| الرمز | الدلالة | الإجراء |
|---|---|---|
| `player_identity_mismatch` | رأس `x-rok2-player` لا يطابق `playerId` المُطالب — محاولة انتحال هوية | راجع anti-cheat؛ قد يحتاج حد معدل إضافي على الراوتر |
| `not_your_alliance` / `not_your_queue` | محاولة تعديل موارد تحالف/طابور لا يملكها اللاعب | طبيعية بكميات قليلة؛ تكرار كثيف = استطلاع IDs — فعّل rate limit |
| `bad_flag_coords` / `bad_structure_coords` | إحداثيات خارج الخريطة أو NaN | تحقق أن العميل يرسل إحداثيات الخريطة الجديدة الصحيحة (بعد أي تحديث لأبعاد الخريطة) |
| `unknown_structure_kind` | نوع بناء تحالف غير معروف في الكتالوج | غالبًا عميل قديم يقرأ كتالوجًا جديدًا — تحقق من توافق إصدار الكتالوج |
| `structure_too_close` / `structure_requires_alliance_territory` | مخالفات مواضع | غالبًا طبيعية (تجارب لاعبين)؛ كثرة مفاجئة تستحق مراجعة UX |
| `alliance_structure_cap_reached` / `structure_kind_cap_reached` | السقوف Reached | طبيعية |
| `invalid_seconds` | قيمة تسريع غير صالحة — مصدر موثوق مُمرَّر بشكل خاطئ | تحقق من `/v1/shop/use-speedup` والعميل |
| `city_not_found` / `queue_not_found` | مرجعيات مفقودة | عادة عميل قديم؛ إذا انتشرت فراجع هجرة الجداول |

**الإجراء العام:** لا يتطلب أي تنبيه تدخلًا فوريًا بذاته؛ التنبيه مؤشر استكشافي. القرار يكون بعد فحص التوزيع والنمط الزمني.

## 5. التشغيل اليومي

1. نقطة الوصول الوحيدة الخارجية: `GET /v1/admin/ops` بمفتاح `x-admin-key` الإداري (عبر `requireAdmin`).
2. `/ops` داخل الـ Durable Object قراءة داخلية فقط — الراوتر العام لا يعرّضها.
3. جميع العتبات في `game/backend/src/data/ops.json` — لتغييرها عدّل الملف ثم أعد النشر؛ لا توجد عتبات hard-coded في الكود (يتحقق منها الحارس).
4. سجل أخطاء الأوامر محصور في الذاكرة بحد `error_log_limit` (200) ثم يستبعد الأقدم؛ عند الحاجة لسجل دائم أضف INSERT في جدول D1 ضمن `recordCommandError()`.

## 6. الفحص الآلي

```bash
npm run test:p7-t15-ops-monitoring   # داخل game/backend — عبر npm run check
```

الحارس يفحص: وجود `ops.json` بكل الثوابت، استيراد `opsData` و`OPS_CONSTANTS` في KingdomShard، وجود `opsSnapshot()` و`recordCommandError()` و`/ops` و`/v1/admin/ops` و`requireAdmin`، وتحديث `lastTickMs` وقياس مدة tick، وعدم وجود عتبات hard-coded (`3600000`، `30000`، `60*60*1000`).
