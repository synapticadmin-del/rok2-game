# حل مشكلة "الشاشة السوداء المنقّطة بالأبيض" (تشويش كشاشة التلفزيون)

## ما هو السبب؟

هذا التشويش **ليس خطأً في كود اللعبة** — هو ناتج عن أن المحرك يحاول الرسم
عبر مسار **DirectX 12 / Shader Model 6 (SM6)** بينما كرت الشاشة أو التعريف
لا يدعمه فعلياً. النتيجة: buffer غير مُهيّأ يُعرض كضجيج أبيض على أسود.

المشروع مضبوط على `EngineAssociation: 5.8`، و UE5 يستخدم SM6 افتراضياً.
مميزات مثل **Nanite** و **Lumen** و **Virtual Shadow Maps** تتطلب SM6 إلزامياً.

## ما تم إصلاحه في المستودع

في `Config/DefaultEngine.ini`:

1. **إضافة `TargetedShaderFormats` صراحةً** — كان `DefaultGraphicsRHI_DX11`
   موجوداً لكن بدون أسطر `+D3D11TargetedShaderFormats=PCD3D_SM5`، فيظل
   المحرك يبني ويستخدم SM6. هذا هو السبب الجذري الأرجح.
2. **تعطيل Nanite / SkinCache / RayTracing / Substrate على مستوى المشروع**
   (كانت معطّلة في `[SystemSettings]` فقط، وهي لا تمنع بناء الـ shaders).
3. حذف قسم وهمي `[/Script/WindowsTargetEditor.WindowsTargetSettings]`
   (اسم غير موجود في المحرك — كان بلا أي تأثير).
4. دمج قسم `[/Script/Engine.Engine]` المكرّر (المكرر كان يلغي السطر الأول).

كذلك أُضيف `RunEditor_SafeMode.bat` لتشغيل المحرر على DX11/SM5 مباشرة.

## خطوات التنفيذ عندك (بالترتيب)

1. **احذف مجلدات الكاش** ثم افتح المشروع — إلزامي بعد تغيير shader formats:
   ```
   game\client-unreal\Binaries
   game\client-unreal\Intermediate
   game\client-unreal\DerivedDataCache
   game\client-unreal\Saved
   ```
2. شغّل `RunEditor_SafeMode.bat` (بعد ضبط `UE_PATH` بداخله).
3. انتظر إعادة بناء الـ shaders (قد تستغرق وقتاً طويلاً أول مرة).

## إن استمرت المشكلة

- **حدّث تعريف كرت الشاشة** من موقع الشركة المصنّعة مباشرة (لا Windows Update).
- جرّب Vulkan بدل DX11: أضف `-vulkan` بدل `-dx11` في ملف الـ bat.
- تحقق من دعم SM6: شغّل `dxdiag` → تبويب Display → قيمة
  *Feature Levels*. إن لم تجد `12_1` فالكرت لا يدعم SM6.
- **الحد الأدنى الواقعي** لتشغيل محرر UE5: كرت مستقل بذاكرة 4GB+ يدعم
  DX12 Feature Level 12_1. كروت Intel HD المدمجة (530 وما قبلها) لن تعطي
  نتيجة مستقرة مهما ضُبطت الإعدادات.

## تمييز نوع المشكلة

| ما تراه | السبب المرجّح |
|---------|----------------|
| تشويش أبيض/أسود في **نافذة المحرر نفسها** | مشكلة RHI/SM6 — اتبع ما سبق |
| المحرر سليم، لكن الشاشة سوداء عند **Play** | لا يوجد PlayerStart في المستوى — شغّل `setup_level.py` |
| شاشة سوداء + لا واجهة | فشل اتصال الـ API — راجع Output Log |
