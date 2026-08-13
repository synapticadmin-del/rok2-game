<#
.SYNOPSIS
  يستورد حزمة الوحدات البشرية 3D (P8-T8): 17 موديلًا low-poly
  (infantry/archer/cavalry × T1–T5 + siege_arcuballista + siege_mangonel)
  من Content/Art/HumanUnits إلى /Game/Art/HumanUnits في المشروع.
.DESCRIPTION
  يستورد الموديلات دفعة واحدة عبر UnrealEditor -run=ImportAssets (StaticMeshFactory)،
  مع تحقق من اكتمال الحزمة قبل الاستيراد. موديلات الحصار T3–T5 (Ballista/Trebuchet/
  Catapult) من Kenney Castle Kit موجودة أصلًا ولا يعاد استيرادها.
.EXAMPLE
  .\scripts\Import-HumanUnits.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Import-HumanUnits.ps1 -EngineRoot $env:UE_ROOT -ReplaceExisting
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ReplaceExisting
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$SourceDir = Join-Path $PSScriptRoot '..\Content\Art\HumanUnits'
$PackageDir = '/Game/Art/HumanUnits'

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

function Get-HumanUnitFiles {
    Get-ChildItem $SourceDir -Filter *.glb | Sort-Object Name | Select-Object -ExpandProperty FullName
}

function Test-PackageExists {
    $ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
    $UProject = (Get-ChildItem $ProjectRoot -Filter *.uproject | Select-Object -First 1).FullName
    # UE يحفظ الأصول في Content/Art/HumanUnits/<file>/<file>.uasset بعد الاستيراد
    $Disk = Join-Path $ProjectRoot ('Content' + ($PackageDir -replace '/', '\'))
    $Existing = Get-ChildItem $Disk -Recurse -Filter *.uasset -ErrorAction SilentlyContinue
    return ($Existing.Count -ge 17)
}

$Files = Get-HumanUnitFiles
if ($Files.Count -ne 17) {
    throw "حزمة HumanUnits غير مكتملة: {0}/17 ملفًا في '$SourceDir'. نفذ scripts\generate_human_units_glb.py أولًا." -f $Files.Count
}

$EditorCmd = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
if (Test-PackageExists -and -not $ReplaceExisting) {
    Write-Host "حزمة /Game/Art/HumanUnits موجودة بالفعل (17 أصلًا+). استخدم -ReplaceExisting لإعادة الاستيراد."
    exit 0
}

$TempList = New-TemporaryFile
$Files | Set-Content $TempList -Encoding ASCII

Write-Host "استيراد {0} موديلًا بشريًا إلى {1}..." -f $Files.Count, $PackageDir
$Args = @(
    (Resolve-Path (Join-Path $PSScriptRoot '..\*.uproject') | Select-Object -First 1).Path
    '-run=ImportAssets'
    "-importfilename={0}" -f $TempList.FullName
    "-packagepath={0}" -f $PackageDir
    '-importsettings=StaticMeshFactory'
    '-nohmd'
)
$Proc = Start-Process -FilePath $EditorCmd -ArgumentList $Args -Wait -PassThru -NoNewWindow
$ExitCode = $Proc.ExitCode
Remove-Item $TempList -Force

if ($ExitCode -ne 0) {
    throw "فشل الاستيراد (exit {0}). راجع مخرجات UnrealEditor أعلاه." -f $ExitCode
}
Write-Host "تم استيراد {0} موديلًا بشريًا إلى {1}." -f $Files.Count, $PackageDir
