<#
.SYNOPSIS
  يستورد حزمة أيقونات خريطة العالم (P7-T10): عُقد الموارد والبرابرة، الأهداف
  (العرش/التاج، الممرات/البوابات)، منشآت التحالف (برج/منجنيق)، وأيقونات
  مسيرات فروع الوحدات الأربعة (infantry/cavalry/archer/siege).
.DESCRIPTION
  الصور PNG في Content/Art/WorldMapIcons هي أصول أصلية (14 ملف). يستورد السكربت
  المجموعة دفعة واحدة إلى /Game/Art/WorldMapIcons عبر UnrealEditor -run=ImportAssets
  (TextureFactory)، مع تحقق من اكتمال الحزمة قبل الاستيراد.
.EXAMPLE
  .\scripts\Import-WorldMapIcons.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Import-WorldMapIcons.ps1 -EngineRoot $env:UE_ROOT -ReplaceExisting
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

function New-ImportGroup {
    param([string]$Name, [string]$Destination, [string]$Factory, [string[]]$Files)
    $Group = @{
        GroupName = $Name
        DestinationPath = $Destination
        FileNames = $Files
        bReplaceExisting = [bool]$ReplaceExisting
        bSkipReadOnly = $true
    }
    if (-not [string]::IsNullOrWhiteSpace($Factory)) { $Group.FactoryName = $Factory }
    return $Group
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Import-WorldMapIcons.ps1 مخصص للتشغيل المحلي على Windows مع Unreal Engine.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$Engine = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

$Icons = Get-ChildItem (Join-Path $ProjectRoot 'Content\Art\WorldMapIcons') -Filter '*.png' | Sort-Object Name | ForEach-Object { $_.FullName }
if ($Icons.Count -lt 14) { throw "حزمة أيقونات خريطة العالم غير مكتملة: مطلوب 14 PNG، وُجد $($Icons.Count)." }

$Groups = @(
    (New-ImportGroup -Name 'WorldMapIcons' -Destination '/Game/Art/WorldMapIcons' -Factory 'TextureFactory' -Files $Icons)
)

$ConfigDirectory = Join-Path $ProjectRoot 'Saved\ImportConfigs'
New-Item -ItemType Directory -Path $ConfigDirectory -Force | Out-Null
$ConfigPath = Join-Path $ConfigDirectory 'world-map-icons-import.json'
@{ ImportGroups = $Groups } | ConvertTo-Json -Depth 8 | Set-Content -Path $ConfigPath -Encoding UTF8

$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$LogPath = Join-Path $LogDirectory ("import-world-map-icons-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host "[ROK2] Importing $($Icons.Count) world map icons (P7-T10) with UE 5.4.4: $Engine" -ForegroundColor Cyan
& $EditorCmd $ProjectFile '-run=ImportAssets' "-importSettings=$ConfigPath" '-nosourcecontrol' '-unattended' '-nop4' 2>&1 | Tee-Object -FilePath $LogPath
