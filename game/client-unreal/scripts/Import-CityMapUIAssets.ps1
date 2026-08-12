<#
.SYNOPSIS
  يستورد رموز المدينة والخريطة وأزرار الواجهة، ويستورد نماذج Kenney Castle Kit اختيارياً.

.DESCRIPTION
  الصور PNG في Content/Art/UIIcons وContent/Art/UIButtons وContent/Art/CityBuildingIcons هي أصول أصلية قابلة
  للاستيراد تلقائياً. نماذج GLB في Content/Art/KenneyCastleKit من Kenney Castle
  Kit (CC0)؛ استخدم -ImportMeshes عند توفر مستورد glTF/Interchange في نسخة Unreal
  المحلية. إن لم يكن المستورد متاحاً، يبقى المسار آمناً: تستورد الصور وتفتح النماذج
  من Content Browser يدوياً بعد تفعيل GLTF Importer أو Interchange.

.EXAMPLE
  .\scripts\Import-CityMapUIAssets.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
  .\scripts\Import-CityMapUIAssets.ps1 -EngineRoot $env:UE_ROOT -ImportMeshes -ReplaceExisting
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ImportMeshes,
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
    throw 'Import-CityMapUIAssets.ps1 مخصص للتشغيل المحلي على Windows مع Unreal Engine.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$Engine = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Icons = Get-ChildItem (Join-Path $ProjectRoot 'Content\Art\UIIcons') -Filter '*.png' | Sort-Object Name | ForEach-Object { $_.FullName }
$Buttons = Get-ChildItem (Join-Path $ProjectRoot 'Content\Art\UIButtons') -Filter '*.png' | Sort-Object Name | ForEach-Object { $_.FullName }
$CityBuildings = Get-ChildItem (Join-Path $ProjectRoot 'Content\Art\CityBuildingIcons') -Filter '*.png' | Sort-Object Name | ForEach-Object { $_.FullName }
if ($Icons.Count -lt 16) { throw "حزمة الرموز غير مكتملة: مطلوب 16 PNG، وُجد $($Icons.Count)." }
if ($Buttons.Count -lt 4) { throw "حزمة الأزرار غير مكتملة: مطلوب 4 PNG، وُجد $($Buttons.Count)." }
if ($CityBuildings.Count -lt 10) { throw "حزمة صور مباني المدينة غير مكتملة: مطلوب 10 PNG، وُجد $($CityBuildings.Count)." }

$Groups = @(
    (New-ImportGroup -Name 'CityMapUiIcons' -Destination '/Game/Art/UIIcons' -Factory 'TextureFactory' -Files $Icons),
    (New-ImportGroup -Name 'CityMapUiButtons' -Destination '/Game/Art/UIButtons' -Factory 'TextureFactory' -Files $Buttons),
    (New-ImportGroup -Name 'CityBuildingPortraits' -Destination '/Game/Art/CityBuildingIcons' -Factory 'TextureFactory' -Files $CityBuildings)
)

if ($ImportMeshes) {
    $Meshes = Get-ChildItem (Join-Path $ProjectRoot 'Content\Art\KenneyCastleKit') -Filter '*.glb' | Sort-Object Name | ForEach-Object { $_.FullName }
    if ($Meshes.Count -lt 1) { throw 'لم توجد نماذج GLB في Content\Art\KenneyCastleKit.' }
    # GLTFImportFactory متاح عند تفعيل GLTF Importer/Interchange في محرر UE المحلي.
    $Groups += (New-ImportGroup -Name 'KenneyCastleKit' -Destination '/Game/Art/KenneyCastleKit' -Factory 'GLTFImportFactory' -Files $Meshes)
}

$ConfigDirectory = Join-Path $ProjectRoot 'Saved\ImportConfigs'
New-Item -ItemType Directory -Path $ConfigDirectory -Force | Out-Null
$ConfigPath = Join-Path $ConfigDirectory 'city-map-ui-assets-import.json'
@{ ImportGroups = $Groups } | ConvertTo-Json -Depth 8 | Set-Content -Path $ConfigPath -Encoding UTF8

$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$LogPath = Join-Path $LogDirectory ("import-city-map-ui-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host "[ROK2] Importing $($Icons.Count) icons, $($Buttons.Count) button skins, and $($CityBuildings.Count) city building portraits with $Engine" -ForegroundColor Cyan
if ($ImportMeshes) {
    Write-Host '[ROK2] Importing GLB meshes through GLTFImportFactory.' -ForegroundColor Cyan
} else {
    Write-Host '[ROK2] GLB import skipped. Use -ImportMeshes after enabling GLTF Importer/Interchange.' -ForegroundColor Yellow
}
& $EditorCmd $ProjectFile '-run=ImportAssets' "-importSettings=$ConfigPath" '-nosourcecontrol' '-unattended' '-nop4' 2>&1 | Tee-Object -FilePath $LogPath
if ($LASTEXITCODE -ne 0) {
    throw "فشل استيراد أصول المدينة والخريطة (رمز: $LASTEXITCODE). راجع: $LogPath"
}
Write-Host "[ROK2] City/map UI assets imported. Log: $LogPath" -ForegroundColor Green
