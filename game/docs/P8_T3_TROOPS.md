# P8-T3: نظام الوحدات — tiers T1–T5 والوحدات الخاصة الحضارية

## الهدف

تحويل منظومة الوحدات من ثوابت hard-coded إلى نظام بيانات كامل مستمد من `troop_tiers.json`: خمس درجات تدريب (T1 → T5) مقفلة تدريجيًا بمستوى City Hall، أربع فروع (مشاة/فرسان/رماة/حبال)، ثلاثي المقابلة القياسي (infantry ⟷ cavalry ⟷ archer)، ووحدة خاصة واحدة لكل حضارة بإحصاءات معدّلة ترث من درجة أساسها.

## الملفات

| الملف | الدور |
|-------|-------|
| `src/data/troop_tiers.json` | البيانات الوحيدة للمصدر: 4 فروع + 5 درجات، لكل درجة إحصاءات الفروع الأربعة + `unlock_building_level` (T1=1, T2=3, T3=5, T4=7, T5=9) + عدادات المقابلة |
| `src/data/civilizations.json` | `special_unit` لكل حضارة: id + branch + `unlock_tier` + `stat_mods` (attack/defense/health) |
| `src/do/sim/troops.ts` | منطق نقّي جديد: `unitTier`/`unitBranch`/`isSpecialUnit`/`tierData`/`hallUnlocksTier`/`troopTierStats`/`unitName`/`maxTroopTier`/`specialUnitsForCiv`/`specialUnitStats`/`unitStats`/`unitAtk`/`counterMult`/`trainableUnits` |
| `src/do/sim/combat.ts` | `unitAtk` + `counterMult` من `troops.ts`؛ المعركة تقبل الآن `attackerCiv`/`defenderCiv` لتفعيل الوحدات الخاصة |
| `src/do/KingdomShard.ts` | `CityEntity.civ` جديد (schema/persist/loadState/upsert)؛ civ يُمرر لـ `resolveCombat` في المواضع الأربعة (ممر/عرش/هدف رئيسي/بربري) |
| `src/http/router.ts` | `trainableUnits` يُبنى ديناميكيًا من tiers بدل ثابت؛ تدريب whitelist من ids القانونية + فحص unlock level + مدة `trainDurationSec` + civ في city_upsert |
| `src/lib/gameData.ts` | `trainCost`/`trainDurationSec`/`unitPower` تُقرأ من `troop_tiers.json` بدل hard-coded |
| `scripts/troops_offline_test.mjs` | الحارس — عقد JSON + صيغ الدرجات + المقابلة + وحدات الحضارات + القائمة القابلة للتدريب (EXIT=0) |

## القواعد الأساسية

1. **البيانات لا hard-code:** كود الوحدات كله يقرأ من `troop_tiers.json` (و`civilizations.json` للخاصة)؛ لا ثوابت إحصائية داخل الكود.
2. **فك الهوية:** `branch_tN` (مثل `cavalry_t4`) وحدة قياسية درجة N من الفرع؛ أي id بدون `_t` وحدة خاصة (مثل `legionary`) تُحل عبر civ. الدرجات خارج النطاق تُclamp إلى `maxTroopTier`.
3. **قفل الدرجات:** الدرجة T تُفتح عند مستوى City Hall ≥ `unlock_building_level` للدرجة؛ التدريب يفشل برفض موثق دون المستوى، والقائمة المعروضة للعميل تولَّد ديناميكيًا (T5 مخفية حتى level 9).
4. **المقابلة:** مثلث متوازن — مشاة ⟶ فرسان ⟶ رماة ⟶ مشاة بمعامل 1.15 للمفيدة و0.87 للضعيفة؛ الحبال محايدة (1.0) ما عدا حبال ضد حبال (1.1)؛ ids غير معروفة = 1.0 بأمان.
5. **الوحدة الخاصة:** واحدة لكل حضارة مُعرَّفة بفرعها ودرجة قفلها ونسب التعديل؛ إحصاءاتها = إحصاءات الدرجة الأساسية بعد تطبيق `stat_mods`، وتبقى محايدة في المقابلة (1.0) كسلوك آمن موثق.
6. **التدريب:** التكلفة والمدة من JSON؛ القدرة = دالة تصاعدية للإحصاءات (مصدر واحد مع القتال)؛ تدريب وحدة مقفلة يُرفض على مستوى الراوتر.
7. **القتال:** الهجمات في جميع المواضع الأربعة تعتمد `unitAtk`/`counterMult` الموحدة، مع civ للمهاجم والمدافع؛ لا civ = لا وحدات خاصة (توافقية تامة مع ما قبل النظام).
8. **المدينة:** `civ` حقل اختياري غير تخريبي في CityEntity يُحفظ/يُحمَّل/يُحدَّث تلقائيًا مع باقي حالة المدينة.

## التحقق الآلي

`npm run test:p8-t3-troops` (EXIT=0) مدرج في `npm run check` بعد `test:p8-t2-equipment`، وفي `scripts/run_offline_tests.mjs` ضمن `npm run test:offline`.

## تحفظات

- شاشة تدريب الوحدات الجديدة (T4/T5 والوحدة الخاصة) في العميل [UE] تُبنى لاحقًا؛ endpoints القائمة (`/v1/city/train`) ترجع الآن قائمة ديناميكية فتعمل على العميل الحالي دون تغيير.
- القبول النهائي لسلوك المقابلة والوحدات الخاصة في قتال حي يتطلب E2E مع P8-T5؛ الحراس يثبتون العقد الرياضية حاليًا.

## مراجع

- وثيقتا `P8_T1_TALENTS.md` و`P8_T2_EQUIPMENT.md` (الأنظمة الثلاثة تُطبَّق معًا في `fetchMarchCommander`/`resolveCombat`).
