# P8-T6: المهام اليومية والجوائز (Daily Quests)

**الالتزام:** إكمال عمق أنظمة اللعب اليومية — حلقة Engagement يومية مع جوائز ملموسة تدفع اللاعب للعودة كل يوم.
**التاريخ:** 13 أغسطس 2026
**Commit:** `2477e6e`

## ملخص

نظام مهام يومية وأسبوعية حتمي (deterministic): 5 مهام يوميًا من 8 أنواع، و3 مهام أسبوعية من 5 أنواع. التقدم يُجمع تلقائيًا من أحداث اللعبة الفعلية (تدريب، معارك، جمع، بحث، تسريع، مساعدة تحالف، تطوير مباني). اكتمال 100 نقطة يومية يمنح **مفتاحًا ذهبيًا** (200 جوهرة)، واكتمال 300 نقطة أسبوعية يمنح **صندوقًا أسبوعيًا** (500 جوهرة + 2 مسرّع ساعة).

## الملفات

| الملف | الدور |
|---|---|
| `src/data/daily_quests.json` | بيانات الأنواع، المجمعات (goal_range / point_options)، الثوابت، الجوائز |
| `src/do/sim/daily_quests.ts` | منطق نقّي: توزيع حتمي seededRandom، `applyProgress`، `questDay`/`questWeek` |
| `migrations/0012_daily_quests.sql` | جداول `player_quests` / `player_quest_points` / `player_quest_rewards` |
| `src/do/KingdomShard.ts` | helpers + endpoints داخلية `/quests/state` و`/quests/progress` + رصد من tick وcombat |
| `src/http/router.ts` | Endpoints عامة: `GET /v1/quests`، `POST /v1/quests/claim`، `POST /v1/quests/redeem-golden-key`، `POST /v1/quests/redeem-weekly-chest` |
| `scripts/daily_quests_offline_test.mjs` | حارس نقّي (25+ فحصًا) |

## مصادر التقدم (progress_sources)

| المصدر | الحدث المنتج | الموضع |
|---|---|---|
| `train` | تدريب جنود (اكتمال طابور) | KingdomShard tick: completedQueues + router city/train |
| `battle_win` | نصر في هجوم على مدينة | `settleAttackerCombat` |
| `barb_kill` | قتل وحدات برابرة | `resolveMarchArrival` (victory) |
| `gather` | اكتمال جمع الموارد (الكمية المجمعة) | tick: gathering block |
| `help` | مساعدة عضو تحالف | router alliance/help |
| `research_start` | بدء بحث في الأكاديمية | KingdomShard tick + router city/research |
| `speedup` | استخدام مسرّع | router shop/use-speedup |
| `build_upgrade` | تطوير مبنى | KingdomShard tick + router city/upgrade |

## الجوائز

| الجائزة | الشرط | المكافأة |
|---|---|---|
| مفتاح ذهبي | 100 نقطة يومية، مرة واحدة يوميًا | 200 جوهرة + سجل `golden_key` |
| صندوق أسبوعي | 300 نقطة أسبوعية، مرة واحدة أسبوعيًا | 500 جوهرة + 2 × `speedup_1h` |

## حتمية التوزيع

نفس `player_id` + نفس اليوم UTC → نفس المهام الخمس (djb2 hash + seededRandom). هذا يجعل النظام قابلًا للاختبار وقابلًا لإعادة الإنتاج في أي shard.

## حارس الجودة

`scripts/daily_quests_offline_test.mjs` — Job: `test:p8-t6-daily-quests`، مشمول في `npm run check` و`test:offline`.
