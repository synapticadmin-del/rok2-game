@echo off
setlocal
REM ============================================================
REM  ROK2 - استيراد الأصول الخام (GLB/WAV/PNG) إلى .uasset
REM
REM  يعمل بدون فتح نافذة المحرر ومع -nullrhi، أي بلا أي استخدام
REM  لكرت الشاشة. هذا يحلّ مشكلة "المجسّمات البدائية" على جهاز
REM  كرت شاشته لا يدعم SM6.
REM
REM  شغّل هذا الملف *قبل* BuildAndroid.bat
REM ============================================================

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.4"
set "PROJECT=%~dp0Rok2.uproject"
set "SCRIPT=%~dp0import_assets.py"
set "MATERIALS=%~dp0create_materials.py"

set "CMD_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%CMD_EXE%" (
  echo [!] UnrealEditor-Cmd.exe غير موجود في: %CMD_EXE%
  echo     عدّل المتغير UE_ROOT اعلاه.
  pause & exit /b 1
)

echo.
echo ==== ROK2 :: Creating project materials (M_Rok2Base / M_Rok2Unlit) ====
echo.

REM مواد المشروع أولاً: مواد المحرك بلا أي VectorParameter، فبدون هاتين
REM المادتين لا يجد الكود بارامتر "Color" وتظهر القلعة والسور والمباني رمادية.
"%CMD_EXE%" "%PROJECT%" ^
  -run=pythonscript ^
  -script="%MATERIALS%" ^
  -nullrhi ^
  -unattended ^
  -nosplash ^
  -nopause ^
  -stdout ^
  -utf8output

if not "%ERRORLEVEL%"=="0" (
  echo [!] فشل توليد المواد — لا تكمل، الألوان لن تظهر.
  pause & exit /b 1
)

echo.
echo ==== ROK2 :: Importing raw assets (headless, no GPU) ====
echo.

REM مجلد لكل جلسة محرر: استيراد كل المجلدات في جلسة واحدة كان يُسقط المحرك
REM بـ `Assertion failed: IsValid()` داخل AssetTools بعد عدة مئات من الملفات.
set "RC=0"
for %%J in (
  "Art/kaykit"
  "Art/Commanders"
  "Art/WorldMapIcons"
  "Art/UIIcons"
  "Art/UIButtons"
  "Art/CityBuildingIcons"
  "Art/CivIcons"
  "Art/CivBackgrounds"
  "Art/Tavern"
  "Audio"
) do (
  echo --- %%~J ---
  set "ROK2_JOB=%%~J"
  "%CMD_EXE%" "%PROJECT%" ^
    -run=pythonscript ^
    -script="%SCRIPT%" ^
    -nullrhi ^
    -unattended ^
    -nosplash ^
    -nopause ^
    -stdout ^
    -utf8output
  if errorlevel 1 (
    echo [!] فشل استيراد %%~J
    set "RC=1"
  )
)
set "ROK2_JOB="

echo.
echo ==== ROK2 :: Reparenting glTF materials away from Interchange plugin content ====
echo.

REM مستورد glTF يربط المادة المستوردة بوالد داخل ملحق Interchange
REM (/Interchange/gltf/MaterialInstances/MI_Default_Opaque)، وذلك الملحق معلن
REM SupportedTargetPlatforms = Win64/Linux/Mac فقط، فيفشل كوك أندرويد بـ
REM "Content is missing from cook" وتسقط كل مجسمات KayKit من الـ APK.
REM لذلك تُعاد المواد المستوردة إلى مادة المشروع M_Rok2Gltf بعد كل استيراد.
"%CMD_EXE%" "%PROJECT%" ^
  -run=pythonscript ^
  -script="%~dp0scripts\reparent_gltf_materials.py" ^
  -nullrhi ^
  -unattended ^
  -nosplash ^
  -nopause ^
  -stdout ^
  -utf8output

if not "%ERRORLEVEL%"=="0" (
  echo [!] فشل فك ارتباط مواد glTF — كوك اندرويد سيفشل.
  set "RC=1"
)

echo.
if "%RC%"=="0" (
  echo ==== SUCCESS ====
  echo الأصول استُوردت إلى Content\Art و Content\Audio كـ .uasset
  echo الخطوة التالية: BuildAndroid.bat
) else (
  echo ==== FAILED (exit %RC%) ====
  echo راجع الرسائل اعلاه — ابحث عن اسطر تبدأ بـ [ROK2]
)
pause
endlocal
