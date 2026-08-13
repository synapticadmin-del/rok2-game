# P8-T7: مسار P8 للعميل C++ (Unreal Engine)

**الحالة:** مكتمل — بنيوي (التحقق النهائي بالبناء وPIE على UE 5.4.4 Windows).
**الالتزام:** `P8-T7: مسار P8 للعميل C++ (مواهب/معدات/دروع/تهجير/مهام/ملك)`

## الهدف

إيصال أنظمة عمق المرحلة P8 (المواهب T1، الحدادة T2، الدروع والتهجير T5، المهام اليومية T6، ملك المملكة) إلى العميل C++ عبر طبقة `URok2Api` الجاهزة لـ Blueprint، مع فحص بنيوي مضمّن في الريبو لأن بيئة التنفيذ لا تملك أدوات Unreal.

## ما أُضيف إلى `Source/Rok2`

### `Public/Rok2Types.h` — 11 struct جديد

| Struct | الغرض |
|--------|-------|
| `FRok2TalentNode` / `FRok2TalentTree` | عقدة موهبة (id/name/tree/level/maxLevel/powerCost/statMods/prerequisites) وشجرة كاملة |
| `FRok2EquipmentItem` / `FRok2EquipmentBlueprint` / `FRok2EquipmentSlot` | قطعة معدات (id/slot/blueprint/quality/material/stats) وموازفاتها |
| `FRok2CommanderTalents` | حالة مواهب قائد: العقدة/النقاط المتاحة/المخصصة |
| `FRok2ShieldOption` / `FRok2ActionPointState` | خيارات الدروع (دقائق/جواهر) وحالة AP المدينة (cap/درع/حمى الحرب/تهجير) |
| `FRok2DailyQuest` / `FRok2QuestState` | مهمة يومية/أسبوعية وحالة النقاط + أهلية المفتاح الذهبي والصندوق الأسبوعي |
| `FRok2KingMarker` / `FRok2GameMeta` | ملك المملكة على العرش وخزينة meta للمواهب والمعدات |
| تحديث `FRok2WorldSnapshot` | حقل `King` + قائمة `CapturedSiteIds` |

### `Public/Rok2Api.h` + `Private/Rok2Api.cpp` — 17 UFUNCTION BlueprintCallable

| الفئة | الوظائف | endpoints |
|-------|---------|-----------|
| المواهب | `FetchTalents`، `AllocateTalent`، `RespecTalents` | `GET /v1/commanders`، `POST /v1/commander/talent/allocate`، `POST /v1/commander/talent/reset` |
| الحدادة | `FetchEquipment`، `CraftEquipment`، `EquipItem`، `UnequipItem`، `MergeItems` | `GET /v1/commander/equipment`، `POST /v1/commander/equipment/{craft,equip,unequip,merge}` |
| الدروع/التهجير | `FetchShieldOptions`، `ActivateShield`، `RelocateCity` | `GET /v1/ap/state`، `POST /v1/shield/activate`، `POST /v1/city/relocate` |
| المهام | `FetchQuests`، `ClaimQuest`، `RedeemGoldenKey`، `RedeemWeeklyChest` | `GET /v1/quests`، `POST /v1/quests/{claim,redeem-golden-key,redeem-weekly-chest}` |
| العالم | `FetchKing`، `MarchToHolySite` | لقطة العالم (king/throne)، `POST /v1/world/march` مع `targetType=holy_site` |

### تفاصيل تنفيذية مهمة

1. **`FetchTalents`** يقرأ `GET /v1/commanders` ثم يدمج خزينة المواهب `Meta.TalentTrees` (من `/v1/meta/talents` الذي يُسحب تلقائيًا في `FetchMeta`) مع تخصيصات الخادم `talentAllocations` لبناء العقدة بمستواها الحالي.
2. **`FetchEquipment`** يقرأ `equipped` من `commander.equipment` ويبث `OnEquipmentUpdated` بخانات الست (`weapon/helmet/chest/gloves/legs/boots`) مع `bFilled`.
3. **`FetchKing`** يعيد تشغيل `RefreshWorld()`؛ لقطة العالم هي مصدر الحقيقة للعرش والملك، و`ParseWorld` الجديد يقرأ `snapshot.king` و`snapshot.throne` ويسجل المواقع المقدسة المحتلة في `World.CapturedSiteIds` ثم يبث `OnKingUpdated`/`OnWorldSnapshot`.
4. **`ParseWorld`** أُضيف إليه King/Throne/HolySites قبل البث — يعمل على `world_delta` وأيضًا snapshot كامل (TryGet آمن).
5. **`MarchToHolySite`** يرسل `scout: 10` مع `targetType=holy_site` و`holySiteId`؛ الخادم يضع الإحداثيات تلقائيًا ويفرض قفل التقويم (`temple_locked`) والموقع غير المكتشف (`holy_site_not_found`).
6. **`UpsertKing`** يحفظ الملك في `World.King` ويبث `OnKingUpdated` لاستخدام WorldRenderer.

### `Rok2WorldRenderer.cpp/h` — علامة الملك على العرش

`DrawKingMarker()` تُرسم في نهاية `RefreshFromApi` عندما يكون `World.King` نشطًا، مع رمز `SpawnedThrone` على إحداثيات العرش؛ تُعاد التهيئة لكل دورة رسم.

## حارس الجودة

`game/client-unreal/scripts/verify_p8_client.mjs` — 58 فحصًا بنيويًا: وجود الـ structs في `Rok2Types.h`، المطابقة declaration→implementation لكل وظيفة، تطابق endpoints مع الباك اند (router.ts/KindomShard.ts الفعلي)، وقراءة king/throne/holySites في `ParseWorld`.

```bash
cd game/client-unreal && node scripts/verify_p8_client.mjs
# 58 PASSED, 0 FAILED (client-unreal)
```

## حدود هذا البند

لا يوجد UE SDK في بيئة التنفيذ (Linux sandbox)، لذلك القبول النهائي — بناء C++ بلا أخطاء وPIE — على UE 5.4.4 Windows. الشاشات الوظيفية (UMG) تُبنى في `P8-T7` كـ API Layer جاهز؛ ربطها بـ UMG ضمن مسار العميل عند توفر بيئة Unreal.

## علاقة البنود

`P8-T7` يستهلك endpoints من T1/T2/T4/T5/T6 ويرفقها بملك المملكة. لا يعيق P8-T8 (وحدات بشرية 3D) — مسار مستقل.
