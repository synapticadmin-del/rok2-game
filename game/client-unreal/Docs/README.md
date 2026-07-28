# ROK2 — Unreal Engine Client

## المسار
```
C:\Users\kayf\Desktop\rok2\game\client-unreal
```

## المحرك
- Unreal Engine 5.8
- C++ module `Rok2`
- أهداف: Windows + Android (APK)

## بنية الكود
```
Source/
├── Rok2Target.cs            # Game target
├── Rok2EditorTarget.cs      # Editor target
└── Rok2/
    ├── Rok2.Build.cs        # Module deps (HTTP/WebSockets/Json/UMG)
    ├── Rok2.cpp             # Module impl
    └── Public/ Private/
        ├── Rok2Types.h      # DTOs (Player/City/Pass/...)
        ├── Rok2Api.h/.cpp   # Cloudflare HTTP + WS client
        ├── Rok2GameMode     # Entry + tick + Api singleton
        ├── Rok2PlayerController   # Input + camera control
        ├── Rok2IsometricCamera   # Strategy camera (pitch -50°)
        ├── Rok2CityBuilder       # Isometric city tiles + buildings
        ├── Rok2WorldRenderer     # Map markers (cities/passes/nodes)
        ├── Rok2ProceduralAssets  # Runtime-tinted materials (no texture files needed)
        ├── Rok2BootWidget        # Login + civ select UI
        └── Rok2CityWidget       # Resources/buildings/train/alliance HUD
```

## التشغيل على جهازك
1. ثبت Unreal Engine 5.4 (من Epic Games Launcher)
2. افتح `Rok2.uproject` بنقرة مزدوجة
3. أول مرة: UE يسأل لو عايز نسخة المحرك — اختر 5.4
4. انتظر توليد project files + compile
5. في Editor: زر **Play** للاختبار في الـ viewport
6. للبناء على Android:
   - Edit → Project Settings → Android SDK (تأكد من Android Studio + NDK)
   - Platforms → Android → Package Project → APK
7. APK سيتولّد في:
   ```
   C:\Users\kayf\Desktop\rok2\game\client-unreal\Binaries\Android\
   ```
8. انقل APK لهاتفك وثبّته

## API المتصل
- Base: `https://rok2-api.lolelarap.workers.dev`
- WS: `wss://.../v1/world/ws`
- مفتاح admin للـ force tick: `rok2-dev-admin`

## ما الذي يعمل
- Login guest + init city
- إظهار الموارد والمباني
- تدريب جنود
- إنشاء تحالف
- خريطة العالم: مدن/ممرات/موارد كـ 3D markers
- كاميرا isometric بتحكم WASD + Wheel
- WebSocket live updates (optional)
- قتال الممرات عبر API

## أصول الفن
- حاليًا: Engine basic shapes (Cube/Sphere/Cylinder) + materials ملوّنة runtime
- لترقية: ضع Static Meshes احترافية في `Content/Art/Buildings/`
  أو استورد موديلات من Quaternius/Kenney (مجانية)
