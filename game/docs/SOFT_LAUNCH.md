# Soft Launch — خطة الإطلاق المحدود (P3-T5)

> **الهدف:** إطلاق اللعبة لمملكة واحدة أو اثنتين فقط (لا نشر عام)، وقياس **retention** يومياً
> حتى نقرر بثقة: هل نجتاز بوابة المرحلة 3 (موسم ألفا كامل + مؤشرات retention فوق العتبات)؟
>
> كل الإعدادات تُقرأ من `data/softlaunch.json` — لا قيم ثابتة في الكود.

---

## 1. المكوّنات (أين يعيش كل شيء)

| المكوّن | الملف | الدور |
|---------|-------|-------|
| إعدادات الإطلاق | `data/softlaunch.json` | قائمة الممالك (مفتوحة/مغلقة + سعة) + عتبات retention + بوابة النجاح |
| منطق القراءة | `game/backend/src/do/sim/retention.ts` | `isKingdomOpen` / `kingdomCapacity` / `cohortDayOf` / `pct` — يقرأ JSON فقط |
| بوابة الانضمام | `game/backend/src/http/router.ts` → `POST /v1/city/init` | رفض `kingdom_not_open_for_launch` / `kingdom_full` |
| تتبع النشاط | `game/backend/src/lib/context.ts` | upsert يومي في `player_activity` + `last_seen_at` على accounts (تلقائي مع كل طلب مُصادق) |
| قاعدة البيانات | `game/backend/migrations/0006_retention.sql` | جدول `player_activity(day, player_id)` + `accounts.last_seen_at` |
| القياس | `GET /v1/admin/retention` (إداري) + `GET /v1/launch/status` (عام) | DAU + رجوع cohorts + حالة الممالك |

---

## 2. التهيئة: فتح/إغلاق ممالك

عدّل `data/softlaunch.json` ثم اعمل `wrangler deploy` (أو `git push` إن كان CI ينشر تلقائياً):

```json
"kingdoms": [
  { "id": "kingdom-1", "name": "المملكة الأولى (ألفا)", "open": true,  "max_players": 500 },
  { "id": "kingdom-2", "name": "المملكة الثانية (احتياطية)", "open": false, "max_players": 500 }
]
```

- **`open: true`** = المملكة تقبل لاعبين جدد في `POST /v1/city/init`.
- **`max_players`** = سقف اللاعبين في قاعدة D1 لهذه النسخة المنشورة؛ عند بلوغه يرفض السيرفر `kingdom_full`.
- **مملكة ثانية:** انشر worker إضافي بـ `KINGDOM_ID=kingdom-2` في `wrangler.toml` الخاص به، واجعل `open: true` له عند الحاجة. الـ Durable Object يُشتق من `KINGDOM_ID` تلقائياً (`idFromName`)، فكل مملكة معزولة بـ shard مستقل.

> التحقق السريع بعد النشر: `GET /v1/launch/status` يعرض الممالك الحالية والإشغال.

---

## 3. النشر (أول مرة بعد P3-T4 + P3-T5)

الـ backend على main يتضمن migration جديدين يجب تطبيقهما على قاعدة D1 الحية:

```bash
cd game/backend

# 1) طبّق migrations على D1 البعيد (shop + retention)
wrangler d1 migrations apply rok2-db --remote

# 2) انشر الـ worker
wrangler deploy
```

> **ملاحظة:** `migrations/0005_shop.sql` يضيف عمود `gems` إلى `cities` وجدولَي المتجر/VIP،
> و`0006_retention.sql` يضيف جدول `player_activity` وعمود `accounts.last_seen_at`.
> الاثنان idempotent (`CREATE TABLE IF NOT EXISTS`) لكن `ALTER TABLE` يفشل إذا العمود موجود —
> طبّقهما مرة واحدة فقط على كل بيئة.

### التحقق من نجاح النشر

```bash
# صحة عامة
curl https://<worker-url>/v1/health

# حالة الإطلاق (يجب أن تُظهر kingdom-1 مفتوحة)
curl https://<worker-url>/v1/launch/status

# endpoint retention محمي بالمفتاح الإداري
curl -H "x-admin-key: $ADMIN_KEY" https://<worker-url>/v1/admin/retention
```

---

## 4. دعوة اللاعبين

1. وزّع رابط العميل (UE5 PC/Android) على مجموعة ألفا الصغيرة فقط (عشرات اللاعبين، لا آلاف).
2. اللاعب يدخل كضيف (`POST /v1/auth/guest`) ثم ينشئ مدينته (`POST /v1/city/init`).
   البوابة ترفض تلقائياً إن المملكة مغلقة أو ممتلئة — لا حاجة لأي منطق دعوة إضافي في هذه المرحلة.
3. راقب الإشغال عبر `GET /v1/launch/status` (حقل `players` مقابل `max_players`).

---

## 5. قياس retention (يومياً)

التتبع تلقائي: كل طلب مُصادق يحدّث `player_activity(day, player_id)` و`accounts.last_seen_at`.

### الاستعلام

```bash
curl -H "x-admin-key: $ADMIN_KEY" https://<worker-url>/v1/admin/retention
```

### شكل الاستجابة

```json
{
  "ok": true,
  "today_utc_day": 20140,
  "dau": 87,
  "buckets": [1, 3, 7, 14, 30],
  "targets": { "d1_min_pct": 40, "d7_min_pct": 15, "d30_min_pct": 5 },
  "cohorts": [
    { "cohort_day": 20139, "size": 42, "retention": { "d1": 52.4 } },
    { "cohort_day": 20133, "size": 38, "retention": { "d1": 44.7, "d7": 18.4 } }
  ],
  "tracked_players": 123
}
```

- **`dau`** = عدد اللاعبين الفريدين النشطين خلال آخر `active_threshold_days` (افتراضياً يوم واحد).
- **`cohorts[i].retention.dN`** = نسبة لاعبي يوم الإنشاء `cohort_day` الذين عادوا في اليوم `cohort_day + N` أو بعده.
  يظهر المفتاح `dN` فقط عندما يكون اليوم المطلوب قد حلّ فعلاً (`cohort_day + N <= today`).

### قراءة النتائج مقابل العتبات

| المؤشر | العتبة (من `retention.targets`) | معناها |
|--------|----------------------------------|--------|
| D1 retention | ≥ 40% | هل يعود اللاعب في اليوم التالي لأول جلسة؟ |
| D7 retention | ≥ 15% | هل يلتزم اللاعب بعد أسبوع؟ |
| D30 retention | ≥ 5% | هل يبقى اللاعب بعد شهر (بداية الالتصاق طويل الأمد)؟ |

> cohort يحتاج وقتاً ليكتمل: لا تحكم على D7 قبل مرور 7 أيام على يوم إنشائه.
> اقرأ فقط المفاتيح الظاهرة في الاستجابة (الغائب منها لم يحن قياسه بعد).

---

## 6. بوابة نجاح المرحلة 3

من `data/softlaunch.json` → `success_gate`:

- `min_kingdoms_live: 1` — مملكة واحدة على الأقل تعيش بلاعبين حقيقيين.
- `min_seasons_completed: 1` — موسم ألفا كامل يُلعب حتى النهاية (تتويج تحالف فائز في Zone 3).
- مؤشرات retention فوق العتبات في أول مملكة (D1/D7/D30 من الجدول أعلاه).

**قرار الاجتياز** يدوي: عند اكتمال أول موسم على مملكة الإطلاق، اسحب `/v1/admin/retention`
وقارن آخر cohorts مكتملة بالعتبات. إن تحقق كل شيء، حدّث `PLAN.md` بتحقق بوابة المرحلة 3.

---

## 7. تشغيل يومي مختصر (Runbook)

| الوتيرة | الإجراء | الأداة |
|---------|---------|--------|
| يومياً | سحب retention ومقارنته بالعتبات | `GET /v1/admin/retention` |
| يومياً | التأكد أن الإشغال لم يبلغ السقف | `GET /v1/launch/status` |
| عند امتلاء مملكة | فتح `kingdom-2` في JSON + نشر worker ثانٍ | تعديل `softlaunch.json` |
| أسبوعياً | مراجعة أقدم cohort لاكتمال D7 | من الاستجابة نفسها |
| نهاية الموسم | التحقق من تتويج تحالف + قرار البوابة | `/v1/season/scoreboard` + retention |

---

## 8. ملاحظات وحدود معروفة

- **التتبع يبدأ من تاريخ النشر** — cohorts قبل تطبيق migration 0006 لن يكون لها أيام نشاط مسجلة،
  فستظهر بنسب رجوع صفرية حتى يبدأ اللاعبون القدامى بالتفاعل بعد النشر.
- **DAU على مستوى النسخة المنشورة** — لكل worker (مملكة) قاعدة D1 خاصة، فالقياس معزول لكل مملكة افتراضياً.
- **لا تحليلات خارجية بعد** — هذا القياس داخلي بالكامل (D1 فقط). ربط أداة تحليلات خارجية (Amplitude/GameAnalytics)
  خطوة لاحقة عند الحاجة، والبيانات الخام متاحة في `player_activity` للتصدير.
- **`x-admin-key`** مطلوب لـ `/v1/admin/retention` — احفظ `ADMIN_KEY` في أسرار البيئة ولا تشاركه مع اللاعبين.
