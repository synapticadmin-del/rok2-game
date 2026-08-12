<#
.SYNOPSIS
  يستورد أصول شاشة اختيار الحضارات كـ Texture2D داخل مشروع Unreal.

.DESCRIPTION
  ينشئ إعداد ImportAssetsCommandlet موقتاً ويستورد فقط خلفيات الحضارات الست
  وشعاراتها التشغيلية وصور القادة المبدئية. يمكن تكراره بأمان بعد تحديث PNG.

.EXAMPLE
  .\scripts\Import-CivVisuals.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
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
    $Candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) { $Candidates += $RequestedRoot }
    foreach ($Version in @('5.8', '5.7', '5.6', '5.5', '5.4')) {
        $Candidates += "C:\Program Files\Epic Games\UE_$Version"
    }
    foreach ($Candidate in $Candidates | Select-Object -Unique) {
        if (Test-Path (Join-Path $Candidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe')) {
            return (Resolve-Path $Candidate).Path
        }
    }
    throw 'لم يُعثر على UnrealEditor-Cmd.exe. مرر -EngineRoot أو عيّن UE_ROOT لمحرك UE 5.4+.'
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
foreach ($AssetPath in @($Backgrounds + $Icons + $Commanders)) {
    if (-not (Test-Path $AssetPath)) { throw "الأصل المطلوب غير موجود: $AssetPath" }
}

$ConfigDirectory = Join-Path $ProjectRoot 'Saved\ImportConfigs'
New-Item -ItemType Directory -Path $ConfigDirectory -Force | Out-Null
$ConfigPath = Join-Path $ConfigDirectory 'civ-visuals-import.json'
$Config = @{
    ImportGroups = @(
        (New-ImportGroup -Name 'CivBackgrounds' -Destination '/Game/Art/CivBackgrounds' -Files $Backgrounds),
        (New-ImportGroup -Name 'CivIcons' -Destination '/Game/Art/CivIcons' -Files $Icons),
        (New-ImportGroup -Name 'CivCommanders' -Destination '/Game/Art/Commanders' -Files $Commanders)
    )
}
$Config | ConvertTo-Json -Depth 8 | Set-Content -Path $ConfigPath -Encoding UTF8

$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$LogPath = Join-Path $LogDirectory ("import-civ-visuals-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host "[ROK2] Importing 18 civilization visuals with $Engine" -ForegroundColor Cyan
& $EditorCmd $ProjectFile '-run=ImportAssets' "-importSettings=$ConfigPath" '-nosourcecontrol' '-unattended' '-nop4' 2>&1 | Tee-Object -FilePath $LogPath
if ($LASTEXITCODE -ne 0) {
    throw "فشل استيراد أصول الحضارات (رمز: $LASTEXITCODE). راجع: $LogPath"
}
Write-Host "[ROK2] Civilization visuals imported. Log: $LogPath" -ForegroundColor Green
