#!/usr/bin/env python3
"""يفك ترميز الأصول الثنائية المخزنة base64 في الريبو (WAV/PNG/GLB) إلى ملفات
binary حقيقية قبل استيرادها في Unreal — نفس نمط _ensure_binary_glb (P2-T7).

تخزين git للثنائيات في هذا المشروع يتم كنص base64 (يفك ترميزه إلى الملف
الأصلي). هذا السكربت يعيد كتابة الملفات في مكانها بالنسخة الثنائية.
يعمل خارج Unreal (python عادي) وداخله.

الاستخدام:  python3 scripts/decode_binary_assets.py [repo_root]
"""
import base64
import os
import sys

MAGIC = {
    ".wav": b"RIFF",
    ".png": b"\x89PNG",
    ".glb": b"glTF",
}

SCAN_DIRS = [
    os.path.join("game", "client-unreal", "Content", "Audio"),
    os.path.join("game", "client-unreal", "Content", "Art", "Commanders"),
    os.path.join("game", "client-unreal", "Content", "Art", "kaykit"),
]


def decode_file(path):
    ext = os.path.splitext(path)[1].lower()
    magic = MAGIC.get(ext)
    if not magic:
        return False
    with open(path, "rb") as fh:
        head = fh.read(len(magic))
    if head == magic:
        return False  # binary سليم أصلاً
    with open(path, "rb") as fh:
        raw = fh.read()
    try:
        decoded = base64.b64decode(raw, validate=True)
    except Exception:
        return False
    if decoded[: len(magic)] != magic:
        return False  # ليس base64 لهذه الصيغة
    with open(path, "wb") as fh:
        fh.write(decoded)
    return True


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
    fixed = 0
    scanned = 0
    for sub in SCAN_DIRS:
        base = os.path.join(root, sub)
        for dirpath, _dirs, files in os.walk(base):
            for name in files:
                if os.path.splitext(name)[1].lower() not in MAGIC:
                    continue
                scanned += 1
                p = os.path.join(dirpath, name)
                if decode_file(p):
                    print(f"decoded: {os.path.relpath(p, root)}")
                    fixed += 1
    print(f"done: {fixed}/{scanned} files decoded to binary.")


if __name__ == "__main__":
    main()
