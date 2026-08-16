<#
.SYNOPSIS
  يستورد الأصول البصرية الجديدة (P16) كـ Texture2D داخل مشروع Unreal.
.DESCRIPTION
  يستورد من Content/Art:
    - Splash/           خلفيات الشاشة الافتتاحية والتحميل والعاصفة القادمة (3)
    - SeasonStory/      خلفيات فصول قصة الموسم الأربعة
    - CommanderSkins/   جلود القادة النادرة للحضارات الست
    - Events/           فن أحداث P10 (الحانة، الإكسبيديشن، الكانيون، أوزيريس)
  يمكن تكراره بأمان بعد تحديث أي PNG (bReplaceExisting عبر -ReplaceExisting).
.EXAMPLE
  .\scripts\Import-NewVisuals.ps1 -EngineRoot '/opt/UnrealEngine'
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
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        return (Resolve-Path $RequestedRoot).Path
    }
    if ($env:OS -eq 'Windows_NT') {
        return 'C:\Program Files\Epic Games\UE_5.4'
    }
    return '/opt/UnrealEngine'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$Engine = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot

if ($env:OS -eq 'Windows_NT') {
    $EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $VersionFile = Join-Path $Engine 'Engine\Build\Build.version'
    if (-not (Test-Path $VersionFile)) {
        throw 'لم يُعثر على Engine\Build\Build.version في: {0}' -f $Engine
    }
    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = '{0}.{1}.{2}' -f $Version.MajorVersion, $Version.MinorVersion, $Version.PatchVersion
    if ($ActualVersion -ne '5.4.4') {
        throw ('يتطلب استيراد أصول ROK2 Unreal Engine 5.4.4، لكن المحدد هو {0}: {1}' -f $ActualVersion, $Engine)
    }
}
else {
    $EditorCmd = Join-Path $Engine 'Engine/Binaries/Linux/UnrealEditor-Cmd'
    if (-not (Test-Path $EditorCmd)) {
        throw ('لا يوجد UnrealEditor-Cmd في {0} — نفذ make أولاً' -f $Engine)
    }
}

if (-not (Test-Path $EditorCmd)) {
    throw 'UnrealEditor-Cmd غير موجود في: {0}' -f $EditorCmd
}
if (-not (Test-Path $ProjectFile)) {
    throw 'Rok2.uproject غير موجود في: {0}' -f $ProjectFile
}

$Groups = @(
    @{ Name = 'P16-T1 Splash';    Dest = '/Game/Art/Splash';         Src = (Join-Path $ProjectRoot 'Content\Art\Splash') },
    @{ Name = 'P16-T2 Season';    Dest = '/Game/Art/SeasonStory';    Src = (Join-Path $ProjectRoot 'Content\Art\SeasonStory') },
    @{ Name = 'P16-T3 Skins';     Dest = '/Game/Art/CommanderSkins'; Src = (Join-Path $ProjectRoot 'Content\Art\CommanderSkins') },
    @{ Name = 'P16-T4 Events';    Dest = '/Game/Art/Events';         Src = (Join-Path $ProjectRoot 'Content\Art\Events') }
)

if ($env:OS -ne 'Windows_NT') {
    $Groups = $Groups | ForEach-Object {
        $_.Src = $_.Src.Replace('\', '/')
        return $_
    }
}

$SettingsFile = Join-Path $ProjectRoot ('Content\{0}Import-NewVisuals.json' -f [System.IO.Path]::DirectorySeparatorChar)
$ImportData = @()
foreach ($Group in $Groups) {
    if (-not (Test-Path $Group.Src)) {
        Write-Warning ('مجلد غير موجود، تخطٍ: {0}' -f $Group.Src)
        continue
    }
    $Files = (Get-ChildItem -Path $Group.Src -Filter *.png -File | Sort-Object Name | Select-Object -ExpandProperty FullName)
    if (-not $Files) {
        Write-Warning ('لا ملفات PNG في: {0}' -f $Group.Src)
        continue
    }
    $ImportData += @{
        GroupName        = $Group.Name
        DestinationPath  = $Group.Dest
        FactoryName      = 'TextureFactory'
        FileNames        = $Files
        bReplaceExisting = [bool]$ReplaceExisting
        bSkipReadOnly    = $true
    }
    Write-Host ('[+] {0}: {1} ملف -> {1}' -f $Group.Name, ($Files.Count))
}

if (-not $ImportData) {
    throw 'لا توجد مجموعات استيراد — تحقق من مجلدات Content/Art'
}

$SettingsFile
$ImportData | ConvertTo-Json -Depth 4 | Set-Content -Path $SettingsFile -Encoding UTF8

$ArgsLine = (
    '"{0}" -run=ImportAssets -Settings="{1}" -unattended' -f $ProjectFile, $SettingsFile
)
Write-Host ('[*] استيراد {0} مجموعة عبر ImportAssetsCommandlet...' -f $ImportData.Count)

$Process = Start-Process -FilePath $EditorCmd -ArgumentList $ArgsLine `
    -NoNewWindow -Wait -PassThru

if ($Process.ExitCode -ne 0) {
    throw ('ImportAssetsCommandlet فشل بـ exit code {0}' -f $Process.ExitCode)
}

Remove-Item -Path $SettingsFile -Force -ErrorAction SilentlyContinue
Write-Host '[+] اكتمل استيراد الأصول البصرية الجديدة (P16)'
