#!/usr/bin/env python3
"""يقيس خصائص PNG من بايتاته — للحكم على أصل لا على نصّ الكود الذي يقرأه.

يقيس ما ثبت أنه يفسد الشكل في هذا المشروع:
  · رقعة شطرنج **مطبوعة**: بكسلات رمادية فاتحة معتمة تُرسم كخلفية حقيقية
  · غياب الشفافية أصلاً (`alpha == 255` في كل بكسل)
  · الأبعاد — أصلٌ 1920px يُعرض بـ64px يحتاج مقاس رسم صريح

الاستخدام:
    python measure_png.py <ملف|مجلد> [...]
    python measure_png.py game/client-unreal/Content/Art/CivIcons --json
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

import numpy as np
from PIL import Image

# العتبات نفسها المستخدمة في scripts/strip_civ_emblem_background.py وحارس P24-T9.
NEUTRAL_SATURATION = 8
NEUTRAL_MIN_VALUE = 225


def measure(path: pathlib.Path) -> dict:
    image = Image.open(path).convert("RGBA")
    data = np.array(image)
    alpha = data[:, :, 3]
    rgb = data[:, :, :3].astype(np.int16)

    opaque = alpha == 255
    neutral_light = (
        opaque
        & (rgb.max(2) - rgb.min(2) <= NEUTRAL_SATURATION)
        & (rgb.min(2) >= NEUTRAL_MIN_VALUE)
    )
    total = alpha.size
    return {
        "file": path.name,
        "width": int(image.size[0]),
        "height": int(image.size[1]),
        "transparentPixels": int((alpha == 0).sum()),
        "transparentRatio": round(float((alpha == 0).mean()), 4),
        "opaqueNeutralLightPixels": int(neutral_light.sum()),
        "opaqueNeutralLightRatio": round(float(neutral_light.sum() / total), 4),
        "hasAnyTransparency": bool(int(alpha.min()) < 255),
    }


def verdict(row: dict) -> str:
    if not row["hasAnyTransparency"]:
        return "لا شفافية أصلاً — الخلفية مطبوعة في البكسل"
    if row["opaqueNeutralLightRatio"] > 0.02:
        return "رمادي فاتح معتم مرتفع — رقعة مطبوعة محتملة"
    if row["transparentRatio"] < 0.05:
        return "شفافية ضئيلة — تحقّق أن الأصل يُقصَد معتماً"
    return "سليم"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    targets: list[pathlib.Path] = []
    for raw in args.paths:
        path = pathlib.Path(raw)
        if path.is_dir():
            targets.extend(sorted(path.glob("*.png")))
        elif path.is_file():
            targets.append(path)
        else:
            sys.exit(f"مسار غير موجود: {path}")

    rows = [measure(p) for p in targets]
    if args.json:
        print(json.dumps(rows, ensure_ascii=False, indent=2))
        return

    for row in rows:
        print(
            f"{row['file']:<34} {row['width']}x{row['height']:<6}"
            f" شفاف={row['transparentRatio'] * 100:5.1f}%"
            f" رماديفاتح={row['opaqueNeutralLightRatio'] * 100:5.2f}%"
            f"  {verdict(row)}"
        )


if __name__ == "__main__":
    main()
