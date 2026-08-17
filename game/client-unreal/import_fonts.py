"""
ROK2 — استيراد ملفات .ttf في Content/Fonts/Faces كأصول FontFace

لماذا FontFace فقط ولا أصل `Font`؟
  `unreal.FontData` و`unreal.Typeface` غير مكشوفَين لـPython في UE 5.4، فلا
  سبيل لبناء CompositeFont داخل أصل Font من سكربت. البديل الأنظف: نستورد
  الأوجه السبعة كأصول FontFace، ويبني `URok2Typography` في C++ الـ
  CompositeFont من هذه الأوجه بأسماء أوزان Regular/Bold/Black. النتيجة نفسها
  في التشغيل، والأصول أقل عدداً وأسهل مراجعة.

يجب التشغيل بـ`-ExecutePythonScript` لا `-run=pythonscript`: استيراد FontFace
يستدعي `UFontFace::CacheSubFaces()` التي تطلب `FSlateApplication::Get()`،
وcommandlet الـpythonscript لا يُنشئ تطبيق Slate فيسقط المحرك بـ
`Assertion failed: CurrentApplication.IsValid()`.

  UnrealEditor-Cmd.exe <project>.uproject
      -ExecutePythonScript="import_fonts.py" -nullrhi -unattended -nosplash
"""
import os
import sys
import unreal

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FACES_DIR = os.path.join(SCRIPT_DIR, "Content", "Fonts", "Faces")
FACES_PKG = "/Game/Fonts/Faces"

# الأسماء التي يبحث عنها `URok2Typography::FacePackagePath` بالحرف.
EXPECTED = [
    "ArefRuqaa-Regular",
    "ArefRuqaa-Bold",
    "Cairo-Regular",
    "Cairo-Bold",
    "Cairo-Black",
    "Cinzel-Regular",
    "Cinzel-Bold",
]

log = unreal.log
warn = unreal.log_warning
err = unreal.log_error


def import_face(name):
    pkg = "%s/%s.%s" % (FACES_PKG, name, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pkg):
        log("[ROK2] موجود مسبقاً: %s" % name)
        return True

    src = os.path.join(FACES_DIR, name + ".ttf")
    if not os.path.isfile(src):
        err("[ROK2] ملف الخط غير موجود: %s" % src)
        return False

    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = FACES_PKG
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    created = list(task.get_editor_property("imported_object_paths") or [])
    if not created:
        err("[ROK2] فشل استيراد الخط: %s" % name)
        return False

    face = unreal.EditorAssetLibrary.load_asset(created[0])
    if not isinstance(face, unreal.FontFace):
        err("[ROK2] الأصل ليس FontFace: %s" % created[0])
        return False

    # Inline: البيانات تُحزَّم داخل الأصل لا كملف .ufont جانبي — يضمن وصولها
    # في APK أندرويد بلا اعتماد على مسار ملف خارجي.
    face.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
    unreal.EditorAssetLibrary.save_loaded_asset(face, False)
    log("[ROK2] FontFace: %s" % name)
    return True


def main():
    if not os.path.isdir(FACES_DIR):
        err("[ROK2] مجلد الأوجه غير موجود: %s" % FACES_DIR)
        sys.exit(2)

    failures = [n for n in EXPECTED if not import_face(n)]

    unreal.EditorAssetLibrary.save_directory(FACES_PKG, False, True)

    log("=" * 58)
    log("[ROK2] أوجه جاهزة: %d / %d" % (len(EXPECTED) - len(failures), len(EXPECTED)))
    log("=" * 58)

    if failures:
        sys.exit(1)


main()
