# ROK2 Agent Skills Index (.agents/skills)

This directory contains the custom Agent Skills designed specifically for the `ROK2` MMO Strategy Game project in Antigravity IDE.

## Installed Skills Catalog

| Skill Name | Focus Area | Key Systems & Files Covered |
| :--- | :--- | :--- |
| **`ue5-cpp-gameplay-architect`** | UE 5.4.4 C++ Architecture & Subsystems | `URok2Api`, `URok2ApiSubsystem`, `ARok2WorldRenderer`, GC safety, `TWeakObjectPtr` |
| **`ue5-commonui-game-ux`** | CommonUI, UMG, Design Tokens, Mobile UX | `Rok2Typography.h`, `Rok2VisualTheme.h`, 8pt grid, WCAG AA, Safe-zone insets |
| **`game-economy-system-balancer`** | 4X Game Economy, Combat Formulas, JSON | `data/*.json`, troop counter matrix, progression curves, Monte-Carlo tests |
| **`mass-hism-world-performance`** | HISM, World Map Optimization, Android Perf | `ARok2WorldRenderer`, `URok2Perf`, Draw Calls (<450 on Mobile), instance pooling |
| **`rok2-backend-shard-architect`** | Cloudflare Workers, Durable Objects, D1 | `KingdomShard.ts`, `router.ts`, `world_delta` sync, security & anti-cheat |
| **`ue5-build-acceptance-pipeline`** | Engine Build, PIE 2-Client Test, Android SDK | `Build-Rok2.ps1`, `Run-Rok2RuntimeSmoke.ps1`, `Prepare-AndroidDevelopment.ps1` |
| **`game-asset-generation-architect`** | Asset prompt engineering, visual tiers, isometric rendering | `Content/Art/*`, `scripts/generate_*.py` |
| **`rok2-visual-ui-parity`** | **أسلوب P25:** مرجع RoK بصري → بناء → **التقاط ونظر** → مقارنة → حارس أخيراً | `ROK_Wiki_Assets` (1,487 أصلاً)، `Launch-Rok2Client.ps1`، `Capture-Rok2Window.ps1`، مصائد تخطيط UMG |

---

## قاعدة ملزمة لأي بند واجهة

الحارس البنيوي **يمنع الانحدار ولا ينوب عن النظر.** جلسة سابقة أبلغت «45 فحصاً،
0 فشل» على شاشة أزرارها تركب فوق نصّها وصورة قائدها شريحة مسحوقة على الحافة —
لأن الحارس يفحص أن `SetWidthOverride` مكتوب، لا أن الناتج سليم.

لا يُكتب حارس واجهة قبل رؤية لقطة الشاشة الفعلية. التفصيل والخطوات الخمس في
`rok2-visual-ui-parity/SKILL.md`.

---
*Created and registered for Antigravity IDE.*
