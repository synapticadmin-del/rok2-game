# P9-T1 — تكنولوجيا التحالف (Alliance Technology)

**الحالة:** مكتمل ومُرفوع — commit `e7e796f` (2026-08-13) | **[backend]**
**الحارس:** `game/backend/scripts/alliance_tech_offline_test.mjs` (120 عقدة، ALL PASSED)
**الـ job:** `test:p9-t1-alliance-tech` داخل `npm run check` | `npm run check` كاملة EXIT=0

## الفكرة

في Rise of Kingdoms، تقنية التحالف نظام **بحث جماعي** يبدأه ضابط (R3+) ويكمل **كل أعضاء التحالف بالتبرعات** (كل تبرع = نقطة بحث). هذا البند يجعل التحالف كيانًا ذا تطور دائم: التحالفات النشطة تصبح أقوى قتاليًا واقتصاديًا مع الوقت.

## البيانات — `src/data/alliance_tech.json`

```json
{
  "version": 1,
  "techs": [ /* 20 تقنية */ ],
  "donation": {
    "window_seconds": 1800,
    "max_donations_per_window": 20,
    "points_per_donation": 1
  },
  "research": { "min_rank": "R3" }
}
```

| الفئة | التقنيات (مثال) | الباف |
|-------|-----------------|-------|
| تطوير (development) | research_speed, building_speed, healing_speed, ap_regen | نسب تطوير/شفاء |
| أرض (territory) | gather_bonus, march_speed, hospital_capacity | نسب اقتصاد |
| حرب (war) | attack_buff, defense_buff, hp_buff, siege_damage, pass_damage, rally_boost | نسب قتال |
| مهارة (skill) | help_speed_bonus, commander_xp, commander_skill | نسب مساعدة/خبرة |

كل تقنية: `level_required: number[]` (عتبات نقاط متصاعدة) + `levels` + `effect: { buff: string, per_level: number }`. البافات تتراكم خطيًا: `buff_value = Σ per_level × level`.

## المنطق النقي — `src/do/sim/alliance_tech.ts`

`AllianceTechService` طبقة نقية بلا `fs` ولا اعتماد على الخادم (نفس نمط `sim/daily_quests.ts`):

- `canDonate(nowMs, windows)` — نافذة عائمة 30 دقيقة، سقف 20 تبرعًا؛ النافذة القديمة تُنظف تلقائيًا (يعطي كل عضو 20 نقطة/نصف ساعة بشكل عادل).
- `recordDonation(nowMs, windows)` — يسجل تبرعًا ويعيد مصفوفة جديدة.
- `levelForPoints / pointsForLevel / applyPoints` — سلم مستويات من عتبات JSON.
- `canStartResearch(rank)` — `R3+` فقط.
- `computeBuffs(state)` — تجميع النسب عبر كل التقنيات المنشطة.

## الخادم — `KingdomShard.ts` + `router.ts`

| المكوّن | التفصيل |
|---------|---------|
| `AllianceTechState` | `Map<allianceId, { techs: Map<techId,{points,level,researchStartedAtMs}>, activeResearch: techId \| null }>` + `donationWindows: Map<allianceId, Window[]>` |
| migration 12 | تحميل `alliance_tech_state` و `donation_windows` من SQL |
| persistence | `persistAllianceTech()` في tick + عند كل تغيير |
| endpoints داخلية `/do/` | `alliance-tech-donate` / `alliance-tech-start` / `alliance-tech-state` |
| البث | `allianceTechState` داخل `snapshot(playerId)` (تصفية لتحالف اللاعب فقط) و `worldDelta` (التحالفات غير الخاوية) |
| تطبيق البافات | `scaleTroops(m.troops, 1+mod)` قبل `resolveCombat` في **5 مواضع**: مسيرة عادية، عرش الملك، قلب Zone، موقع مقدس، برابرة — + باف المساعدة في `/v1/alliance/help` |
| rate limits | `alliance_tech_donate` / `alliance_tech_start` في `anticheat.json` |

**router:** `GET /v1/alliance-tech/state` + `POST /v1/alliance-tech/donate` + `POST /v1/alliance-tech/start` (باقي رتبه R3+، بحث واحد نشط لكل تحالف).

## قواعد اللعبة

1. **تبرع:** أي عضو في تحالف يمكنه التبرع (1 نقطة/تبرع، 20/نصف ساعة) — النقاط تضاف لتقنية البحث الجماعي النشط.
2. **البحث:** يبدأه R3+؛ عند اكتمال مستواه تنتقل نقاط التبرعات للمرحلة التالية تلقائيًا؛ يمكن تبديل البحث النشط متى شاء الضباط.
3. **البافات:** تسري على **كل أعضاء التحالف** تلقائيًا في القتال والمساعدة — لا تحتاج أي إجراء من العضو.
4. **استمرارية:** البحث يُحفظ في DO ويُعاد تحميله مع الهجرة الداخلية؛ البافات تتوقف فور مغادرة العضو للتحالف (تُحسب من `m.allianceId` لحظة المعركة).

## ما تبقى للعميل C++ (P9-T7)

شاشة تقنية التحالف (فئة ← قائمة تقنيات ← نافذة بحث نشط + تبرع) تستهلك endpoints أعلاه — تُنفَّذ في بند مسار العميل P9-T7 وتُقبل على PIE/UE 5.4.4 Windows.
