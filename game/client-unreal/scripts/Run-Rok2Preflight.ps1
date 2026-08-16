<#
.SYNOPSIS
  فحص جاهزية شامل قبل بناء ROK2 أو تشغيله على Windows — تقرير واحد بلا بناء.

.DESCRIPTION
  يتحقق دفعة واحدة من: محرك UE 5.4.4 (Build.version)، ملفات المشروع الأساسية
  (Rok2.uproject، خريطة Rok2Main، وحدة Rok2)، وجود أدوات بناء Windows
  (Build.bat/RunUAT.bat/UnrealEditor-Cmd.exe)، Visual Studio MSVC 14.38+ وWindows SDK
  10.0.22621+ عبر vswhere، حالة السجلات السابقة (آخر build-Editor)، ثم يطبع جدول
  جاهزية لكل بند بصيغة PASS/FAIL/WARN مع رمز خروج 0 عند PASS كامل و1 عند أي FAIL.
  لا يبني ولا يشغّل أي شيء — آمن للتشغيل المتكرر.

.EXAMPLE
  .\scripts\Run-Rok2Preflight.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Run-Rok2Preflight.ps1  # يعتمد على UE_ROOT
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = $env:UE_ROOT
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Continue'

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ProjectFile = Join-Path $ProjectRoot 'Rok2.uproject'

$Candidate = if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot
} else {
    'C:\Program Files\Epic Games\UE_5.4'
}
$ResolvedRoot = if (Test-Path $Candidate) { (Resolve-Path $Candidate).Path } else { $Candidate }

$Results = [System.Collections.Generic.List[object]]::new()

function Add-Check {
    param([string]$Name, [string]$Expected, [string]$Actual, [string]$Status, [string]$Note = '')
    $Results.Add([PSCustomObject]@{ Name = $Name; Expected = $Expected; Actual = $Actual; Status = $Status; Note = $Note })
}

# 1 — إصدار المحرك.
$VersionFile = Join-Path $ResolvedRoot 'Engine\Build\Build.version'
if (-not (Test-Path $VersionFile)) {
    Add-Check 'Engine 5.4.4' 'Engine\Build\Build.version = 5.4.4' 'غير موجود' 'FAIL' 'مرر -EngineRoot أو عيّن UE_ROOT لمسار UE 5.4.4'
} else {
    $Version = Get-Content -Path $VersionFile -Raw | ConvertFrom-Json
    $ActualVersion = "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
    if ($ActualVersion -eq '5.4.4') {
        Add-Check 'Engine 5.4.4' 'Engine\Build\Build.version = 5.4.4' $ActualVersion 'PASS'
    } else {
        Add-Check 'Engine 5.4.4' 'Engine\Build\Build.version = 5.4.4' $ActualVersion 'FAIL' 'السكربتات ترفض البناء على نسخ أخرى'
    }
}

# 2 — أدوات البناء داخل المحرك.
foreach ($Tool in @('Engine\Build\BatchFiles\Build.bat', 'Engine\Build\BatchFiles\RunUAT.bat', 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe')) {
    $ToolPath = Join-Path $ResolvedRoot $Tool
    $Status = if (Test-Path $ToolPath) { 'PASS' } else { 'FAIL' }
    Add-Check (Split-Path $Tool -Leaf) 'موجود داخل UE_ROOT' ($ToolPath) $Status
}

# 3 — ملفات المشروع الأساسية.
$ProjectStatus = if (Test-Path $ProjectFile) { 'PASS' } else { 'FAIL' }
Add-Check 'Rok2.uproject' 'موجود في جذر المشروع' $ProjectFile $ProjectStatus
$MapPath = Join-Path $ProjectRoot 'Content\Maps\Rok2Main.umap'
$MapStatus = if (Test-Path $MapPath) { 'PASS' } else { 'FAIL' }
Add-Check 'Rok2Main.umap' 'خريطة البدء في Content\Maps' $MapPath $MapStatus

$Project = Get-Content -Path $ProjectFile -Raw | ConvertFrom-Json
$HasModule = $Project.Modules -and ($Project.Modules | Where-Object { $_.Name -eq 'Rok2' })
$ModuleStatus = if ($HasModule) { 'PASS' } else { 'FAIL' }
Add-Check 'وحدة Rok2' 'Module باسم Rok2 في uproject' ($HasModule.ToString()) $ModuleStatus

# 4 — إعدادات العرض وخريطة البدء.
$EngineIni = Join-Path $ProjectRoot 'Config\DefaultEngine.ini'
$DefaultMap = Select-String -Path $EngineIni -Pattern 'GameDefaultMap=/Game/Maps/Rok2Main' -SimpleMatch | Select-Object -First 1
$DefaultMapStatus = if ($DefaultMap) { 'نعم' } else { 'لا' }
$MapConfigStatus = if ($DefaultMap) { 'PASS' } else { 'FAIL' }
Add-Check 'GameDefaultMap' 'Rok2Main في DefaultEngine.ini' $DefaultMapStatus $MapConfigStatus
$SplashDisabled = (Get-Content -Path $EngineIni -Raw) -match 'bSplashScreen=False'
$SplashStatus = if ($SplashDisabled) { 'PASS' } else { 'FAIL' }
Add-Check 'Splash معطل (P7-T12)' 'bSplashScreen=False' ($SplashDisabled.ToString()) $SplashStatus

# 5 — سلسلة أدوات Visual Studio (vswhere).
$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VsWhere)) {
    Add-Check 'Visual Studio 2022 17.8+' 'vswhere + MSVC 14.38+ وWindows SDK 10.0.22621+' 'vswhere غير موجود' 'WARN' 'ثبّت VS 2022 workload: Game development with C++'
} else {
    $VcVars = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if ([string]::IsNullOrEmpty($VcVars)) {
        Add-Check 'Visual Studio 2022 17.8+' 'MSBuild/MSVC 14.38+' 'غير قابل للاكتشاف' 'FAIL' 'شغّل مثبت VS وأضف Game development with C++'
    } else {
        Add-Check 'Visual Studio 2022 17.8+' 'MSBuild/MSVC 14.38+ قابل للاكتشاف' (Split-Path $VcVars -Parent) 'PASS'
    }
    $Sdk = & $VsWhere -latest -products * -find 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt' | Select-Object -First 1
    if ([string]::IsNullOrEmpty($Sdk)) {
        Add-Check 'Windows SDK' '10.0.22621.0+' 'غير قابل للاكتشاف' 'WARN' 'ثبّت Windows 11 SDK (10.0.22621+)'
    } else {
        Add-Check 'Windows SDK' '10.0.22621.0+' 'VS SDK متوفر' 'PASS'
    }
}

# 6 — JDK 17 لـ Android (P7-T11/BF-001).
$Jdk17 = 'C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
if (Test-Path (Join-Path $Jdk17 'bin\java.exe')) {
    Add-Check 'JDK 17 (Android P7-T11)' 'bin\java.exe موجود' $Jdk17 'PASS' 'Build-Rok2.ps1 يوجّه JAVA_HOME إليه قبل بناء Android'
} else {
    Add-Check 'JDK 17 (Android P7-T11)' 'bin\java.exe موجود' 'غير موجود' 'WARN' 'بناء Android سيفشل (BF-001) ما لم يُضبط JAVA_HOME يدويًا'
}

# 7 — سجل آخر بناء (تقرير لا شرط).
$LogDir = Join-Path $ProjectRoot 'Saved\BuildLogs'
$LastBuild = if (Test-Path $LogDir) {
    Get-ChildItem -Path $LogDir -Filter 'build-Editor-*.log' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
} else { $null }
$LastBuildNote = if ($LastBuild) { "آخر بناء: $($LastBuild.Name) ($($LastBuild.LastWriteTime))" } else { 'لا سجلات بناء بعد' }
Add-Check 'سجل آخر بناء المحرر' 'build-Editor-*.log في Saved\BuildLogs' $LastBuildNote 'INFO'

# التقرير.
$Pad = ($Results.Name | Measure-Object -Property Length -Maximum).Maximum
$Results | Format-Table -AutoSize -Wrap -Property Name, Expected, Status, Note | Out-String | Write-Host

$Failed = @($Results | Where-Object { $_.Status -eq 'FAIL' }).Count
if ($Failed -gt 0) {
    Write-Host "[ROK2] Preflight: $Failed failure(s). أصلح FAIL قبل Build-Rok2.ps1." -ForegroundColor Red
    exit 1
}
Write-Host '[ROK2] Preflight: كل البنود PASS/WARN. يمكنك تشغيل Build-Rok2.ps1 -Target Editor.' -ForegroundColor Green
exit 0
