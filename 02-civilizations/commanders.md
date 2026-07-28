# القادة / الأبطال (Commanders)

## ما ظهر من تفكيك التثبيت
من `lc_hero.dat` بعد فك XOR:
- ندرة: **NORMAL / ADVANCED / ELITE / EPIC / LEGENDARY**
- حقول: Skills, Star Upgrade, Unit Capacity, Talent trees
- مسارات مواهب ظاهرة بالأسماء: **Infantry, Archer, Cavalry, Integration, Leadership, Conquering, Garrison, Peacekeeping, Gathering, Versatility, Skill, Develop**
- قادة ظاهرين في النصوص: Julius Caesar, Sun Tzu, Frederick I, Constantine I, Minamoto no Yoshitsune, Yi Seong-Gye, Cao Cao, Richard I, Charles Martel, El Cid, Hermann, Kusunoki Masashige, Scipio Africanus, Joan of Arc, Eulji Mundeok, Boudica, Pelagius, Spartacus, Lohar, City Keeper, Markswoman, Centurion...

> في لعبتك: استخدم شخصيات أصلية أو تاريخية بالترخيص المناسب — لا تنسخ نصوص/صور RoK.

---

## هيكل القائد (Data)
```json
{
  "id": "cmd_starter_roma",
  "name_key": "cmd.roma.valeria",
  "rarity": "epic",
  "civilization": "rome",
  "roles": ["infantry", "garrison"],
  "max_level": 60,
  "max_stars": 6,
  "skills": [
    {"slot": 1, "type": "active", "unlock_star": 0},
    {"slot": 2, "type": "passive", "unlock_star": 1},
    {"slot": 3, "type": "passive", "unlock_star": 2},
    {"slot": 4, "type": "passive", "unlock_star": 4},
    {"slot": 5, "type": "expertise", "unlock_star": 6}
  ],
  "talent_trees": ["infantry", "garrison", "skill"]
}
```

## الندرة والاقتصاد
| ندرة | الحصول | قوة متوقعة |
|------|--------|------------|
| Advanced | مبكر/مجاني | تعليمي |
| Elite | أحداث/نمو | متوسط |
| Epic | F2P path واضح | قوي في تخصص |
| Legendary | Gacha/أحداث كبرى | سقف الموسم |

## أزواج القادة (Primary + Secondary)
- مسيرة واحدة = قائد أساسي + ثانوي
- المهارات: Active من الأساسي غالبًا + passives من الاثنين حسب القواعد
- Unit capacity من الاثنين + tech + castle/vip

## تخصصات يجب دعمها
1. Field PvP
2. Rally (قيادة هجوم جماعي)
3. Garrison (دفاع مدينة/ممر/موقع)
4. Peacekeeping (Barbarians)
5. Gathering
6. Integration/Support

## MVP Commanders (10)
- 6 starters (واحد لكل حضارة)
- 2 epic مجانيين عبر قصة
- 2 legendary: واحد event F2P grind + واحد premium

لا تبدأ بـ 100 قائد.
