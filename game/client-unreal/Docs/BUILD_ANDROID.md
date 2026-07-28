# Build Android APK — خطوات مفصّلة

## المتطلبات
1. Unreal Engine 5.4 مع Android components (SetupAndroid)
2. Android Studio (NDK r25c, SDK Platform 34)
3. JDK 17

## خطوات
1. افتح المشروع في UE Editor
2. تأكد من Project Settings → Android SDK paths صحيحة
3. Platforms → Android:
   - Package Name: `com.rok2.thrones`
   - Orientation: Portrait
   - Min SDK: 24
   - Target SDK: 34
   - Vulkan: ✅
4. File → Package Project → Android → Android (ASTC)
5. انتظر البناء (10–30 دقيقة أول مرة)
6. المخرجات:
```
client-unreal\Binaries\Android\Rok2-arm64.apk
```

## تثبيت على الهاتف
- فعّل USB Debugging على أندرويد
- انسخ APK للهاتف وثبّته
- أو:
```bash
adb install -r "C:\Users\kayf\Desktop\rok2\game\client-unreal\Binaries\Android\Rok2-arm64.apk"
```

## ملاحظات
- المشروع يطلب صلاحيات: INTERNET
- أصول Engine basic shapes لتقليل حجم APK الأول
- عند إضافة فن احترافي: ضعه في `Content/Art/` واربطه في `Rok2CityBuilder`
