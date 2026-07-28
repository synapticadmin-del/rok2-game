# شجرة البحث (Research Tree) — P2-T3

> المصدر الموحد: `data/research.json` (يُخدم من `/v1/meta/techtree` و`/v1/meta/all`).
> الخادم هو السلطة: التكلفة والمدة والمتطلبات والبافات كلها تُحسب على السيرفر.

## البنية

فرعان، 5 تقنيات لكلٍ منهما، 5 مستويات لكل تقنية:

| الفرع | التقنيات |
|-------|----------|
| **economy** | Agriculture (إنتاج +3%/مستوى)، Sickle (تجميع +5%)، Masonry (بناء +4%)، Logistics (حمل +6%)، Military Training (تدريب +5%) |
| **military** | Military Discipline (هجوم +2%)، Iron Armor (دفاع +2%)، Pathfinding (سرعة مسير +4%)، Field Medicine (صحة +3%)، Advanced Tactics (هجوم +4%) |

## القواعد

- **التكلفة:** `base_cost × cost_mult^(level−1)` — `cost_mult = 1.6`
- **المدة:** `base_duration_sec × duration_mult^(level−1)` — `duration_mult = 1.5`
- **الأكاديمية:** المستوى المطلوب = `academy_base_req × level`
- **المتطلبات (prerequisites):** يجب إكمال تقنيات سابقة لمستوى معين قبل البدء (مثلاً Advanced Tactics يتطلب Military Discipline L3 + Iron Armor L3)
- **الباف:** يُطبق فور اكتمال الطابور: `buff.per_level × المستوى المكتمل`

## البافات المطبقة فعلياً

| الباف | أين يُطبق |
|-------|-----------|
| `resource_production` | `applyProduction` في refreshCity — مضاعف على كل معدلات الإنتاج |
| `training_speed` | مدة طابور التدريب في `/v1/city/train` |
| `march_speed` | سرعة المسير في `createMarch` |
| `troop_attack` | `resolveCombat` لطرف المهاجم في كل معارك الممرات/العرش/البربر |

(البافات الأخرى موثقة في البيانات وتُقرأ من `researchBuff` — تُربط بأنظمتها عند بنائها: gather_speed بالتجميع، troop_health/troop_defense بالدفاع الحقيقي للمدن.)

## التخزين

جدول `player_research` (migration `0003_research.sql`):
```sql
(player_id, tech_id, level, updated_at) — PK (player_id, tech_id)
```
يكتبه الـ Durable Object عند اكتمال طابور `research` في الـ tick + يبث `tech_researched` عبر WebSocket.

## الـ API

- `GET /v1/research` — الشجرة كاملة مع مستويات اللاعب وتفاصيل المستوى التالي (تكلفة/مدة/متطلب أكاديمية)
- `POST /v1/city/research { techId }` — بدء بحث: يتحقق (tech موجود، لم يبلغ الحد، مستوى الأكاديمية، المتطلبات، الموارد) ثم يخصم ويطابور

## الاختبار

```bash
node scripts/research_offline_test.mjs   # 13 فحصاً للرياضيات والشجرة
BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/research_test.mjs
```
