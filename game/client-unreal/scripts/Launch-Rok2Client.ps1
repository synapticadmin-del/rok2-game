<#
.SYNOPSIS
  يشغّل عميل ROK2 غير المطبوخ في نافذة، جاهزاً للالتقاط.

.DESCRIPTION
  `Binaries/Win64/Rok2.exe` المبنيّ يسقط بـ`Failed to initialize ShaderCodeLibrary`
  لأن المشروع غير مطبوخ ولا مكتبة shaders عالمية معه. المسار العامل هو تشغيل
  المحرك في وضع `-game` على ملف المشروع مباشرة.

  و`-NoLiveCoding` ضروري: خادم Live Coding يسجّل نفسه ويحجز الوحدة، فيمنع أي
  بناء تالٍ ويُبقي عملية معلّقة بعد الإغلاق.

.EXAMPLE
  .\scripts\Launch-Rok2Client.ps1
  .\scripts\Launch-Rok2Client.ps1 -Width 1280 -Height 800 -WaitSeconds 90
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.4',
    [int]$Width = 1280,
    [int]$Height = 800,
    [int]$WaitSeconds = 75,
    [switch]$WithSound
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$EditorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$VersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (-not (Test-Path $EditorExe) -or -not (Test-Path $VersionFile)) {
    throw "لم يُعثر على UnrealEditor.exe أو Build.version في: $EngineRoot"
}
$Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
$ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
if ($ActualVersion -ne '5.4.4') {
    throw "يتطلب تشغيل عميل ROK2 محرك 5.4.4، والموجود ${ActualVersion}: $EngineRoot"
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
if (-not (Test-Path $ProjectFile)) { throw "ملف المشروع غير موجود: $ProjectFile" }

# نافذة سابقة تُغلق أولاً: نافذتان تحملان العنوان نفسه تجعلان الالتقاط عشوائياً.
Get-Process UnrealEditor -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowTitle -like 'Rok2*' } |
    ForEach-Object {
        Write-Host "[ROK2] إغلاق نافذة عميل سابقة (PID $($_.Id))" -ForegroundColor DarkYellow
        $_.CloseMainWindow() | Out-Null
        Start-Sleep -Milliseconds 1500
        if (-not $_.HasExited) { $_.Kill() }
    }

$Arguments = @(
    "`"$ProjectFile`"", '-game', '-windowed',
    "-resx=$Width", "-resy=$Height",
    '-NoLiveCoding', '-log'
)
if (-not $WithSound) { $Arguments += '-nosound' }

Write-Host "[ROK2] تشغيل العميل ${Width}x${Height} على UE $ActualVersion" -ForegroundColor Cyan
Start-Process -FilePath $EditorExe -ArgumentList $Arguments | Out-Null

# الشاشة تحتاج زمناً حتى تُبنى: تحميل الوحدة + الخريطة + طلب /v1/meta/all.
# الانتظار الأعمى هنا لأن العميل لا يبثّ إشارة جاهزية إلى الخارج.
Write-Host "[ROK2] انتظار $WaitSeconds ثانية حتى تُبنى الشاشة..." -ForegroundColor DarkGray
$deadline = (Get-Date).AddSeconds($WaitSeconds)
$found = $null
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds 3
    $found = Get-Process UnrealEditor -ErrorAction SilentlyContinue |
             Where-Object { $_.MainWindowTitle -like 'Rok2*' } | Select-Object -First 1
    if ($found) { break }
}
if (-not $found) {
    throw "لم تظهر نافذة العميل خلال $WaitSeconds ثانية. راجع: $ProjectRoot\Saved\Logs\Rok2.log"
}

# النافذة ظهرت، لكن الشاشة قد تكون ما زالت تنتظر بيانات الخادم.
Start-Sleep -Seconds 12
Write-Host "[ROK2] العميل جاهز: $($found.MainWindowTitle) (PID $($found.Id))" -ForegroundColor Green
Write-Host "[ROK2] التقط بـ: .\scripts\Capture-Rok2Window.ps1 -Name <اسم>" -ForegroundColor Green
