<#
.SYNOPSIS
  يستورد حزمة أصول الحانة والصناديق والمفاتيح والمنحوتات (P10-T7) من
  Content/Art/Tavern إلى /Game/Art/Tavern في مشروع Unreal Engine 5.
.DESCRIPTION
  يستورد الموديلات 3D (4 GLB: building_tavern, chest_silver, chest_gold, chest_equipment)
  والأيقونات 2D (23 PNG: صناديق، مفاتيح، منحوتات، مواد ومخططات) والمؤثرات الصوتية.
.EXAMPLE
  .\scripts\Import-TavernAssets.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ReplaceExisting
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$SourceDir = Join-Path $PSScriptRoot '..\Content\Art\Tavern'
$PackageDir = '/Game/Art/Tavern'

function Resolve-UnrealEngineRoot {
    param([string]$RequestedRoot)
    $Candidate = if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $RequestedRoot
    } else {
        'C:\Program Files\Epic Games\UE_5.4'
    }
    $EditorCmd = Join-Path $Candidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $VersionFile = Join-Path $Candidate 'Engine\Build\Build.version'
    if (-not (Test-Path $EditorCmd)) {
        throw "UnrealEditor-Cmd.exe not found at '$EditorCmd'. مرر -EngineRoot صحيح أو عيّن UE_ROOT."
    }
    if (-not (Test-Path $VersionFile)) {
        throw "Build.version not found at '$VersionFile'."
    }
    $Version = Get-Content $VersionFile -Raw | ConvertFrom-Json
    $Detected = '{0}.{1}' -f $Version.MajorVersion, $Version.MinorVersion
    Write-Host "Engine detected: $Detected ($Candidate)"
    return $EditorCmd
}

function Get-TavernGlbFiles {
    Get-ChildItem $SourceDir -Filter *.glb | Sort-Object Name | Select-Object -ExpandProperty FullName
}

function Get-TavernPngFiles {
    Get-ChildItem $SourceDir -Filter *.png | Sort-Object Name | Select-Object -ExpandProperty FullName
}

$GlbFiles = Get-TavernGlbFiles
$PngFiles = Get-TavernPngFiles

if ($GlbFiles.Count -lt 4) {
    throw "ناقص: مطلوب 4 ملفات GLB في $SourceDir لكن وُجد $($GlbFiles.Count)."
}
if ($PngFiles.Count -lt 23) {
    throw "ناقص: مطلوب 23 ملف PNG في $SourceDir لكن وُجد $($PngFiles.Count)."
}

Write-Host "P10-T7: تم العثور على $($GlbFiles.Count) موديلات GLB و $($PngFiles.Count) أيقونات PNG جاهزة للاستيراد."
