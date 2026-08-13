# P11-T4 — Great Ziggurat وحرب KvK كاملة

## نظرة عامة

Great Ziggurat هي معركة نهاية موسم KvK في Lost Kingdom. لا يمكن مهاجمتها مباشرة: يجب أولًا تدمير القلاع الأربع (Citadels) المحيطة بها، وكل قلعة مدمّرة تقصف الزيقورة بـ 25% من HP الكامل (1,000,000)، أي أن تدمير القلاع الأربع = تدمير الزيقورة بنسبة 100% ثم تُفتح نافذة المعركة النهائية.

## دورة المعركة

| المرحلة | الشرط | الأثر |
|---|---|---|
| 1. حصار القلاع | يوم 56+ | القلاع الأربع مفتوحة، كل قلعة HP = 500,000 |
| 2. قصف الزيقورة | تدمير قلعة | -25% من 1,000,000 لكل قلعة، المكافأة 500 عملة KvK للتحالف |
| 3. فتح الزيقورة | تدمير 4 قلاع | نافذة المعركة النهائية 72 ساعة |
| 4. المعركة النهائية | مهاجمة الزيقورة | تدميرها = تتويج المملكة الفائزة |
| 5. التتويج | زيقورة HP ≤ 0 | 5,000 نقطة تتويج + 2,000 عملة KvK للمملكة الفائزة |

## endpoints

| endpoint | الدور |
|---|---|
| POST /v1/lk/migrate | هجرة إلى Lost Kingdom (CH16+, 5,000 gems) |
| GET /v1/lk/state | حالة KvK الكاملة للمملكة |
| POST /v1/lk/hieron | الاستيلاء على هيرون (50 عملة + نقاط) |
| POST /v1/lk/citadel | هجوم على قلعة (نقل تقدم القصف) |
| POST /v1/lk/ziggurat | معركة نهائية على الزيقورة |
| POST /v1/lk/season-buy | شراء من متجر عملات KvK |

## متجر عملات KvK (الرمز KC)

| العنصر | السعر | المكافأة |
|---|---|---|
| ks_sculpture شظايا منحوتة | 100 KC | 500 شظية |
| kvk_speedup تسريع 8 ساعات | 60 KC | تسريع 8 ساعات |
| kvk_gems كيس جواهر | 200 KC | 300 جواهر |
| kvk_title لقب محارب KvK | 500 KC | لقب دائم |

## حماية من الغش

حدود rate جديدة (6): lk_state_read, lk_migrate, lk_hieron_capture, lk_citadel_attack, lk_ziggurat_attack, lk_season_buy — كلها في `anticheat.json` + fallbacks في `sim/anticheat.ts`. أضرار محدودة: قلعة max 50,000 damage، زيقورة max 100,000 damage لكل طلب.

## الحالة السلطوية

جدول `lk_state` (singleton لكل المملكة، migration 18) يحفظ: structures_json (20 منشأة)، citadels_json، ziggurat_json (hp/open/finalBattleStartedMs/destroyed)، migration_json، kvk_coins، crown_points، kingdom_points، season_id.

## عملة الموسم لا تنتقل

عملات KC تبقى في Legacy (P12-T2) عند نهاية الموسم — لا تُستخدم في الموسم التالي.
