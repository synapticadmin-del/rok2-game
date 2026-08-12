<#
.SYNOPSIS
  يستورد أصول شاشة اختيار الحضارات كـ Texture2D داخل مشروع Unreal.

.DESCRIPTION
  ينشئ إعداد ImportAssetsCommandlet موقتاً ويستورد خلفيات الحضارات الست
  وشعاراتها التشغيلية وصور القادة المبدئية الستة ثم بورتريهات القادة الستة
  الإضافيين من roster البيانات (cmd_*_2). يمكن تكراره بأمان بعد تحديث PNG.

.EXAMPLE
  .\scripts\Import-CivVisuals.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ReplaceExisting
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
    $EditorCmd = Join-Path $Candidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
    if (-not (Test-Path $EditorCmd) -or -not (Test-Path $VersionFile)) {
        throw "لم يُعثر على UnrealEditor-Cmd.exe أو Build.version لمحرك UE 5.4.4 في: $Candidate"
    }
    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
    if ($ActualVersion -ne '5.4.4') {
        throw "يتطلب استيراد أصول ROK2 Unreal Engine 5.4.4، لكن المحرك المحدد هو $ActualVersion: $Candidate"
    }
    return (Resolve-Path $Candidate).Path
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Import-CivVisuals.ps1 مخصص للتشغيل المحلي على Windows مع Unreal Engine.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$Engine = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Civs = @('rome', 'china', 'arabia', 'egypt', 'vikings', 'japan')

function New-ImportGroup {
    param([string]$Name, [string]$Destination, [string[]]$Files)
    return @{
        GroupName = $Name
        DestinationPath = $Destination
        FactoryName = 'TextureFactory'
        FileNames = $Files
        bReplaceExisting = [bool]$ReplaceExisting
        bSkipReadOnly = $true
    }
}

$Backgrounds = $Civs | ForEach-Object { Join-Path $ProjectRoot "Content\Art\CivBackgrounds\bg_$_.png" }
$Icons = $Civs | ForEach-Object { Join-Path $ProjectRoot "Content\Art\CivIcons\icon_$($_)_runtime.png" }
$Commanders = $Civs | ForEach-Object { Join-Path $ProjectRoot "Content\Art\Commanders\cmd_$($_)_starter.png" }
$CmdRoster2 = $Civs | ForEach-Object { Join-Path $ProjectRoot "Content\Art\Commanders\cmd_$($_)_2.png" }
foreach ($AssetPath in @($Backgrounds + $Icons + $Commanders + $CmdRoster2)) {
    if (-not (Test-Path $AssetPath)) { throw "الأصل المطلوب غير موجود: $AssetPath" }
}

$ConfigDirectory = Join-Path $ProjectRoot 'Saved\ImportConfigs'
New-Item -ItemType Directory -Path $ConfigDirectory -Force | Out-Null
$ConfigPath = Join-Path $ConfigDirectory 'civ-visuals-import.json'
$Config = @{
    ImportGroups = @(
        (New-ImportGroup -Name 'CivBackgrounds' -Destination '/Game/Art/CivBackgrounds' -Files $Backgrounds),
        (New-ImportGroup -Name 'CivIcons' -Destination '/Game/Art/CivIcons' -Files $Icons),
        (New-ImportGroup -Name 'CivCommanders' -Destination '/Game/Art/Commanders' -Files $Commanders),
        (New-ImportGroup -Name 'CivCommanders2' -Destination '/Game/Art/Commanders' -Files $CmdRoster2)
    )
}
$Config | ConvertTo-Json -Depth 8 | Set-Content -Path $ConfigPath -Encoding UTF8

$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$LogPath = Join-Path $LogDirectory ("import-civ-visuals-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host "[ROK2] Importing 24 civilization visuals with UE 5.4.4: $Engine" -ForegroundColor Cyan
& $EditorCmd $ProjectFile '-run=ImportAssets' "-importSettings=$ConfigPath" '-nosourcecontrol' '-unattended' '-nop4' 2>&1 | Tee-Object -FilePath $LogPath
if ($LASTEXITCODE -ne 0) {
    throw "فشل استيراد أصول الحضارات (رمز: $LASTEXITCODE). راجع: $LogPath"
}
Write-Host "[ROK2] Civilization visuals imported. Log: $LogPath" -ForegroundColor Green
