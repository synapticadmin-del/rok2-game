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
JOBS = [
    ("Art/kaykit",         "/Game/Art/kaykit",         False),
    ("Art/Commanders",     "/Game/Art/Commanders",     False),
    ("Art/WorldMapIcons",  "/Game/Art/WorldMapIcons",  False),  # P7-T10: أيقونات خريطة العالم (14 PNG)
    ("Audio",              "/Game/Audio",              True),   # لازم: <civ>/music
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

            if unreal.EditorAssetLibrary.does_asset_exist("%s/%s.%s" % (dest, name, name)):
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

    for src_rel, dst_root, keep_tree in JOBS:
        tasks, skipped = collect(src_rel, dst_root, keep_tree)
        all_skipped += skipped
        if not tasks:
            log("[ROK2] %s: لا جديد للاستيراد." % src_rel)
            continue

        log("[ROK2] %s: استيراد %d ملف -> %s" % (src_rel, len(tasks), dst_root))
        # دفعة واحدة أسرع بكثير من ملف-ملف
        tools.import_asset_tasks(tasks)

        for t in tasks:
            created = list(t.get_editor_property("imported_object_paths") or [])
            if created:
                total_ok += 1
            else:
                total_fail += 1
                err("[ROK2] فشل استيراد: %s" % t.filename)

    unreal.EditorAssetLibrary.save_directory("/Game/Art", False, True)
    unreal.EditorAssetLibrary.save_directory("/Game/Audio", False, True)

    log("=" * 58)
    log("[ROK2] نجح: %d   فشل: %d   متخطّى: %d" % (total_ok, total_fail, len(all_skipped)))
    for p, why in all_skipped:
        warn("[ROK2] تخطٍّ %s — %s" % (p, why))
    log("=" * 58)

    if total_fail:
        # اجعل الفشل مرئياً لسكربت الـ bat
        sys.exit(1)


main()
