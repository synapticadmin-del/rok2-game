# الأنظمة الأساسية (Core Systems)

## 1) الموارد
| المورد | مصادر | مصارف |
|--------|-------|-------|
| Food | Farm + map | Troops/upkeep/train |
| Wood | Lumber Mill + map | Build/train |
| Stone | Quarry + map | Build متقدم |
| Gold | Goldmine + map | Train متقدم / research |
| Gems | أحداث/شراء/نادر | Speedups/premium |
| Action Points | تجدد زمني | PvE / scouting costs |
| VIP points | شراء/نشاط | QoL |

## 2) المدينة والمباني (من نصوص build_en)
**Economic:** City Hall, Farm, Lumber Mill, Quarry, Goldmine, Storehouse, Trading Post, Courier Station, Shop  
**Military:** Wall, Watchtower, Barracks, Stable, Archery Range, Siege Workshop, Hospital, Castle, Scout Camp  
**Social/Progress:** Academy, Alliance Center, Tavern, Monument, Builder's Hut  

### ملاحظات تصميم من النصوص
- City Hall قلب السقف
- Builder's Hut = طوابير البناء (إضافي بجواهر)
- Hospital capacity حرج للـ PvP
- Castle مرتبط بالـ Rally capacity
- Tavern = بوابة القادة/الصناديق
- Alliance Center = helps + reinforcements

## 3) الجنود
أنواع: **Infantry / Cavalry / Archer / Siege**  
Tier عبر Research:
- T1: Swordsman / Bowman / Light Cavalry / Arcuballista
- T2: Spearman / Composite Bowman / Heavy Cavalry / Mangonel
- T3: Long Swordsman / Crossbowman / Knight / Ballista
- T4: Royal Guard / Royal Crossbowman / Royal Knight / Trebuchet (+ Special units)
- T5: لاحقًا

خصائص: ATK, DEF, HP, Speed, Load, Train cost/time, Counter table

## 4) البحث (Economic / Military)
من `research_en` تظهر شجرة كاملة تقريبًا:
- Economic: Irrigation, Sawmill, Masonry, Coinage, Engineering, Mathematics...
- Military: unit unlocks + ATK/DEF/HP + march speed + scouting + medical...

## 5) التحالف
Create/Join/Ranks/Help/Technology/Territory Flags/Forts/Rally/Chat/Shop/Gifts  
Passes & Holy sites تُحتل باسم التحالف.

## 6) القتال (Server authority)
مدخلات: troops + commanders + techs + buffs + type(field/rally/garrison/pve/siege)  
مخرجات: dead / severely wounded / lightly wounded + report  
Hospital يستقبل severely wounded حسب نوع المعركة (في بعض المواقع المقدسة نسبة موت أعلى).

## 7) الخريطة
انظر مجلد `01-map/` بالكامل (Zones/Passes/Core).

## 8) المواسم
- Season score
- Zone unlock schedule
- Finals rewards
- Migration rules بين الممالك (اختياري متأخر)

## 9) Monetization (بحذر)
Speedups, Gems, VIP, Bundles, Battle Pass, Commander sculptures  
حافظ على F2P path عبر التحالف والأحداث.
