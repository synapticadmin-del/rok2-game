#!/usr/bin/env python3
"""يفكّ لقطات `07-game-design/assets` — ملفاتها بامتداد .jpg ومحتواها base64 لا JPEG.

كل قراءة مباشرة لها تفشل بـ`UnidentifiedImageError`، ولهذا بقيت المراجع البصرية
الملزمة في المشروع غير مقروءة لجلسات كاملة. البايتات الأولى `2f396a2f34...` أي
نصّ ASCII يبدأ بـ`/9j/4` — وهو ترويسة JPEG مُرمَّزة base64.

الاستخدام:
    python decode_reference_images.py            # يفكّ كل الملفات
    python decode_reference_images.py ui-city    # ما يطابق النمط
"""

from __future__ import annotations

import base64
import io
import os
import pathlib
import sys

from PIL import Image

REPO_ROOT = pathlib.Path(__file__).resolve().parents[4]
ASSETS = REPO_ROOT / "07-game-design" / "assets"
MAX_EDGE = 1400


def out_dir() -> pathlib.Path:
    base = pathlib.Path(os.environ.get("TEMP", "/tmp")) / "rokref"
    base.mkdir(parents=True, exist_ok=True)
    return base


def decode(path: pathlib.Path, destination: pathlib.Path) -> tuple[int, int] | None:
    raw = path.read_bytes()
    try:
        image = Image.open(io.BytesIO(raw))
    except Exception:
        try:
            image = Image.open(io.BytesIO(base64.b64decode(raw.decode("ascii").strip())))
        except Exception as error:
            print(f"  تخطٍّ {path.name}: {error}")
            return None

    width, height = image.size
    scale = min(1.0, MAX_EDGE / max(width, height))
    if scale < 1.0:
        image = image.resize((int(width * scale), int(height * scale)), Image.LANCZOS)
    image.convert("RGB").save(destination, quality=88)
    return width, height


def main() -> None:
    if not ASSETS.is_dir():
        sys.exit(f"مجلد المراجع غير موجود: {ASSETS}")

    pattern = sys.argv[1] if len(sys.argv) > 1 else ""
    target = out_dir()
    count = 0
    for path in sorted(ASSETS.iterdir()):
        if not path.is_file() or pattern and pattern not in path.name:
            continue
        destination = target / (path.stem + ".jpg")
        size = decode(path, destination)
        if size:
            count += 1
            print(f"  {path.name}  {size[0]}x{size[1]}  ->  {destination}")

    if not count:
        sys.exit("لم يُفكّ أي ملف")
    print(f"\n{count} مرجعاً في: {target}")


if __name__ == "__main__":
    main()
