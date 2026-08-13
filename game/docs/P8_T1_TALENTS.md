# P8-T1: نظام مواهب القادة (Talents)

**البند:** `P8-T1` من PLAN.md — عمق أنظمة اللعب (المرحلة 8)
**الحالة:** مكتمل ✅
**البروتوكول:** كود + حارس + وثيقة + بند PLAN + سطر سجل + test job

## 1. الملخص

نظام مواهب القادة يمنح كل قائد **شجرتي مواهب** يُنفق فيهما نقاط تُمنح تلقائيًا بمستوى القائد:

| الشجرة | الفروع | الغرض |
|--------|--------|-------|
| **troop_type** (نوع القوات) | infantry / cavalry / archer / siege | تعزيز القوات من نوع الفرع المحدد فقط (هجوم/دفاع/صحة/سرعة مسير/سرعة تدريب) |
| **role** (الدور) | attack / defense / support | تعزيز دور القائد العام (هجوم، دفاع، دعم اقتصادي) |

القائد المرافق لأي مسيرة يمنح قواتها بافات المواهب في القتال (مماثلة لآلية مهارات P2-T1 وأبحاث P2-T3).

## 2. الثوابت — من JSON لا hard-coded

كل القيم في `game/backend/src/data/talents.json` وتُقرأ عبر `TALENT_CONSTANTS` في `sim/talents.ts`:

| الثابت | القيمة | المعنى |
|--------|--------|--------|
| `talent_points_per_level` | 1 | نقطة موهبة لكل مستوى قائد (من 1 حتى 60) |
| `points_cap_rarity` | common 40 / elite 60 / epic 80 / legendary 100 | سقف إجمالي النقاط وفق ندرة القائد |
| `max_points_per_node` | 5 | حد افتراضي لأي عقدة |
| `reset_refund_ratio` | 0.8 | نسبة استرجاع النقاط عند إعادة الضبط |
| `reset_cooldown_ms` | 60000 | مهلة إعادة الضبط |
| `talent_buff_stat_cap` | 0.3 | سقف إجمالي باف أي stat واحد |

الشجرة تحتوي 32 عقدة (20 في troop_type + 12 في role)، لكل عقدة `id`, `branch`, `stat`, `per_point`, `max_points`.

## 3. نقاط الربط في الكود

| الملف | الإضافة |
|-------|---------|
| `src/data/talents.json` | ملف البيانات الجديد (شجرتان + ثوابت) |
| `src/do/sim/talents.ts` | منطق المواهب: فهارس العقد، حساب النقاط المكتسبة/السقف، التحقق من التخصيص، حساب البافات، إعادة الضبط |
| `src/do/sim/commanders.ts` | `talentAllocations?: Record<string,number>` في `CommanderInstance` + `commanderRarity()` |
| `src/do/sim/combat.ts` | `resolveCombat` اكتسب معاملين `attackerTalentAttackMod`/`defenderTalentAttackMod` + باف `counter_damage` على مثلث التفوق (سقف 0.15) |
| `src/do/KingdomShard.ts` | `fetchMarchCommander` يجلب المستوى والمواهب معًا من D1 + تمرير `talentAttackMod` في مواضع القتال الأربعة (ممر/عرش/هدف رئيسي/بربري) |
| `src/http/router.ts` | `GET /v1/meta/talents` + المواهب داخل `meta/all` + توسيع `commanderJson` بإحصاءات المواهب + `POST /v1/commander/talent/allocate` + `POST /v1/commander/talent/reset` + `march_commanders.talents_json` عند التعيين |
| `migrations/0010_talents.sql` | عمود `talents_json` في `player_commanders` و`march_commanders` (افتراضي `'{}'`) |

## 4. تدفق التخصيص

1. اللاعب يربح `level × 1` نقطة (حدّ أقصى وفق الندرة).
2. `POST /v1/commander/talent/allocate {commanderId, nodeId, points}` يتحقق: وجود العقدة، `points ≤ max_points`، عدم تجاوز النقاط المتاحة — ثم يحفظ `talents_json`.
3. `POST /v1/commander/talent/reset {commanderId}` يمسح التخصيص ويعيد `reset_refund_ratio × النقاط المصروفة` كـ نقاط متاحة (تُرَد داخليًا كإعادة توزيع في التصميم الحالي؛ يمكن تطوير مُردّد خارجي في P8-T2/T6).
4. عند تعيين القائد على مسيرة تُنسخ `talents_json` إلى `march_commanders`.

## 5. التأثير في القتال

- **troop_attack** (من أي عقدة) تُجمع وتُضاف إلى `aCommMod`/`dCommMod` مثل المهارات والأبحاث، مع سقف `talent_buff_stat_cap` لكل stat.
- **counter_damage** يضيف ضررًا تفوقيًا فوق مثلث التفوق الافتراضي (سقف إجمالي 0.15 على الطرفين).
- باقي الإحصاءات (march_speed, training_speed, gathering_speed, troop_defense, troop_health, siege_damage, ...) متاحة للاستهلاك في مساراتها الخاصة (مسيرات/تدريب/مستودعات) دون تغيير منطق القتال الحالي.
- القائد بدون مواهب: `talentAttackMod(null) === 0` — توافقية كاملة مع كل القتال القائم.

## 6. بوابة القبول (الحارس)

`scripts/talents_offline_test.mjs` يتحقق من 64 عقدًا تشمل: بنية JSON (شجرتان، 32 عقدة، فهارس)، الثوابت من JSON، نقاط المستوى والسقوف، التحقق من التخصيص (قبول/رفض)، جمع البافات والسقف، باف القتال والتوافقية العكسية، آلية إعادة الضبط، مخطط الترحيل، وتغطية ندرة كل القادة الـ 18.

يُشغَّل أيضًا تلقائيًا ضمن `npm run test:offline` وأُدرج في سلسلة `npm run check` باسم `test:p8-t1-talents` (قبل `test:pie-android-acceptance`).

**نتيجة التحقق: 64/64 عقدًا ناجحة — EXIT=0.**

## 7. ما يلي (خارج نطاق هذا البند)

- شاشة المواهب في العميل C++ (UE5): بند `P8-T7` مسار العميل العام.
- استهلاك إحصاءات غير هجومية (march_speed إلخ) في منطق المسير والتدريب — تُطبَّق عند المرور على أنظمتها القائمة.
- مُردّد خارجي (عملة ذهبية) لإعادة الضبط — مرشح لـ P8-T2/T6.

## 8. سجل القبول

| العقد | النتيجة |
|-------|---------|
| بنية talents.json (شجرتان، 32 عقدة) | ✅ |
| ثوابت من JSON (لا hard-coded) | ✅ |
| نقاط مستوى + سقف ندرة | ✅ |
| تحقق تخصيص (نجاح/فشل) | ✅ |
| بافات قتال + توافقية عكسية | ✅ |
| إعادة ضبط 80% | ✅ |
| مخطط D1 (0010) | ✅ |
| npm run check كامل | ✅ EXIT=0 |
