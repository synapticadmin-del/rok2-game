"""
ROK2 — استيراد الأصول الخام إلى .uasset  (يعمل HEADLESS بلا واجهة رسومية)

لماذا منفصل عن setup_level.py؟
  setup_level.py يعدّل المستوى ويحتاج محرراً رسومياً حياً. هذا السكربت
  يستخدم AssetTools فقط، فيعمل داخل commandlet مع -nullrhi — أي بلا أي
  اعتماد على كرت الشاشة. هذا ما يسمح ببناء أصول حقيقية على جهاز كرت
  شاشته لا يدعم SM6.

التشغيل:
  ImportAssets.bat            (مستحسن)
أو يدوياً:
  UnrealEditor-Cmd.exe <project>.uproject -run=pythonscript
      -script="import_assets.py" -nullrhi -unattended -nosplash

يُصلح هذا السكربت مشكلتين كانتا في setup_level.py:
  1. تسطيح الشجرة: كان يستورد كل ملفات Audio إلى /Game/Audio مباشرة،
     فتتصادم ملفات music.wav الستة (حضارة واحدة تنجو). الكود يتوقع
     /Game/Audio/<civ>/music و /Game/Audio/sfx/<name>.
  2. الاعتماد على ترميز base64 — الأصول الآن ثنائيات حقيقية في المستودع.
"""
import base64
import os
import sys
import unreal

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CONTENT = os.path.join(SCRIPT_DIR, "Content")

MAGIC = {".glb": b"glTF", ".wav": b"RIFF", ".png": b"\x89PNG"}

# (مجلد المصدر النسبي, وجهة /Game, هل نحافظ على الشجرة الفرعية؟)
#
# ملاحظة صيانة 2026-08-15: كانت القائمة تحتوي kaykit/Commanders/WorldMapIcons/Audio
# فقط، فبقيت 94 صورة واجهة (أيقونات، جلود أزرار، صور مباني، شعارات وخلفيات
# الحضارات) بصيغة PNG على القرص بلا .uasset — أي غير موجودة للعبة. النتيجة:
# GetIconBrush يسقط إلى الراسم الإجرائي 32×32، وApplyButtonSkin يعود بلا جلد
# فتظهر أزرار Slate الرمادية الافتراضية. هذه أهم أسباب رداءة الواجهة.
JOBS = [
    ("Art/kaykit",            "/Game/Art/kaykit",            False),
    ("Art/Commanders",        "/Game/Art/Commanders",        False),
    ("Art/WorldMapIcons",     "/Game/Art/WorldMapIcons",     False),  # P7-T10: أيقونات خريطة العالم
    ("Art/UIIcons",           "/Game/Art/UIIcons",           False),  # أيقونات الواجهة (20)
    ("Art/UIButtons",         "/Game/Art/UIButtons",         False),  # جلود الأزرار (4)
    ("Art/CityBuildingIcons", "/Game/Art/CityBuildingIcons", False),  # صور المباني (58)
    ("Art/CivIcons",          "/Game/Art/CivIcons",          False),  # شعارات الحضارات (6)
    ("Art/CivBackgrounds",    "/Game/Art/CivBackgrounds",    False),  # خلفيات الحضارات (6)
    ("Art/Tavern",            "/Game/Art/Tavern",            False),  # P10-T7: الحانة والصناديق
    ("Audio",                 "/Game/Audio",                 True),   # لازم: <civ>/music
]

log = unreal.log
warn = unreal.log_warning
err = unreal.log_error


def decode_in_place(path, magic):
    """يفك ترميز base64 في مكانه مع الحفاظ على الامتداد.

    بعض الأصول محفوظة في المستودع كنص base64 (لأن رفع الثنائيات عبر واجهة
    GitHub النصية يفسدها). الفك idempotent: الملفات الثنائية أصلاً تُترك.
    الحفاظ على الامتداد مهم — المحاولة القديمة كانت تكتب ملف '.bin' جانبي
    فيرفضه المحرك بخطأ 'unknown extension'.
    """
    try:
        with open(path, "rb") as fh:
            raw = fh.read()
        if raw[: len(magic)] == magic:
            return False                      # ثنائي سليم
        decoded = base64.b64decode(raw, validate=True)
        if decoded[: len(magic)] != magic:
            return False                      # ليس base64 لهذا النوع
        with open(path, "wb") as fh:
            fh.write(decoded)
        return True
    except Exception:
        return False


def is_valid_binary(path, ext):
    """يرفض الملفات النصية/الفارغة قبل تسليمها للمستورد."""
    magic = MAGIC.get(ext)
    if not magic:
        return False
    try:
        with open(path, "rb") as fh:
            return fh.read(len(magic)) == magic
    except OSError:
        return False


def already_imported(dest, name, src_path, ext):
    """هل الأصل موجود فعلاً؟ يفحص المسار المسطّح والشجرة التي ينتجها مستورد glTF.

    مستورد Interchange لملفات .glb لا ينتج أصلاً واحداً باسم الملف، بل مجلداً
    فيه StaticMeshes/ و Materials/ و Textures/. الفحص القديم كان يسأل عن
    /Game/Art/kaykit/building_castle.building_castle فلا يجده أبداً، فيعيد
    استيراد كل ملفات GLB في كل تشغيل — وهذا ما كان يُسقط المحرك بـ
    `Assertion failed: IsValid()` داخل AssetTools ويوقف بقية الاستيراد.
    """
    if unreal.EditorAssetLibrary.does_asset_exist("%s/%s.%s" % (dest, name, name)):
        return True

    if ext == ".glb":
        nested = "%s/%s/StaticMeshes/%s.%s" % (dest, name, name, name)
        if unreal.EditorAssetLibrary.does_asset_exist(nested):
            return True
        # حتى لو تغيّر اسم الميش داخل الملف، وجود المجلد يكفي دليلاً
        if unreal.EditorAssetLibrary.does_directory_exist("%s/%s" % (dest, name)):
            return True

    return False


def collect(src_rel, dst_root, keep_tree):
    src_root = os.path.join(CONTENT, src_rel)
    if not os.path.isdir(src_root):
        warn("[ROK2] مجلد غير موجود، تخطٍّ: %s" % src_root)
        return [], []

    tasks, skipped = [], []
    for dirpath, _dirs, files in os.walk(src_root):
        for fname in sorted(files):
            ext = os.path.splitext(fname)[1].lower()
            if ext not in MAGIC:
                continue
            src = os.path.join(dirpath, fname)
            name = os.path.splitext(fname)[0]

            if decode_in_place(src, MAGIC[ext]):
                log("[ROK2] فُكّ ترميز base64: %s" % fname)

            if not is_valid_binary(src, ext):
                skipped.append((src, "ليس ثنائياً صالحاً (ما زال base64؟)"))
                continue

            dest = dst_root
            if keep_tree:
                rel = os.path.relpath(dirpath, src_root)
                if rel not in (".", ""):
                    dest = dst_root + "/" + rel.replace("\\", "/")

            if already_imported(dest, name, src, ext):
                continue

            t = unreal.AssetImportTask()
            t.filename = src
            t.destination_path = dest
            t.destination_name = name
            t.replace_existing = True
            t.automated = True
            t.save = True
            tasks.append(t)
    return tasks, skipped


def main():
    total_ok = 0
    total_fail = 0
    all_skipped = []

    tools = unreal.AssetToolsHelpers.get_asset_tools()

    # ROK2_JOB يقصر التشغيل على وجهة واحدة. ImportAssets.bat يستدعي السكربت
    # مرة لكل مجلد لأن استيراد كل شيء في جلسة واحدة كان يُسقط المحرك بـ
    # `Assertion failed: IsValid()` داخل AssetTools بعد عدة مئات من الملفات؛
    # كل مجلد على حدة ينجح، والجلسة الجديدة تبدأ من حالة نظيفة.
    only = os.environ.get("ROK2_JOB", "").strip()
    jobs = [j for j in JOBS if not only or j[0] == only]
    if only and not jobs:
        err("[ROK2] ROK2_JOB=%s لا يطابق أي مهمة معروفة." % only)
        sys.exit(2)

    for src_rel, dst_root, keep_tree in jobs:
        tasks, skipped = collect(src_rel, dst_root, keep_tree)
        all_skipped += skipped
        if not tasks:
            log("[ROK2] %s: لا جديد للاستيراد." % src_rel)
            continue

        log("[ROK2] %s: استيراد %d ملف -> %s" % (src_rel, len(tasks), dst_root))

        for t in tasks:
            fname = os.path.basename(t.filename)
            try:
                tools.import_asset_tasks([t])
            except Exception as exc:
                total_fail += 1
                err("[ROK2] استثناء أثناء استيراد %s — %s" % (fname, exc))
                continue

            created = list(t.get_editor_property("imported_object_paths") or [])
            if created:
                total_ok += 1
            else:
                total_fail += 1
                err("[ROK2] فشل استيراد: %s" % fname)

    for save_root in ("/Game/Art", "/Game/Audio"):
        if unreal.EditorAssetLibrary.does_directory_exist(save_root):
            unreal.EditorAssetLibrary.save_directory(save_root, False, True)

    log("=" * 58)
    log("[ROK2] نجح: %d   فشل: %d   متخطّى: %d" % (total_ok, total_fail, len(all_skipped)))
    for p, why in all_skipped:
        warn("[ROK2] تخطٍّ %s — %s" % (p, why))
    log("=" * 58)

    if total_fail:
        # اجعل الفشل مرئياً لسكربت الـ bat
        sys.exit(1)


main()
