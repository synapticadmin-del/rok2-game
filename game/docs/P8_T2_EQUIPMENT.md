# P8-T2: نظام معدات القادة — الحداد (Blacksmith / Equipment)

## الهدف

نظام معدات كامل للقادة على غرار Rise of Kingdoms: 6 خانات معدّات (سلاح/خوذة/صدر/قفازات/سروال/حذاء)، تصنيع من مواد أولية، دمج 4 قطع متطابقة للترقية إلى جودة أعلى، تجهيز/خلع على القائد، مكافأة مجموعة (set bonus)، وبافات قتالية تُطبق في جميع مواضع القتال.

## الملفات

| الملف | الدور |
|-------|-------|
| `src/data/equipment.json` | البيانات الوحيدة للمصدر: 6 خانات + 4 مواد أولية × 5 جودات + 6 blueprints + set bonus + جداول التكلفة والنطاقات |
| `src/do/sim/equipment.ts` | منطق نقّي: تصنيع، دمج، تجهيز/خلع، حساب البافات (set bonus مقيد بـ `set_bonus_cap`) |
| `src/do/sim/combat.ts` | يستقبل `attackerEquipmentMod`/`defenderEquipmentMod` ويضيفهما لمعامل الهجوم والدفاع |
| `src/do/KingdomShard.ts` | يجلب `equipmentState` مع القائد في `fetchMarchCommander` (مصدر واحد للدولتَين) ويمرر الباڤ لمواضع القتال الأربعة |
| `src/http/router.ts` | endpoints: `GET /v1/commander/equipment/state`, `POST /v1/commander/equipment/craft|merge|equip|unequip`, `GET /v1/meta/equipment` (ضمن `meta/all`)، وبوابة مستوى City Hall `blacksmith_unlock_city_hall_level` |
| `migrations/0011_equipment.sql` | عمود `equipment_json` غير تخريبي في `player_commanders` و`march_commanders` |
| `scripts/equipment_offline_test.mjs` | الحارس — 72 عقدة تحقق من عقد JSON + صيغ التصنيع/الدمج/set bonus/القتال + وجود endpoints والمهاجرات |

## القواعد الأساسية

1. **البيانات لا hard-code:** كل قيم الجودات والتكاليف والنطاقات وحدّ السقف من `equipment.json`؛ الكود يقرأ فقط.
2. **بُنو الحداد:** لا يفتح الحداد إلا عند مستوى City Hall ≥ `blacksmith_unlock_city_hall_level` (من JSON).
3. **التصنيع:** تكلفة ذهب + موارد مدينة (تُخصم وتُسترد عند فشل التحقق)؛ الإحصاءات مولّدة من blueprint + نطاق الجودة + seed شبه عشوائي قابل للإعادة (نفس المدخلات = نفس الناتج).
4. **الدمج:** 4 قطع متطابقة (نفس الخانة والجودة) → قطعة واحدة بالجودة التالية؛ أقصى جودة عند `max_quality_index` يفشل الدمج برسالة.
5. **التجهيز:** خانة واحدة لكل قطعة؛ الخلع يعيد القطعة للمخزون.
6. **Set bonus:** 2/4/6 قطع مجهزة → باف `troop_attack` تدريجي مقيد بسقف `set_bonus_cap` (لا يتجاوز مجموع كل الباڤات السقف).
7. **القتال:** باف المعدات (`troop_attack` من الإحصاءات + set bonus) يُضاف كضرب في معامل الهجوم على جانبي أي مواجهة (ممر/عرش/هدف رئيسي/بربري)؛ قائد بلا معدات = باف صفر (توافقية كاملة مع ما قبل النظام).
8. **المسيرة:** عند تعيين قائد على مسيرة تُصوَّر `equipment_json` في `march_commanders` مع المواهب، فيتصرف القتال على الحالة المصورة لا الحالة الحية.

## التحقق الآلي

`npm run test:p8-t2-equipment` (72 عقدة، EXIT=0) مدرج في `npm run check` بعد `test:p8-t1-talents`، وفي `scripts/run_offline_tests.mjs`.

## تحفظات

- شاشة المعدات في العميل (6 خانات + التصنيع) في P8-T7 [UE] — الـ endpoints جاهزة للاستهلاك.
- القبول النهائي للباڤات في القتال على بيانات حية يتطلب E2E مع P8-T7؛ الحراس يثبتون العقد الرياضية حاليًا.

## مراجع

- وثيقة المواهب `P8_T1_TALENTS.md` (النظامان متكاملان في `fetchMarchCommander`).
- وثيقة جسر القبول `P7_T9_FULL_PLAY_LOOP_ACCEPTANCE.md`.
