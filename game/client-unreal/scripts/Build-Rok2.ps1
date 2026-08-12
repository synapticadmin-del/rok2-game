<#
.SYNOPSIS
  يبني عميل ROK2 من سطر الأوامر باستخدام Unreal Engine 5.4.4 المثبت محلياً.

.DESCRIPTION
  يدعم بناء Rok2Editor للتحقق قبل PIE، وبناء Rok2 Development/Shipping لنظام
  Windows، والتحزيم الاختياري عبر Unreal Automation Tool. لا يغير ملفات المشروع
  ولا يمسح ملفات وسيطة إلا عند تمرير -Clean صراحةً.

.EXAMPLE
  .\scripts\Build-Rok2.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Build-Rok2.ps1 -Target Development -Package -OutputDirectory 'D:\Builds\ROK2'
#>
[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Development', 'Shipping')]
    [string]$Target = 'Editor',

    [ValidateSet('Win64')]
    [string]$Platform = 'Win64',

    [string]$EngineRoot = $env:UE_ROOT,

    [switch]$Clean,

    [switch]$Package,

    [switch]$ImportCivVisuals,

    [switch]$ImportCityMapUiAssets,

    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-UnrealEngineRoot {
    param([string]$RequestedRoot)

    # المشروع مقفل على UE 5.4.4 كي تظل نتائج البناء وإعادة الاستيراد قابلة للتكرار.
    $Candidate = if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $RequestedRoot
    } else {
        'C:\Program Files\Epic Games\UE_5.4'
    }
    $BuildBat = Join-Path $Candidate 'Engine\Build\BatchFiles\Build.bat'
    $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
    if (-not (Test-Path $BuildBat) -or -not (Test-Path $VersionFile)) {
        throw @"
لم يتم العثور على Unreal Engine 5.4.4 في المسار: $Candidate
ثبّت UE 5.4.4 عبر Epic Games Launcher، ثم مرر -EngineRoot أو عيّن UE_ROOT، مثال:
`$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.4'
"@
    }

    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
    if ($ActualVersion -ne '5.4.4') {
        throw "يتطلب ROK2 Unreal Engine 5.4.4، لكن المحرك المحدد هو $ActualVersion: $Candidate"
    }
    return (Resolve-Path $Candidate).Path
}

function Invoke-UnrealBatchFile {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$LogPath
    )

    "`n> $FilePath $($Arguments -join ' ')" | Tee-Object -FilePath $LogPath -Append
    & $FilePath @Arguments 2>&1 | Tee-Object -FilePath $LogPath -Append
    if ($LASTEXITCODE -ne 0) {
        throw "فشل Unreal بالأمر أعلاه (رمز الخروج: $LASTEXITCODE). راجع السجل: $LogPath"
    }
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Build-Rok2.ps1 مخصص للبناء المحلي على Windows مع Unreal Engine وVisual Studio C++ toolchain.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
if (-not (Test-Path $ProjectFile)) {
    throw "ملف المشروع غير موجود: $ProjectFile"
}

$ResolvedEngineRoot = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
if ($ImportCivVisuals) {
    $ImportScript = Join-Path $PSScriptRoot 'Import-CivVisuals.ps1'
    Write-Host '[ROK2] Importing civilization visuals before build.' -ForegroundColor Cyan
    & $ImportScript -EngineRoot $ResolvedEngineRoot -ReplaceExisting
    if ($LASTEXITCODE -ne 0) {
        throw "فشل استيراد أصول الحضارات قبل البناء (رمز الخروج: $LASTEXITCODE)."
    }
}
if ($ImportCityMapUiAssets) {
    $ImportScript = Join-Path $PSScriptRoot 'Import-CityMapUIAssets.ps1'
    Write-Host '[ROK2] Importing city/map/UI PNG assets before build.' -ForegroundColor Cyan
    & $ImportScript -EngineRoot $ResolvedEngineRoot -ReplaceExisting
    if ($LASTEXITCODE -ne 0) {
        throw "فشل استيراد أصول المدينة والخريطة والواجهة قبل البناء (رمز الخروج: $LASTEXITCODE)."
    }
}
$BuildBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
$RunUatBat = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$BuildLog = Join-Path $LogDirectory "build-$Target-$Timestamp.log"

Write-Host "[ROK2] Engine: UE 5.4.4 — $ResolvedEngineRoot" -ForegroundColor Cyan
Write-Host "[ROK2] Project: $ProjectFile" -ForegroundColor Cyan
Write-Host "[ROK2] Target: $Target ($Platform)" -ForegroundColor Cyan

if ($Clean) {
    foreach ($Directory in @('Binaries', 'Intermediate')) {
        $Path = Join-Path $ProjectRoot $Directory
        if (Test-Path $Path) {
            Write-Host "[ROK2] Removing $Path" -ForegroundColor Yellow
            Remove-Item -Path $Path -Recurse -Force
        }
    }
}

if ($Target -eq 'Editor') {
    $BuildArguments = @('Rok2Editor', $Platform, 'Development', $ProjectFile, '-WaitMutex', '-NoHotReload')
} else {
    $BuildArguments = @('Rok2', $Platform, $Target, $ProjectFile, '-WaitMutex', '-NoHotReload')
}
Invoke-UnrealBatchFile -FilePath $BuildBat -Arguments $BuildArguments -LogPath $BuildLog

if ($Package) {
    if ($Target -eq 'Editor') {
        throw 'التحزيم يتطلب -Target Development أو -Target Shipping، وليس Editor.'
    }
    if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
        $OutputDirectory = Join-Path $ProjectRoot "Artifacts\Win64-$Target"
    }
    $ResolvedOutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
    $PackageLog = Join-Path $LogDirectory "package-$Target-$Timestamp.log"
    $UatArguments = @(
        'BuildCookRun',
        "-project=$ProjectFile",
        '-noP4',
        '-platform=Win64',
        "-clientconfig=$Target",
        '-build', '-cook', '-stage', '-pak', '-iostore', '-archive',
        "-archivedirectory=$ResolvedOutputDirectory"
    )
    Invoke-UnrealBatchFile -FilePath $RunUatBat -Arguments $UatArguments -LogPath $PackageLog
    Write-Host "[ROK2] Package completed: $ResolvedOutputDirectory" -ForegroundColor Green
    Write-Host "[ROK2] Package log: $PackageLog" -ForegroundColor Green
}

Write-Host "[ROK2] Build completed successfully." -ForegroundColor Green
Write-Host "[ROK2] Build log: $BuildLog" -ForegroundColor Green
