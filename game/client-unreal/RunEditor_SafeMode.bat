@echo off
REM ============================================================
REM  ROK2 - Safe Mode Editor Launcher
REM  يشغّل محرر Unreal على مسار DX11 / Shader Model 5 فقط،
REM  ويعطّل كل ما يسبب "الشاشة السوداء المنقّطة" على كروت
REM  الشاشة الضعيفة أو المدمجة (Intel HD/UHD).
REM
REM  الاستخدام:
REM   1) عدّل UE_PATH بمسار محرر Unreal عندك.
REM   2) شغّل الملف بالنقر المزدوج.
REM ============================================================

set "UE_PATH=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%~dp0Rok2.uproject"

if not exist "%UE_PATH%" (
  echo [!] لم يتم العثور على محرر Unreal في:
  echo     %UE_PATH%
  echo     عدّل المتغير UE_PATH داخل هذا الملف.
  pause
  exit /b 1
)

echo Launching ROK2 in SAFE MODE (DX11 / SM5)...
start "" "%UE_PATH%" "%PROJECT%" ^
  -dx11 ^
  -sm5 ^
  -nohmd ^
  -noVSync ^
  -ForceRes ^
  -windowed -ResX=1280 -ResY=720 ^
  -ExecCmds="r.ScreenPercentage 100, r.PostProcessAAQuality 0, r.DefaultFeature.AutoExposure 0, r.Nanite 0, r.Lumen.DiffuseIndirect.Allow 0, r.Shadow.Virtual.Enable 0, viewmode lit"
