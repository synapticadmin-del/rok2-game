<#
.SYNOPSIS
  يبني عميل ROK2 من سطر الأوامر باستخدام Unreal Engine 5.4.4 المثبت محلياً.

.DESCRIPTION
  يدعم بناء Rok2Editor للتحقق قبل PIE، وبناء Rok2 Development/Shipping لنظامي
  Windows وAndroid، والتحزيم الاختياري عبر Unreal Automation Tool. لا يغير ملفات المشروع
  ولا يمسح ملفات وسيطة إلا عند تمرير -Clean صراحةً.

.EXAMPLE
  .\scripts\Build-Rok2.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.4'
  .\scripts\Build-Rok2.ps1 -Target Development -Platform Win64 -Package -OutputDirectory 'D:\Builds\ROK2'
  .\scripts\Build-Rok2.ps1 -Target Development -Platform Android -Package -OutputDirectory 'D:\Builds\ROK2-Android'
#>
[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Development', 'Shipping')]
    [string]$Target = 'Editor',

    [ValidateSet('Win64', 'Android')]
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
        throw "يتطلب ROK2 Unreal Engine 5.4.4، لكن المحرك المحدد هو $ActualVersion في المسار: ${Candidate}"
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

# P7-T12 fix: Gradle 7.6.3 bundled with UE 5.4 cannot parse class file
# major version 65 (Java 21). The Android Studio JBR ships Java 21, so
# we redirect JAVA_HOME to Microsoft JDK 17 before invoking Build.bat.
# UEDeployAndroid inherits JAVA_HOME from the process environment.
if ($Platform -eq 'Android') {
    $Jdk17 = 'C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
    if (Test-Path (Join-Path $Jdk17 'bin\java.exe')) {
        $env:JAVA_HOME = $Jdk17
        $env:PATH = "$Jdk17\bin;$env:PATH"
        Write-Host "[ROK2] JAVA_HOME redirected to JDK 17 for Gradle compatibility" -ForegroundColor DarkGray
    }
}

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
    if ($Platform -eq 'Android') {
        throw 'لا يدعم Android هدف Editor. استخدم -Target Development أو -Target Shipping.'
    }
    $BuildArguments = @('Rok2Editor', $Platform, 'Development', $ProjectFile, '-WaitMutex', '-NoHotReload')
} else {
    $BuildArguments = @('Rok2', $Platform, $Target, $ProjectFile, '-WaitMutex', '-NoHotReload')
}

# P7-T12 fix:Copy GoogleVR PermissionHelper stub into Intermediate/Android JavaLibs before any Android build.
if ($Platform -eq 'Android') {
    $StubSrc = Join-Path $ProjectRoot 'Build\Android\JavaLibs\vrpermissionstub'
    $JavaLibsBase = Join-Path $ProjectRoot 'Intermediate\Android\arm64\JavaLibs'
    if ((Test-Path $StubSrc) -and (Test-Path $JavaLibsBase)) {
        $StubDest = Join-Path $JavaLibsBase 'vrpermissionstub'
        if (Test-Path $StubDest) { Remove-Item -Recurse -Force $StubDest }
        Copy-Item -Recurse -Force $StubSrc $StubDest
        Write-Host "[ROK2] Copied vrpermissionstub to Intermediate JavaLibs" -ForegroundColor DarkGray
    }
}

Invoke-UnrealBatchFile -FilePath $BuildBat -Arguments $BuildArguments -LogPath $BuildLog

if ($Package) {
    if ($Target -eq 'Editor') {
        throw 'التحزيم يتطلب -Target Development أو -Target Shipping، وليس Editor.'
    }
    if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
        $OutputDirectory = Join-Path $ProjectRoot "Artifacts\$Platform-$Target"
    }
    $ResolvedOutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
    $PackageLog = Join-Path $LogDirectory "package-$Target-$Timestamp.log"
    # Keep each supported UAT platform explicit: Win64 remains the established
    # desktop contract while Android uses the same Development/Shipping flow.
    $UatPlatformArgument = if ($Platform -eq 'Win64') { '-platform=Win64' } else { '-platform=Android' }
    $UatArguments = @(
        'BuildCookRun',
        "-project=$ProjectFile",
        '-noP4',
        $UatPlatformArgument,
        "-clientconfig=$Target",
        '-cookflavor=ASTC',
        '-build', '-cook', '-cookall', '-stage', '-pak', '-iostore', '-archive',
        "-archivedirectory=$ResolvedOutputDirectory"
    )
    Invoke-UnrealBatchFile -FilePath $RunUatBat -Arguments $UatArguments -LogPath $PackageLog
    Write-Host "[ROK2] Package completed: $ResolvedOutputDirectory" -ForegroundColor Green
    Write-Host "[ROK2] Package log: $PackageLog" -ForegroundColor Green

    # P7-T12 build fix: UE 5.4's Android automation does not copy the cooked
    # pak/ucas/utoc into the gradle assets/ directory on its own. We do it
    # here so gradle packages them into the APK and the game can find assets
    # at runtime (otherwise PreInit fails with "Engine Preinit Failed").
    if ($Platform -eq 'Android') {
        $StagedPaks = Join-Path $ProjectRoot 'Saved\StagedBuilds\Android_ASTC\Rok2\Content\Paks'
        $GradleAssets = Join-Path $ProjectRoot 'Intermediate\Android\arm64\gradle\app\src\main\assets\Rok2\Content\Paks'
        if (Test-Path $StagedPaks) {
            New-Item -ItemType Directory -Force -Path $GradleAssets | Out-Null
            Get-ChildItem -Path $StagedPaks -Filter 'Rok2-Android_ASTC*' -ErrorAction SilentlyContinue | ForEach-Object {
                $TargetName = $_.Name -replace '_ASTC', ''
                $TargetPath = Join-Path $GradleAssets $TargetName
                Copy-Item -Path $_.FullName -Destination $TargetPath -Force
            }
            Get-ChildItem -Path $StagedPaks -Filter 'global*' -ErrorAction SilentlyContinue | ForEach-Object {
                $TargetPath = Join-Path $GradleAssets $_.Name
                Copy-Item -Path $_.FullName -Destination $TargetPath -Force
            }
            Write-Host "[ROK2] Copied staged paks to gradle assets" -ForegroundColor DarkGray

            # Re-run gradle assembleDebug so the APK picks up the new assets
            $GradleDir = Join-Path $ProjectRoot 'Intermediate\Android\arm64\gradle'
            $Env:JAVA_HOME = 'C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
            $Env:PATH = "$Env:JAVA_HOME\bin;$Env:PATH"
            Push-Location $GradleDir
            & cmd.exe /c "gradlew.bat assembleDebug --no-daemon -x lint -x lintAnalyzeDebug -x lintReportDebug" 2>&1 | Out-Host
            Pop-Location
            Write-Host "[ROK2] Gradle assembleDebug completed (exit=$LASTEXITCODE)" -ForegroundColor DarkGray

            # Copy final APK to output directory
            $BuiltApk = Join-Path $GradleDir 'app\build\outputs\apk\debug\app-debug.apk'
            $OutputApk = Join-Path $ResolvedOutputDirectory 'Rok2-arm64.apk'
            if (Test-Path $BuiltApk) {
                Copy-Item -Path $BuiltApk -Destination $OutputApk -Force
                Write-Host "[ROK2] Copied final APK to $OutputApk" -ForegroundColor DarkGray
            }
        }
    }
}

Write-Host "[ROK2] Build completed successfully." -ForegroundColor Green
Write-Host "[ROK2] Build log: $BuildLog" -ForegroundColor Green
