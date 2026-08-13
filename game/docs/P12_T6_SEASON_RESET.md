# P12-T6: نهاية الموسم وإعادة الضبط الموسمي (Season End & Reset)

## الهدف

نظام كامل لإنهاء الموسم الحالي، حساب تقرير نهائي (Champion + Leaderboard + Legacy points)، ثم إعادة ضبط المملكة بالكامل استعدادًا للموسم الثاني. كل القيم مقروءة من `data/shop.json` — لا ثوابت hard-coded.

## البنية

| المكوّن | المسار | الوظيفة |
|---|---|---|
| منطق نقي | `src/do/sim/season_reset.ts` | `computeSeasonReport` + `resetWorldForSeason` + `legacyConfig` + `legacyPointsFromReport` |
| تنفيذ الشارد | `src/do/KingdomShard.ts` | handlers `season-end` / `season-reset` / `season-report` + migration 20 |
| HTTP Router | `src/http/router.ts` | `GET /v1/season/report` + `POST /v1/admin/season-end` + `POST /v1/admin/season-reset` |
| ثوابت | `src/data/shop.json` (`constants.season.legacy_per_1000 = 10`) | نقاط Legacy لكل 1000 نقطة موسم |
| anti-cheat | `src/data/anticheat.json` | rate limits لـ `season_end` / `season_reset` / `season_report_read` |
| حارس جودة | `scripts/p12_offline_test.mjs` | 26 فحصًا — منطق نقي + ثوابت + وثيقة + chain |

## تدفق نهاية الموسم

1. **الإدارة** تستدعي `POST /v1/admin/season-end` (مُحمي بـ `requireAdmin` + rate limit 3/ساعة).
2. الشارد يبني مدخلات التقرير من حالته الحية:
   - `throneScores`: نقاط العرش لكل تحالف (متصدر الموسم = Champion).
   - `playerScores`: أعلى 200 لاعب حسب `power` (جدول D1 `players`).
   - إحصاءات: ممرات محتلة، مناطق مفتوحة، مدن، Citadels مدمرة في Lost Kingdom، مهاجرون، أحداث قصة.
3. `computeSeasonReport` (نقي): ترتيب TOP 50 للتحالفات واللاعبين، وحساب **Legacy points** = `floor(score × legacy_per_1000 / 1000)`.
4. التقرير يُحفظ في `season_reports` + تحديث `season_meta` (`ended=1`, `ended_at_ms`).

## تدفق إعادة الضبط الموسمي

1. **الإدارة** تستدعي `POST /v1/admin/season-reset` (بعد season-end فقط).
2. `resetWorldForSeason` (نقي) يولّد قائمة عمليات؛ الشارد يطبقها:
   - العرش: مالِك/تقدم ← null، حالة `open`، يوم فتح جديد.
   - الممرات (`passes`): مالِك ← null، تقدم ← 0، حالة `open`.
   - المواقع المقدسة (`holy_sites`): مالِك ← null، تقدم ← 0.
   - الأهداف المركزية (`core_objectives`): مالِك/أول ماسك ← null، تقدم ← 0.
   - نقاط العرش (`throne_scores`): ← 0 لكل تحالف.
   - الأعلام (`flags`): حذف علم كل تحالف.
   - قوى اللاعبين (D1 `players.power`): ← 0 (منظفات فقط — المدن/المباني تبقى كأثر الموسم).
   - Lost Kingdom: مسح المنشآت/Citadels، زيّقورة جديدة (hp كامل، مفتوحة)، مسح الهجرة (cooldown يُحفظ في `last_migrated_ms`)، عملات/نقاط ← 0.
   - يوم الموسم ← 0، `season_start_ms` ← الآن.
3. `season_reset_count` يزيد، والشرارة تُحفظ (`season_meta`, `legacy_points` لكل تحالف/لاعب عبر `legacy_points` جدول D1 في persistSeasonReport).

## Legacy Points

- تُحسب تلقائيًا عند season-end ولا تحتاج أي فعل من اللاعب.
- ثابت `legacy_per_1000` (10 افتراضيًا) = نقاط Legacy عن كل 1000 نقطة موسم.
- يُمكن استخدام Legacy مستقبلًا لفتح تذكارات/ألقاب الموسم السابق (P13).

## جدول season_reports (D1)

```sql
CREATE TABLE IF NOT EXISTS season_reports (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  season_id TEXT NOT NULL,
  generated_at INTEGER NOT NULL,
  champion_alliance_id TEXT,
  champion_score REAL,
  report_json TEXT NOT NULL,
  created_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS season_meta (
  id TEXT PRIMARY KEY,
  ended INTEGER NOT NULL DEFAULT 0,
  ended_at_ms INTEGER,
  reset_count INTEGER NOT NULL DEFAULT 0,
  last_reset_at_ms INTEGER
);
CREATE TABLE IF NOT EXISTS legacy_points (
  player_id TEXT,
  alliance_id TEXT,
  season_id TEXT,
  points REAL NOT NULL DEFAULT 0,
  PRIMARY KEY (player_id, season_id)
);
```

## حماية

- `season-end`/`season-reset`: إداري فقط (`X-Admin-Key`) + rate limit 3 actions/ساعة.
- `season-report`: requireAuth + rate limit 60/دقيقة.
- إعادة الضبط لا تتم إلا بعد `ended=1` (skip مع سبب `season_not_ended`).
- idempotency: season-end مكرر يعيد نفس التقرير؛ reset لا يُنفذ قبل الموسم المنتهي.

## فحص الجودة

`test:p12-season-reset` (26 فحصًا): منطق نقي مطابق لمصدر `season_reset.ts`، ثوابت shop.json، anti-cheat limits، وجود الوثيقة، ودمجها في `npm run check` بعد `test:p11-client`.
