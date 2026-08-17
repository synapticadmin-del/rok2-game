<#
.SYNOPSIS
  يستورد رسوم معالم خريطة العالم المقطّعة (P24-T6) إلى /Game/Art/WorldFeatures.

.DESCRIPTION
  الأصول يولّدها `scripts/slice_world_feature_sprites.py` بتقطيع صفائح
  `Content/Art/WorldMapIcons/T_world_*.png` بمركّبات الشفافية المتصلة — فتلك
  صفائح لا sprites: واحدة تحمل أربع عقد موارد ونصاً عربياً مطبوعاً داخل الصورة،
  وأخرى تحمل منشأتين. الناتج 11 معلماً × 3 خرائط (D/N/E) = 33 ملفاً في
  Content/Art/WorldFeatures.

  السكربت يشغّل التقطيع أولاً إن كان Python متاحاً (فلا يُستورد أصلٌ قديم بصمت
  بعد تعديل السكربت)، ثم يستورد الحزمة عبر UnrealEditor -run=ImportAssets.

.EXAMPLE
  .\scripts\Import-WorldFeatureSprites.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Import-WorldFeatureSprites.ps1 -EngineRoot $env:UE_ROOT -ReplaceExisting -SkipSlice
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT,
    [switch]$ReplaceExisting,
    [switch]$SkipSlice
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
        throw "يتطلب استيراد أصول ROK2 Unreal Engine 5.4.4، لكن المحرك المحدد هو ${ActualVersion} في المسار: ${Candidate}"
    }
    return (Resolve-Path $Candidate).Path
}

if ($env:OS -ne 'Windows_NT') {
    throw 'Import-WorldFeatureSprites.ps1 مخصص للتشغيل المحلي على Windows مع Unreal Engine.'
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepoRoot = (Resolve-Path (Join-Path $ProjectRoot '..\..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'
$Engine = Resolve-UnrealEngineRoot -RequestedRoot $EngineRoot
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$FeatureDir = Join-Path $ProjectRoot 'Content\Art\WorldFeatures'

# ── التقطيع قبل الاستيراد ────────────────────────────────────────────────────
# بلا هذا كان تعديل معايير التقطيع في السكربت يبقى بلا أثر حتى يتذكّر أحدهم
# تشغيله يدوياً — فيُستورد أصلٌ قديم ويُظنّ أنه الجديد.
if (-not $SkipSlice) {
    $SliceScript = Join-Path $RepoRoot 'scripts\slice_world_feature_sprites.py'
    if (-not (Test-Path $SliceScript)) {
        throw "سكربت التقطيع غير موجود: $SliceScript"
    }
    $Python = Get-Command python -ErrorAction SilentlyContinue
    if ($null -eq $Python) {
        Write-Warning '[ROK2] Python غير متاح — يُتخطّى التقطيع ويُستورد ما هو على القرص.'
    } else {
        Write-Host '[ROK2] Slicing world feature sprites from sheets...' -ForegroundColor Cyan
        & $Python.Source $SliceScript
        if ($LASTEXITCODE -ne 0) {
            throw "فشل تقطيع الصفائح (exit $LASTEXITCODE) — لا استيراد لأصول غير مكتملة."
        }
    }
}

if (-not (Test-Path $FeatureDir)) {
    throw "مجلد الرسوم المقطّعة غير موجود: $FeatureDir — شغّل scripts/slice_world_feature_sprites.py"
}

$Sprites = Get-ChildItem $FeatureDir -Filter 'T_feat_*.png' | Sort-Object Name | ForEach-Object { $_.FullName }
if ($Sprites.Count -ne 33) {
    throw "حزمة معالم العالم غير مكتملة: مطلوب 33 PNG (11 معلماً × D/N/E)، وُجد $($Sprites.Count)."
}

$Group = @{
    GroupName = 'WorldFeatureSprites'
    DestinationPath = '/Game/Art/WorldFeatures'
    FileNames = $Sprites
    FactoryName = 'TextureFactory'
    bReplaceExisting = [bool]$ReplaceExisting
    bSkipReadOnly = $true
}

$ConfigDirectory = Join-Path $ProjectRoot 'Saved\ImportConfigs'
New-Item -ItemType Directory -Path $ConfigDirectory -Force | Out-Null
$ConfigPath = Join-Path $ConfigDirectory 'world-feature-sprites-import.json'
@{ ImportGroups = @($Group) } | ConvertTo-Json -Depth 8 | Set-Content -Path $ConfigPath -Encoding UTF8

$LogDirectory = Join-Path $ProjectRoot 'Saved\BuildLogs'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
$LogPath = Join-Path $LogDirectory ("import-world-features-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))

Write-Host "[ROK2] Importing $($Sprites.Count) world feature sprites (P24-T6) with UE 5.4.4: $Engine" -ForegroundColor Cyan
& $EditorCmd $ProjectFile '-run=ImportAssets' "-importSettings=$ConfigPath" '-nosourcecontrol' '-unattended' '-nop4' 2>&1 | Tee-Object -FilePath $LogPath

# التحقق بالناتج لا بشفرة الخروج.
#
# `-run=ImportAssets` يعيد 1 ويسجّل `Invalid Destination Path ()` حتى عندما
# يستورد كل الملفات بنجاح — يقرأ الـcommandlet مجموعة فارغة إضافية من إعداداته
# الافتراضية إلى جانب `ImportGroups`. لذلك لا يفحص أيٌّ من سكربتات الاستيراد
# القائمة في هذا المشروع شفرة الخروج.
#
# لكن تجاهلها بلا بديل يعني أن فشلاً حقيقياً يمرّ صامتاً. فالبوابة هنا هي عدد
# أصول `.uasset` الناتجة: 33 أصلاً موجودة = استيراد ناجح، وأي نقص يرفع خطأ.
$Assets = @(Get-ChildItem $FeatureDir -Filter 'T_feat_*.uasset' -ErrorAction SilentlyContinue)
if ($Assets.Count -ne $Sprites.Count) {
    throw "فشل الاستيراد: مطلوب $($Sprites.Count) أصلاً، وُجد $($Assets.Count). السجل: $LogPath"
}

Write-Host "[ROK2] Imported $($Assets.Count) assets. Log: $LogPath" -ForegroundColor Green
