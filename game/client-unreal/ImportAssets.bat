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

set "CMD_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%CMD_EXE%" (
  echo [!] UnrealEditor-Cmd.exe غير موجود في: %CMD_EXE%
  echo     عدّل المتغير UE_ROOT اعلاه.
  pause & exit /b 1
)

echo.
echo ==== ROK2 :: Importing raw assets (headless, no GPU) ====
echo.

"%CMD_EXE%" "%PROJECT%" ^
  -run=pythonscript ^
  -script="%SCRIPT%" ^
  -nullrhi ^
  -unattended ^
  -nosplash ^
  -nopause ^
  -stdout ^
  -utf8output ^
  -FullStdOutLogOutput

set "RC=%ERRORLEVEL%"
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
