# تقطير شامل لأنظمة اللعبة الأصلية (System Distillation Catalog)

مصدر التقطير: تثبيت محلي + locale + ls.dat + assemblies + map packs.  
الغرض: مرجع تصميم — **ليس** سورسًا قابلاً للتشغيل.

---

## 1) مدينة اللاعب
**كيانات:** City Hall, Farm, Lumber Mill, Quarry, Goldmine, Storehouse, Builder’s Hut, Wall, Watchtower, Barracks, Stable, Archery Range, Siege Workshop, Academy, Hospital, Castle, Alliance Center, Tavern, Trading Post, Scout Camp, Courier Station, Shop, Monument  
**Loops:** upgrade → queue/time/resources → power ↑ → unlock caps  
**Sinks:** speedups, gems for extra queue

## 2) موارد
Food/Wood/Stone/Gold + Gems + Action Points + VIP  
إنتاج مباني + جمع بري + أحداث + نهب

## 3) جنود
4 فروع × tiers (T1–T4 ظاهرة بأسماء، T5 لاحقًا في اللعبة الحية)  
تدريب/ترقية/تحميل/سرعة/مستشفى

## 4) قادة
Rarity ladder، skills، stars/sculptures، talents (inf/arch/cav/lead/conq/garrison/gather…)  
Primary+Secondary pairing

## 5) بحث
Economic tree + Military tree (unlock units + stats + scouting + medical…)

## 6) خريطة العالم
- Provinces/Regions  
- Terrain: grass/clay/mountain/river/water/bridge + LOD  
- Pass objects + ownership  
- Holy: Altar/Shrine/Sanctum/Temple  
- Resources nodes / barbarians / forts  
- Player cities + marches + scouts  
- Alliance flags/fortresses territory  

## 7) حركة/قتال
March simulation، path constraints عبر Pass، combat reports، rally/garrison، wounded rules تختلف حسب الهدف

## 8) تحالف
Ranks, help, tech, territory, rally, chat, shop/gifts

## 9) مواسم Lost Kingdom / KVK
- خرائط متعددة `LostLand_Map_*`  
- 8 starting provinces + fortresses  
- Chapter-gated pass levels  
- Center King's Land / Lost Temple  
- Immigration/region matching seasons 1–3 + conquest variants  
- Crystal/outpost/camp side systems في سيناريوهات لاحقة  

## 10) LiveOps / اقتصاد حقيقي
Events، BattlePass(es)، IAP bundles، mail، quests، cosmetics/skins، civilization theme events

## 11) تقني عميل
Unity 2022.3 LTS، IL2CPP، EZ map runtime، LGUI، AssetBundles `.lb`، locale XOR zip، Sentry، RTC/video/webview

## 12) ما يجب إعادة بنائه في ROK2 (أولوية)
1. City economy MVP  
2. Map 2400 with 8/4/1 zones + passes  
3. March + PvE  
4. Alliance + pass capture  
5. Season unlock + Zone3 scoring  
6. Commanders slim set  
7. LiveOps later  

---

## إحصائيات التقطير
- 9622 class names في ls.dat  
- 1704 version_map entries  
- 273 ground tiles (.btil)  
- 851 map materials  
- 20 locale packs مفكوكة تقريبًا في `_extracted`
