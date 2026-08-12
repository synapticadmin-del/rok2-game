<#
.SYNOPSIS
  يفحص جاهزية Android Development لمشروع ROK2 على Windows مع Unreal Engine 5.4.4.

.DESCRIPTION
  يتحقق من نسخة المحرك وإعدادات Android في المشروع، ثم يشغّل Turnkey VerifySdk.
  لا ينزّل أي مكوّن افتراضياً. مرر -InstallSdk فقط على جهاز Windows المقصود لبدء
  Android Studio/Turnkey التفاعلي. لا تحفظ مسارات SDK أو سجلات الجهاز في Git.

.EXAMPLE
  .\scripts\Prepare-AndroidDevelopment.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Prepare-AndroidDevelopment.ps1 -InstallSdk
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$InstallSdk
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-UnrealEngineRoot {
    param([string]$RequestedRoot)

    $Candidate = if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $RequestedRoot
    } else {
        'C:\Program Files\Epic Games\UE_5.4'
    }
    $RunUat = Join-Path $Candidate 'Engine\Build\BatchFiles\RunUAT.bat'
    $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
    if (-not (Test-Path $RunUat) -or -not (Test-Path $VersionFile)) {
        throw "لم يتم العثور على UE 5.4.4 في: $Candidate. ثبّت المحرك ثم مرر -EngineRoot أو عيّن UE_ROOT."
    }

    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
    if ($ActualVersion -ne '5.4.4') {
        throw "يتطلب إعداد Android الخاص بـ ROK2 UE 5.4.4، لكن المحرك المحدد هو $ActualVersion."
    }
    return (Resolve-Path $Candidate).Path
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Prepare-AndroidDevelopment.ps1 مخصص لجهاز Windows عليه UE 5.4.4.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$EngineConfig = Join-Path $ProjectRoot 'Config\DefaultEngine.ini'
if (-not (Test-Path $ProjectFile) -or -not (Test-Path $EngineConfig)) {
    throw 'تعذر العثور على Rok2.uproject أو Config\DefaultEngine.ini.'
}

$ConfigText = Get-Content -Path $EngineConfig -Raw
$RequiredConfig = @(
    '[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]',
    'TargetSDKVersion=34',
    'MinSDKVersion=26',
    'bSupportsVulkan=True',
    'AndroidOrientation=Landscape',
    'BuildConfiguration=PPBC_Development'
)
foreach ($Contract in $RequiredConfig) {
    if (-not $ConfigText.Contains($Contract)) {
        throw "إعداد Android المطلوب مفقود من DefaultEngine.ini: $Contract"
    }
}

$ResolvedEngineRoot = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
$RunUat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'

Write-Host "[ROK2] Engine: UE 5.4.4 — $ResolvedEngineRoot" -ForegroundColor Cyan
Write-Host '[ROK2] Project Android settings: SDK 34, min SDK 26, Vulkan, Landscape, Development.' -ForegroundColor Cyan

if ($InstallSdk) {
    Write-Host '[ROK2] Starting Turnkey Android SDK installation. Follow the Android Studio wizard, enable Android SDK Command-Line Tools (latest), then close Android Studio so Turnkey can finish.' -ForegroundColor Yellow
    & $RunUat 'Turnkey' '-Command=InstallSDK' 'Platform=Android'
    if ($LASTEXITCODE -ne 0) {
        throw "فشل Turnkey InstallSDK (رمز الخروج: $LASTEXITCODE)."
    }
    Write-Host '[ROK2] سجّل الخروج من Windows ثم سجّل الدخول مجدداً قبل تنفيذ VerifySdk.' -ForegroundColor Yellow
}

Write-Host '[ROK2] Verifying Android SDK with Turnkey.' -ForegroundColor Cyan
& $RunUat 'Turnkey' '-Command=VerifySdk' 'Platform=Android'
if ($LASTEXITCODE -ne 0) {
    throw "فشل Turnkey VerifySdk (رمز الخروج: $LASTEXITCODE). شغّل السكربت مع -InstallSdk أو أصلح إعداد Android Studio ثم أعد المحاولة."
}

Write-Host '[ROK2] Android Development environment verified successfully.' -ForegroundColor Green
