@echo off
setlocal
REM ============================================================
REM  ROK2 - Android APK build (headless / بدون فتح المحرر)
REM
REM  لماذا سطر الأوامر؟ لأن التغليف عملية CPU بالكامل (كتابة
REM  الأصول + بناء shaders أندرويد)، فلا تمر على كرت الشاشة
REM  ولا تتأثر بمشكلة التشويش في نافذة المحرر.
REM
REM  عدّل UE_ROOT فقط، ثم شغّل الملف.
REM ============================================================

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.4"
set "PROJECT=%~dp0Rok2.uproject"
set "ARCHIVE=%~dp0Build\Android"

set "UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%UAT%" (
  echo [!] RunUAT.bat غير موجود في: %UAT%
  echo     عدّل المتغير UE_ROOT اعلاه.
  pause & exit /b 1
)

if "%ANDROID_HOME%"=="" (
  echo [!] متغير البيئة ANDROID_HOME غير مضبوط.
  echo     شغّل اولاً: "%UE_ROOT%\Engine\Extras\Android\SetupAndroid.bat"
  pause & exit /b 1
)

if not exist "%ARCHIVE%" mkdir "%ARCHIVE%"

echo.
echo ==== ROK2 :: Android build (Development, arm64, ASTC) ====
echo Project : %PROJECT%
echo Output  : %ARCHIVE%
echo.

call "%UAT%" BuildCookRun ^
  -project="%PROJECT%" ^
  -platform=Android ^
  -cookflavor=ASTC ^
  -clientconfig=Development ^
  -targetplatform=Android ^
  -build -cook -stage -package -pak -iostore -compressed ^
  -archive -archivedirectory="%ARCHIVE%" ^
  -nodebuginfo ^
  -utf8output ^
  -nop4 -unattended -nullrhi

set "RC=%ERRORLEVEL%"
echo.
if "%RC%"=="0" (
  echo ==== SUCCESS ====
  echo الـ APK في: %ARCHIVE%
  echo للتثبيت عبر USB:  adb install -r "%ARCHIVE%\Rok2-arm64.apk"
) else (
  echo ==== FAILED (exit %RC%) ====
  echo راجع السجل في: %USERPROFILE%\AppData\Roaming\Unreal Engine\AutomationTool\Logs
)
pause
endlocal
