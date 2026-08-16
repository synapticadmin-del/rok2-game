<#
.SYNOPSIS
  يشغّل ROK2 كتجربة Standalone قصيرة للتحقق من إقلاع المحرك وتحميل الخريطة.

.DESCRIPTION
  هذا اختبار تشغيل سريع وليس بديلاً عن Play-In-Editor. يطلق UnrealEditor.exe
  بوضع -game مع DX11/SM5 الافتراضي المتوافق، ينتظر المدة المحددة، ثم يراجع سجل
  المحرر بحثاً عن أخطاء قاتلة قبل إيقاف العملية تلقائياً.

.EXAMPLE
  .\scripts\Run-Rok2RuntimeSmoke.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,

    [ValidateRange(20, 600)]
    [int]$TimeoutSeconds = 90,

    [switch]$KeepOpen
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-UnrealEditor {
    param([string]$RequestedRoot)

    $Candidate = if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $RequestedRoot
    } else {
        'C:\Program Files\Epic Games\UE_5.4'
    }
    $Editor = Join-Path $Candidate 'Engine\Binaries\Win64\UnrealEditor.exe'
    $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
    if (-not (Test-Path $Editor) -or -not (Test-Path $VersionFile)) {
        throw "لم يتم العثور على UnrealEditor.exe أو Build.version لمحرك UE 5.4.4 في: $Candidate"
    }
    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
    if ($ActualVersion -ne '5.4.4') {
        throw "يتطلب دخان تشغيل ROK2 Unreal Engine 5.4.4، لكن المحرك المحدد هو $($ActualVersion): $Candidate"
    }
    return (Resolve-Path $Editor).Path
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Run-Rok2RuntimeSmoke.ps1 مخصص للتشغيل المحلي على Windows.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$LogPath = Join-Path $LogDirectory "runtime-smoke-$Timestamp.log"
$Editor = Resolve-UnrealEditor -RequestedRoot $EngineRoot

$Arguments = @(
    "`"$ProjectFile`"",
    '-game', '-log', '-dx11', '-sm5', '-nohmd', '-noVSync',
    '-windowed', '-ResX=1280', '-ResY=720',
    "-Abslog=`"$LogPath`""
)

Write-Host "[ROK2] Starting UE 5.4.4 standalone runtime smoke for $TimeoutSeconds seconds..." -ForegroundColor Cyan
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -PassThru
Start-Sleep -Seconds $TimeoutSeconds

if ($KeepOpen) {
    Write-Host "[ROK2] Client remains open (PID $($Process.Id)). Log: $LogPath" -ForegroundColor Yellow
    exit 0
}

if (-not $Process.HasExited) {
    Stop-Process -Id $Process.Id -Force
    $Process.WaitForExit()
}

if (-not (Test-Path $LogPath)) {
    throw "لم يُنشئ Unreal سجل التشغيل المتوقع: $LogPath"
}

$Log = Get-Content -Path $LogPath -Raw
$FatalSignatures = @(
    'Fatal error:',
    'Unhandled Exception:',
    'LogWindows: Error:'
)
$Failures = @($FatalSignatures | Where-Object { $Log.Contains($_) })
if ($Failures.Count -gt 0) {
    throw "فشل اختبار التشغيل: وُجدت بصمات خطأ قاتل ($($Failures -join ', ')). راجع $LogPath"
}

$MapLoaded = $Log -match 'Browse:.*Rok2Main|Bringing up level for play|LoadMap:'
if (-not $MapLoaded) {
    throw "لم يظهر دليل تحميل خريطة Rok2Main في السجل. راجع $LogPath"
}

Write-Host '[ROK2] Standalone runtime smoke passed: startup, map load, and no fatal signature detected.' -ForegroundColor Green
Write-Host "[ROK2] Runtime log: $LogPath" -ForegroundColor Green
Write-Host '[ROK2] نفّذ الآن قائمة PIE اليدوية في Docs/BUILD_AND_PIE.md لتأكيد واجهة وصوت P7-T1.' -ForegroundColor Yellow
