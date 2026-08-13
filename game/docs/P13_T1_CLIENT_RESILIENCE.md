# P13-T1..T5: موثوقية العميل (Client Resilience Pack)

**التاريخ:** أغسطس 2026 — **الحالة:** مكتملة ومرفوعة ✅

## الهدف

رفع موثوقية عميل UE5 في ظروف الشبكة غير المستقرة (شائع جدًا على أندرويد): لا تضيع رسائل الدردشة عند انقطاع WebSocket، يُكتشف الانقطاع الصامت ويعاد الاتصال تلقائيًا، وتفتح اللعبة فورًا من كاش محلي لبيانات التوازن بدل انتظار الخادم.

## البنود المنفذة

### P13-T1: صندوق واردات WebSocket (WsOutbox)

| المكون | الوصف |
|---|---|
| `TArray WsOutbox` | طابور رسائل مُتراكمة حتى حد 128 (حماية تراكم) |
| `EnqueueWsMessage()` | تخزين رسالة JSON في الطابور بدل فقدها |
| `FlushWsOutbox()` | إرسال كل الرسائل المتراكمة عند عودة الاتصال (يُنادى في `OnConnected` بعد رسالة الترحيب) |
| `SendChat` | عند `!bWsConnected` تُخزن الرسالة وتُشعر اللاعب، لا تُسقط صامتًا |

### P13-T2: كاش بيانات التوازن المحلي

| المكون | الوصف |
|---|---|
| `rok2_meta_cache.json` | في `ProjectSavedDir` (لا hard-coded paths) |
| `SaveMetaCache()` | بعد نجاح `FetchMeta` — وحدات/مبانٍ/معاملات الإنتاج ومواهب |
| `LoadMetaCache()` | في `Init` قبل `FetchMeta` — أول فتح بلا انتظار كامل مع إعادة حساب المعدلات |
| `OnMetaLoaded` | يُبنى بثلاث حالات: `true` من الخادم، `false` من الكاش |

### P13-T3: نبض WS + watchdog للانقطاع الصامت

| الثابت | القيمة | المعنى |
|---|---|---|
| `WsHeartbeatIntervalSeconds` | 30 | نبض دوري `{"type":"heartbeat"}` |
| `WsSilentDisconnectThresholdSeconds` | 90 | إن لم تصل رسالة/نبض رد خلال 90 ثانية: إغلاق + إعادة اتصال + استعادة الحالة (`bRestoreOnNextWsConnection = true`) |
| `WsLastMessageAt` | يصفَّر عند كل رسالة واردة وكل نبض مرسل |

### P13-T4: إشعار Outbox للمستخدم

عند تخزين رسالة، يُعرض: *"الرسالة محفوظة مؤقتًا — ستُرسل تلقائيًا عند عودة الاتصال الحي"* (6 ثوانٍ).

### P13-T5: نظافة التصميم

لا ثوابت hard-coded في cpp (كل الثوابت UPROPERTY قابلة للتعديل من Blueprint/CDO)، ولا نسخ مكررة، ولا `this` capture في lambdas (نمط WeakThis قائم).

## حارس الجودة

`game/client-unreal/scripts/verify_p13_client.mjs` — 40 فحصًا بنيويًا (outbox، heartbeat، watchdog، meta cache، notifications، no hard-coded).

`package.json`: `test:p13-client` في سلسلة `npm run check` (بعد test:p12-client).

## التحقق من التوافق

- كل الأنواع/الـ declarations مضافة في `Rok2Api.h` و`Rok2Types.h` (نمط UPROPERTY BlueprintReadWrite قائم)
- includes: `Misc/FileHelper.h`, `HAL/PlatformFilemanager.h` (مسار `ProjectSavedDir` مستخدم أصلًا في Init)
- التواؤم مع الخادم: رسالة `heartbeat` مقبولة في الـ backend WebSocket handler دون تغيير (رسائل غير معروفة تُتجاهل بأمان)

## ملفات معدلة

`game/client-unreal/Source/Rok2/Public/Rok2Api.h`, `game/client-unreal/Source/Rok2/Private/Rok2Api.cpp`, `game/client-unreal/scripts/verify_p13_client.mjs`, `game/backend/package.json`, `PLAN.md`.
