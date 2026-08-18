#!/usr/bin/env python3
"""يبني لوحة تجميعية من فئة أصول RoK — النظر إلى مئات الملفات واحداً واحداً غير عملي.

الاستخدام:
    python build_contact_sheet.py 09_UI_Frames_HUD_and_Icons
    python build_contact_sheet.py 02_Buildings_and_City_Structures --cell 140 --cols 8
    python build_contact_sheet.py --list

يطبع مسار PNG الناتج؛ اقرأه بأداة قراءة الصور.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import sys

from PIL import Image

ROK_ASSETS = pathlib.Path(os.path.expanduser("~")) / "Desktop" / "ROK_Wiki_Assets"
BACKDROP = (30, 30, 34, 255)


def out_dir() -> pathlib.Path:
    base = pathlib.Path(os.environ.get("TEMP", "/tmp")) / "rokui"
    base.mkdir(parents=True, exist_ok=True)
    return base


def build(category: str, cols: int, cell: int, limit: int) -> pathlib.Path:
    source = ROK_ASSETS / category
    if not source.is_dir():
        sys.exit(f"فئة غير موجودة: {source}")

    files = sorted(
        p for p in source.iterdir()
        if p.is_file() and p.suffix.lower() in {".png", ".jpg", ".jpeg", ".webp"}
    )[:limit]
    if not files:
        sys.exit(f"لا صور في: {source}")

    rows = (len(files) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * cell), BACKDROP)
    placed = 0
    for index, path in enumerate(files):
        try:
            # بعض ملفات الويكي بامتداد .png ومحتوى WebP — PIL يكشف النوع فعلياً.
            image = Image.open(path).convert("RGBA")
        except Exception:
            continue
        image.thumbnail((cell - 10, cell - 10), Image.LANCZOS)
        x = (index % cols) * cell + (cell - image.width) // 2
        y = (index // cols) * cell + (cell - image.height) // 2
        sheet.alpha_composite(image, (x, y))
        placed += 1

    destination = out_dir() / f"sheet_{category}.png"
    sheet.convert("RGB").save(destination, quality=90)
    print(f"{placed}/{len(files)} صورة · {sheet.size[0]}x{sheet.size[1]}")
    print(destination)
    return destination


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("category", nargs="?", help="اسم مجلد الفئة داخل ROK_Wiki_Assets")
    parser.add_argument("--cols", type=int, default=10)
    parser.add_argument("--cell", type=int, default=110)
    parser.add_argument("--limit", type=int, default=120, help="سقف عدد الصور في اللوحة")
    parser.add_argument("--list", action="store_true", help="اسرد الفئات وأعدادها")
    args = parser.parse_args()

    if args.list or not args.category:
        if not ROK_ASSETS.is_dir():
            sys.exit(f"مجلد أصول RoK غير موجود: {ROK_ASSETS}")
        for entry in sorted(ROK_ASSETS.iterdir()):
            if entry.is_dir():
                print(f"{sum(1 for _ in entry.rglob('*') if _.is_file()):>5}  {entry.name}")
        return

    build(args.category, args.cols, args.cell, args.limit)


if __name__ == "__main__":
    main()
