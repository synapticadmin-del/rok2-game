# الحضارات (Civilizations)

## مبدأ التصميم
كل حضارة تعطي:
1. **بونصات دائمة** (اقتصاد/عسكري/جودة حياة)
2. **وحدة خاصة** (Special Unit) تُفتح متأخرًا (حوالي T4)
3. **قائد بداية** مرتبط بالحضارة
4. **ثيم بصري** للمدينة/الجنود/العلم
5. **قيمة/Fantasy** واضحة جملة واحدة

> من التثبيت المحلي ظهرت حضارات في النصوص/الواجهات: Rome, China, Greece, Britain, Germany, France, Spain, Japan, Korea, Arabia, Ottoman, Egypt, Byzantium, Vikings…  
> وكذلك أنظمة `AllUISelectCivilization`, `CivilizationFlagList`, أحداث ثيم (Egypt/Greece skins).

---

## قائمة حضارات مقترحة لنسختك

### للـ MVP (6 حضارات فقط)
| ID | الحضارة | فانتازي جملة | بونصات مقترحة | وحدة خاصة | قائد بداية |
|----|---------|--------------|---------------|-----------|------------|
| civ_rome | **Rome** | نظام وجيوش ثقيلة | Troop Defense +3%, Gathering +5% | Legionary (Inf) | Starter Roman |
| civ_china | **China** | بناء وإمداد | Building Speed +5%, AP recovery +5% | Chu-Ko-Nu (Arch) | Starter Chinese |
| civ_arabia | **Arabia** | خفة وحركة | March Speed +5%, Cavalry ATK +2% | Desert Rider (Cav) | Starter Arab |
| civ_egypt | **Egypt** | اقتصاد وحجر | Resource production +5%, Stone gather +5% | Khopesh Guard (Inf) | Starter Egyptian |
| civ_vikings | **Vikings** | غزو ونهب | Counterattack dmg +3%, Load +5% | Huskarl (Inf) | Starter Viking |
| civ_japan | **Japan** | مهارة وقتل | Troop ATK +2%, XP gain +5% | Samurai (Inf/Cav hybrid feel) | Starter Japanese |

### للتوسع لاحقًا (بعد MVP)
Britain, Germany, France, Spain, Korea, Ottoman, Greece, Byzantium, Maya, Persia, Mongolia, Aksum...

---

## جدول بونصات مفصل (قابل للـ JSON)

### Rome
- Infantry Defense +5%
- Troop Health +2%
- Resource gathering +5%
- **Special:** Legionary — high defense infantry

### China
- Building Speed +5%
- Research Speed +3%
- Action Point recovery +5%
- **Special:** Repeating Crossbow unit — sustained archer DPS

### Arabia
- March Speed +5%
- Cavalry Attack +4%
- Scout map reveal bonus
- **Special:** Camel/Desert Rider — strong open-field cavalry

### Egypt
- All resource production +5%
- Stone gathering +8%
- Healing speed +3%
- **Special:** Khopesh Guard / Chariot variant

### Vikings
- Load +10%
- Damage to barbarians / gather camps +5%
- Troop attack +2%
- **Special:** Huskarl / Longship raiders theme

### Japan
- Troop Attack +3%
- Commander XP +5%
- Training speed +3%
- **Special:** Samurai — elite infantry with skill synergy

---

## قواعد تغيير الحضارة
- مرة مجانية مبكرة / أو بتكلفة متزايدة
- **الوحدة الخاصة** تتحول عبر تحويل مدفوع موارد/وقت
- القادة لا يُمسحون عند التحويل
- Skin المدينة يتغير

---

## ثيمات بصرية لكل حضارة
| حضارة | ألوان | عمارة | صوت/طبل |
|-------|-------|-------|---------|
| Rome | أحمر رخامي | أقواس، قناطر | أبواق |
| China | ذهبي قرمزي | أسقف منحنية | طبول/ناي |
| Arabia | ذهبي رملي | قبب، أقواس حدوة | إيقاعات صحراوية |
| Egypt | تركواز وذهب | أعمدة، مسلات | آلات وترية قديمة |
| Vikings | أزرق حديدي | خشب منحوت | قرون |
| Japan | أسود قرمزي | معابد، شوادق | تايكو |

---

## توازن مهم
- لا حضارة “لازم تتعمل بيها”
- اجعل البونص **~2% إلى 8%** في مجاله
- القوة الحقيقية من: قادة + تحالف + ممرات + اقتصاد شخصي
- الـ Special Unit متأخرة حتى لا تحسم اختيار الحضارة من الدقيقة 1 فقط

---

## بيانات تنفيذ
انظر: `data/civilizations.json`
