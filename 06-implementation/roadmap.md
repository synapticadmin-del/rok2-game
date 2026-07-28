# خطة التنفيذ (Roadmap) — ROK2

## المرحلة 0 — تثبيت الرؤية (أسبوع 1–2)
- [x] بحث أنظمة + تفكيك تثبيت
- [ ] اختيار اسم وهوية (عربي فانتازي / تاريخ بديل)
- [ ] قفل Map Spec: Zone1=8 + جبال/ممرات، Zone2=4، Zone3=1
- [ ] GDD مختصر 15–25 صفحة من هذه الملفات

## المرحلة 1 — Prototype خريطة + مدينة (أسبوع 3–10)
### Client
- مدينة: City Hall + 4 موارد + 3 مباني عسكرية
- خريطة top-down: 8 regions مفصولة جبال
- Pass احتلال بسيط
- March من المدينة لـ resource/barb/pass

### Server
- Auth + player profile
- City state
- March simulation (1s tick)
- Pass ownership

### بوابة نجاح
لاعبان على نفس السيرفر يتنازعان ممرًا ويفهمان الخريطة بدون شرح طويل.

## المرحلة 2 — Vertical Slice (شهر 3–5)
- 6 حضارات
- 10 قادة
- تحالف + rally
- Hospital + battle reports
- Zone2 stubs (passes locked by timer)
- يوميات/متجر sandbox

## المرحلة 3 — Season Alpha (شهر 6–9)
- Zone unlock schedule كامل
- Zone3 core contest
- Season scoring
- Soft launch مملكة واحدة/اثنتين

## المرحلة 4 — Live
- Battle pass
- المزيد من القادة
- Anti-cheat نضج
- Matchmaking ممالك
- تحسين UA

---

## ترتيب بناء الماب تقنيًا (مهم)

1. **Tilemap + mountain collision**
2. **Region polygons + membership test**
3. **Pass entities + traverse rules**
4. **Pathfinding region-aware**
5. **AOI replication**
6. **Holy sites timers**
7. **Season unlock service**
8. **Zone3 scoring**

لا تبدأ بـ KvK متعدد الخرائط. ابدأ بخريطة واحدة موسمية ثابتة بثلاث Zones.

---

## فريق أدنى للـ Prototype
- 1 Game designer
- 2 Unity
- 1–2 Backend
- 1 Artist UI/map
- 1 Producer/QA part-time

---

## ملفات بيانات جاهزة
- `data/civilizations.json`
- `data/zones.json`
- `data/passes.json`
- `data/buildings.json`
- `data/troop_tiers.json`
