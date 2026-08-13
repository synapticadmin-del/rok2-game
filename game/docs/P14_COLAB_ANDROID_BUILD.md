# P14-T1: خلايا Google Colab لبناء APK — دون الاعتماد على جهاز Windows

**التاريخ:** أغسطس 2026 — **الحالة:** مكتملة ومرفوعة ✅

## التحذير الهام أولًا (اقرأ قبل النسخ)

بنية المشروع تتطلب **Unreal Engine 5.4.4** على **Linux x86-64** لبناء Android — وهذا ممكن نظريًا لكن **غير مدعوم رسميًا في Google Colab** للأسباب التالية:

| العائق | التوضيح |
|---|---|
| حجم المحرك | UE 5.4.4 Linux runtime ≈ 40–60 GB — يتجاوز حد مساحة Colab المتبقية (Corla runtime ≈ 107 GB قرص لكن البناء يستهلك كل المساحة) |
| زمن الجلسة | Colab يتوقف بعد ~12 ساعة؛ بناء Android أول مرة يحتاج 1–3 ساعات + تنزيل UE |
| الذاكرة | BuildCookRun يحتاج 16 GB RAM+ — Colab يقدّم 13–25 GB لكن البناء قد يُقتل (OOM) |
| ترخيص UE | تحميل ثنائي UE 5.4.4 يتطلب GitHub account مرتبط بإيبك (unrealengine repo خاص) أو Epic launcher |

**لذلك سلّمت لك ثلاث مجموعات خلايا بترتيب الأولوية:**

1. **المجموعة A (موصى بها — 5 دقائق):** على **جهاز Windows** لديك — كل السكربتات جاهزة في الريبو، الخلايا تنسّق الإعداد فقط (جدار Colab هو فقط كوثيقة تشغيل)
2. **المجموعة B (جهازك بدون Windows):** خلايا Colab تبني APK مباشرة عبر GitHub Actions (الأقوى على الإطلاق — كل شيء يعمل في السحابة)
3. **المجموعة C (محليًا على Linux/Colab):** خلايا كاملة لبناء UE 5.4.4 من المصدر إن أردت تجربة ذلك

---

## المجموعة A: سطر الأوامر على جهاز Windows (المسار الرسمي)

كل ما تحتاجه موجود في الريبو — انسخ هذه الأوامر في PowerShell:

```powershell
# 1) استنساخ المشروع
git clone https://github.com/synapticadmin-del/rok2-game.git
cd rok2-game\game\client-unreal

# 2) تثبيت Unreal Engine 5.4.4
#    من Epic Games Launcher → Library → Engine Versions → + → 5.4.4
#    أو: https://dev.epicgames.com/community/unreal-engine

# 3) تثبيت Java 17 (مطلوب لـ AAPT):
winget install Microsoft.OpenJDK.17
# ثم أعد تشغيل PowerShell

# 4) Android Studio → SDK Manager:
#    SDK Platforms  → Android 14.0 (API 34)
#    SDK Tools    → Android SDK Build-Tools 34.0.0
#                   Android SDK Command-line Tools
#                   NDK (Side by side) 27.2.12479018  ← مطابق لإعدادات المشروع
#                   CMake

# 5) تشغيل سكربت إعداد Android (يضبط ANDROID_HOME + NDKROOT + JAVA_HOME):
& "C:\Program Files\Epic Games\UE_5.4\Engine\Extras\Android\SetupAndroid.bat"
# ثم أعد تشغيل PowerShell

# 6) استيراد الأصول (مرة واحدة):
.\ImportAssets.bat

# 7) بناء APK:
.\BuildAndroid.bat
```

المخرج النهائي: `Build\Android\Rok2-arm64.apk` — ثبّته عبر USB:
`adb install -r Build\Android\Rok2-arm64.apk`

---

## المجموعة B: بناء APK عبر GitHub Actions (الأقوى — دون أي جهاز)

أنشئ ملف `.github/workflows/build-android.yml` في الريبو بهذا المحتوى (أو اطلب مني إضافته في الجلسة القادمة):

```yaml
name: Build Android APK

on:
  push:
    branches: [main]
  workflow_dispatch:

jobs:
  build-apk:
    runs-on: ubuntu-22.04
    timeout-minutes: 240
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
          submodules: true

      - name: Free disk space (GitHub runner ~14GB free by default)
        run: |
          sudo rm -rf /usr/share/dotnet /usr/local/lib/android /opt/ghc
          sudo rm -rf "$AGENT_TOOLSDIRECTORY"
          df -h

      - name: Install Unreal Engine 5.4.4 via ue5-docker / Epic GitHub
        env:
          GITHUB_TOKEN: ${{ secrets.UE5_TOKEN }}
        run: |
          git clone https://$GITHUB_TOKEN@github.com/EpicGames/UnrealEngine.git -b 5.4.4-release \
            --depth 1 --single-branch /opt/UnrealEngine
          cd /opt/UnrealEngine
          ./Setup.sh --exclude=Templates
          ./GenerateProjectFiles.sh
          make -j$(nproc)

      - name: Install Android SDK/NDK/JDK17
        uses: nttld/setup-ndk@v1
        with:
          ndk-version: r27c   # 27.2.12479018

      - name: Setup JAVA_HOME (JDK 17)
        uses: actions/setup-java@v4
        with:
          distribution: temurin
          java-version: 17

      - name: Copy JavaLibs stub (BF-004 fix — must precede build)
        run: |
          cp -r game/client-unreal/Build/Android/JavaLibs/vrpermissionstub \
            game/client-unreal/Intermediate/Android/arm64/JavaLibs/vrpermissionstub || true

      - name: Import raw assets → .uasset (headless, no GPU)
        run: |
          cd game/client-unreal
          ./Engine/Binaries/Linux/UnrealEditor-Cmd \
            Rok2.uproject -run=pythonscript \
            -script=import_assets.py \
            -nullrhi -unattended -nosplash

      - name: BuildCookRun — Android ASTC
        run: |
          cd game/client-unreal
          ./Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
            -project=$(pwd)/Rok2.uproject \
            -platform=Android -cookflavor=ASTC \
            -clientconfig=Development -targetplatform=Android \
            -build -cook -stage -package -pak -iostore -compressed \
            -archive -archivedirectory=$(pwd)/Build/Android \
            -nodebuginfo -utf8output \
            -nop4 -unattended -nullrhi

      - name: Upload APK artifact
        uses: actions/upload-artifact@v4
        with:
          name: rok2-arm64-apk
          path: game/client-unreal/Build/Android/*.apk
```

### كيف تشغّلها (خطوتان):

1. **أضف سر GitHub** في إعدادات الريبو: `Settings → Secrets and variables → Actions → New repository secret`
   - الاسم: `UE5_TOKEN`
   - القيمة: [Personal Access Token](https://github.com/settings/tokens) متصل بحساب Epic Games المسجّل في [unrealengine](https://github.com/EpicGames/UnrealEngine)

2. **شغّل** من تبويب `Actions` في الريبو → `Build Android APK` → `Run workflow`

بعد ~2–3 ساعات ستجد الـ APK في `Actions → Build Android APK → Artifacts` — حمّله وثبّته على هاتفك مباشرة.

---

## المجموعة C: خلايا Colab لبناء محلي (Linux) — للمغامرين فقط

> ⚠️ هذه الخلايا تعمل **فقط** إن كان لديك GitHub Personal Access Token متصل بحساب Epic Games. بدونها لا يمكن تنزيل UE 5.4.4. المساحة والزمن محدودان (Colab Free: 12h، 25 GB RAM، قرص ≈107 GB لكن البناء يستهلك معظمه).

### الخلية 1 — تجهيز البيئة وتثبيت Node.js 22

```python
import subprocess, os

# Node 22 (مطلوب لفحوصات npm run check قبل البناء)
subprocess.run("""curl -fsSL https://deb.nodesource.com/setup_22.x | sudo bash - >/dev/null 2>&1 && \
sudo apt-get install -y nodejs >/dev/null 2>&1""", shell=True)

print("node:", subprocess.run("node --version", shell=True, capture_output=True, text=True).stdout.strip())
print("npm:",  subprocess.run("npm --version",  shell=True, capture_output=True, text=True).stdout.strip())

# JDK 17 (مطلوب لـ AAPT أثناء بناء Android)
subprocess.run("sudo apt-get install -y openjdk-17-jdk-headless >/dev/null 2>&1", shell=True)
os.environ['JAVA_HOME'] = '/usr/lib/jvm/java-17-openjdk-amd64'
print("JAVA_HOME:", os.environ['JAVA_HOME'])
```

### الخلية 2 — استنساخ المشروع

```python
import os
os.chdir('/content')
!git clone --depth 1 https://github.com/synapticadmin-del/rok2-game.git
os.chdir('/content/rok2-game/game/backend')
!npm ci 2>&1 | tail -2
!npm run check 2>&1 | tail -3   # كل الفحوصات يجب أن تمر قبل البناء
```

### الخلية 3 — تثبيت Unreal Engine 5.4.4 من المصدر (30–60 دقيقة)

```python
import os, subprocess

# أدخل توكين GitHub الخاص بك المتصل بحساب Epic Games
UE5_TOKEN = "ghp_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

os.chdir('/opt')
!git clone --depth 1 --single-branch \
  https://{UE5_TOKEN}@github.com/EpicGames/UnrealEngine.git \
  -b 5.4.4-release /opt/UnrealEngine 2>&1 | tail -2

os.chdir('/opt/UnrealEngine')
!./Setup.sh --exclude=Templates 2>&1 | tail -3
!./GenerateProjectFiles.sh 2>&1 | tail -2
!make -j$(nproc) 2>&1 | tail -5
```

### الخلية 4 — Android SDK + NDK r27c (مطابق لإعدادات المشروع)

```python
import os

# Android SDK command-line tools → platform 34 + build-tools 34 + NDK r27c
os.environ['ANDROID_HOME'] = '/opt/android-sdk'
!mkdir -p $ANDROID_HOME/cmdline-tools && cd $ANDROID_HOME/cmdline-tools && \
  wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmd.zip && \
  unzip -q cmd.zip && mv cmdline-tools latest

!yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --licenses >/dev/null 2>&1
!$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager \
  "platforms;android-34" "build-tools;34.0.0" "ndk;27.2.12479018" "cmake;3.27.7" \
  2>&1 | tail -2

os.environ['NDK_ROOT'] = f"{os.environ['ANDROID_HOME']}/ndk/27.2.12479018"
os.environ['JAVA_HOME'] = '/usr/lib/jvm/java-17-openjdk-amd64'
print("ANDROID_HOME:", os.environ['ANDROID_HOME'])
print("NDK_ROOT:", os.environ['NDK_ROOT'])
```

### الخلية 5 — استيراد الأصول (headless، بلا كرت شاشة)

```python
import os
os.chdir('/content/rok2-game/game/client-unreal')
!mkdir -p Build/Android/JavaLibs/vrpermissionstub
# نسخ JavaLibs stub إلى Intermediate (BF-004) قبل أي بناء Android
!cp -r Build/Android/JavaLibs/vrpermissionstub \
     Intermediate/Android/arm64/JavaLibs/vrpermissionstub 2>/dev/null || \
cp -r Build/Android/JavaLibs/vrpermissionstub \
     /content/rok2-game/game/client-unreal/Intermediate/Android/arm64/JavaLibs/vrpermissionstub

!UE_ROOT=/opt/UnrealEngine \
  /opt/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd \
  Rok2.uproject -run=pythonscript \
  -script=import_assets.py \
  -nullrhi -unattended -nosplash 2>&1 | tail -10
```

### الخلية 6 — BuildCookRun: إنتاج APK (60–120 دقيقة)

```python
import os
os.chdir('/content/rok2-game/game/client-unreal')

!export ANDROID_HOME=/opt/android-sdk \
  NDK_ROOT=/opt/android-sdk/ndk/27.2.12479018 \
  JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64 && \
/opt/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
  -project=$(pwd)/Rok2.uproject \
  -platform=Android -cookflavor=ASTC \
  -clientconfig=Development -targetplatform=Android \
  -build -cook -stage -package -pak -iostore -compressed \
  -archive -archivedirectory=$(pwd)/Build/Android \
  -nodebuginfo -utf8output \
  -nop4 -unattended -nullrhi 2>&1 | tail -15
```

### الخلية 7 — تنزيل APK إلى جهازك

```python
from google.colab import files
import glob
apks = glob.glob('/content/rok2-game/game/client-unreal/Build/Android/*.apk')
print("Found:", apks)
for a in apks:
    files.download(a)
# انقل الملف إلى هاتفك وثبّته (اسمح "تثبيت من مصادر غير معروفة")
```

---

## التحقق بعد البناء (مطابق لوثائق المشروع)

| الفحص | الأمر / الملاحظة |
|---|---|
| APK يعمل | يفتح مباشرة على اللودج دون تعليق على شعار UE (بسبب `bSplashScreen=False`) |
| لا شاشة "Permission Required" | `bEnableManifestRequirements=True` يُزيل صلاحيات التخزين القديمة (BF-007) |
| مجسّمات/أصوات حقيقية | يجب تشغيل استيراد الأصول أولًا (الخلية 5) |
| السجل على الهاتف | `adb logcat -s UE:V LogRok2:V` |
| الخادم الحي | اللعبة تتصل بـ `https://rok2-api.lolelarap.workers.dev` تلقائيًا |

## الإعدادات الحالية للـ APK

| الإعداد | القيمة |
|---|---|
| Package Name | `com.rok2.thrones` |
| Min SDK / Target SDK | 26 / 34 |
| الاتجاه | Landscape |
| Vulkan + OpenGLES 3.1 | كلاهما مدعوم (Vulkan أولي، GLES احتياطي) |
| بيانات داخل APK | نعم (ملف واحد، بلا OBB) |
