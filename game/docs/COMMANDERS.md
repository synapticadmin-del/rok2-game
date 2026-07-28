# نظام القادة (Commanders) — P2-T1

> المصدر الموحد للبيانات: `data/commanders.json` (تُخدم من `/v1/meta/commanders` و`/v1/meta/all`).
> الخادم هو السلطة: كل البافات والخبرة والمستويات تُحسب على السيرفر فقط.

## نموذج البيانات

كل قائد في `data/commanders.json` يملك:

| الحقل | الوصف |
|-------|-------|
| `id` | معرّف ثابت (مثال: `cmd_rome_starter`, `julius_caesar`) |
| `rarity` | `elite` (قادة البداية) / `epic` / `legendary` |
| `nation` | الحضارة المرتبطة — يطابق `civilizations.json` |
| `base_stats` | قيم مرجعية للتوازن المستقبلي (لا تدخل القتال حالياً) |
| `skills` | **3 مهارات بالضبط:** واحدة `attack`، واحدة `defense`، واحدة `passive` |

أنواع المهارات:

| النوع | التأثير |
|-------|---------|
| `attack` | يزيد فعالية قوات المسيرة في القتال (`troop_attack` أو باف فرعي `infantry/cavalry/archer`) — `per_level × مستوى المهارة` |
| `defense` | يخفّض فعالية المهاجم ضد قوات القائد عندما يكون مدافعاً (سقف التخفيض 50%) |
| `passive` | `march_speed` (سرعة مسير) أو `xp_gain` (خبرة إضافية) — لا يدخل حساب القتال مباشرة |

ثوابت النظام (`constants` في نفس الملف):
- `max_level: 60`، `max_skill_level: 5`
- `tome_xp: 500` خبرة لكل تومة
- `starter_tomes: 3` تومات مع قائد البداية
- `summon_cost_gold: 500` ذهب لاستدعاء قائد
- `skill_upgrade_tome_cost: 2` تومتان لرفع مهارة

## دورة حياة القائد

1. **البداية:** `POST /v1/city/init` يمنح تلقائياً قائد البداية المطابق للحضارة (`starter_commander` في `civilizations.json`) مع 3 تومات.
2. **الاستدعاء:** `POST /v1/commander/summon { commanderId }` — 500 ذهب، قائد جديد بمستوى 1 ومهارات `[1,1,1]`.
3. **رفع المستوى:** `POST /v1/commander/levelup { commanderId, tomes }` — كل تومة = 500 XP، رفع تلقائي متعدد المستويات.
4. **رفع المهارة:** `POST /v1/commander/skill { commanderId, skillSlot: 1..3 }` — يتطلب مستوى قائد `10 × (المستوى المستهدف)` + تومتان.
5. **الإرسال للمعركة:** أضف `primaryCommanderId` عند إنشاء مسيرة (`/v1/world/march` أو `/v1/world/pass/attack`) أو عيّنه لاحقاً بـ `POST /v1/commander/assign { marchId, commanderId }`.
6. **الخبرة من القتال:** كل قتيل من قوات الخصم = 2 XP للقائد المرافق عند حلول المعركة.

## القتال (server-authoritative)

في `resolveCombat`:
- `aEff = aPower × counterMult × (1 + attackMod_attacker) × (1 − min(0.5, defenseMod_defender))`
- `dEff = dPower × (1 + attackMod_defender)`

الباف يُقرأ من مستويات المهارات المخزنة في `march_commanders.skills_json` وقت إنشاء المسيرة — أي رفع مهارة لاحق لا يؤثر على مسيرة جارية.

## الجداول (D1 — migration `0002_commanders.sql`)

```sql
player_commanders(id, player_id, commander_id, level, xp, tomes, skills_json, created_at)
march_commanders(march_id PK, player_id, commander_id, skills_json, created_at)
```

## قائمة القادة الحالية (12)

- **قادة البداية الستة:** Scipio (Rome)، Sun Tzu (China)، Baibars (Arabia)، Cleopatra (Egypt)، Ragnar (Vikings)، Kusunoki Masashige (Japan)
- **الأسطوريون الستة:** Julius Caesar، Richard the Lionheart، Yi Seong-Gye، Genghis Khan، Joan of Arc، Alexander the Great

## الاختبار

```bash
BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/commanders_test.mjs
```

يغطي: شكل البيانات الموحدة، منح قائد البداية، رفع المستوى بالتومات، حارس مستوى المهارة، الاستدعاء وتكراره، هجوم ممر بقائد مرافق، واكتساب الخبرة بعد المعركة.
