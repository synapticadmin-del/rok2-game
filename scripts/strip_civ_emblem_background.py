#!/usr/bin/env python3
"""P24-T9: يفصل خلفية شعارات الحضارات المطبوعة داخل البكسل.

المشكلة المقيسة: ثلاثة من الشعارات الستة في `Content/Art/CivIcons` تحمل
**رقعة شطرنج مطبوعة** — قناة الشفافية فيها 255 في كل بكسل (لا شفافية أصلاً)،
والمربعات الرمادية/البيضاء التي تعني «شفاف» في محرر الصور صارت **بكسلات
معتمة**. فأي عرض للشعار يرسم تلك الرقعة كخلفية حقيقية.

  icon_rome_runtime.png    alpha0=0  بكسلات رمادية فاتحة معتمة: 55%
  icon_china_runtime.png   alpha0=0  ...                        46%
  icon_arabia_runtime.png  alpha0=0  ...                        53%

والثلاثة الأخرى (egypt/japan/vikings) شفافيتها سليمة — خلفيتها الخضراء
مفصولة فعلاً (1.4–1.6 مليون بكسل بشفافية صفر) — فيتخطّاها هذا السكربت.

المعايير ثوابت مسمّاة لا حكم بصري:

  NEUTRAL_SATURATION  فرق أقصى/أدنى قناة ≤ 8   — الرقعة رمادية محضة
  NEUTRAL_MIN_VALUE   أدنى قناة ≥ 225          — الرقعة فاتحة (234–255 مقيسة)
  BLEED_ITERATIONS    8                        — تسريب اللون داخل الشفاف

**لماذا تسريب اللون:** ترك البكسل الشفاف أبيضَ يُنتج هالة بيضاء عند تصغير
1920px إلى 76px، لأن mipmap يمزج RGB مع تجاهل alpha. فلون البكسل الشفاف
يُستبدَل بلون أقرب بكسل معتم بثماني تكرارات توسيع — والشفافية تبقى صفراً.

يعمل من جذر المستودع بلا مسارات محلية مؤقتة، وتشغيله مكرراً آمن: ملفٌ فيه
شفافية أصلاً يُتخطّى.
"""

from __future__ import annotations

import hashlib
import json
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image

NEUTRAL_SATURATION = 8
NEUTRAL_MIN_VALUE = 225
BLEED_ITERATIONS = 8

REPO_ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = REPO_ROOT / "game" / "client-unreal" / "Content" / "Art" / "CivIcons"
# التقرير **خارج** `Content/`: أي ملف غير أصل داخل مجلد المحتوى يحاول محرر UE
# استيراده عند فتح المشروع — و`alpha_report.json` فتح حوار «استيراد DataTable».
REPORT_PATH = REPO_ROOT / "game" / "docs" / "P24_T9_CIV_EMBLEM_ALPHA.json"
CIVS = ("rome", "china", "arabia", "egypt", "vikings", "japan")


def border_connected(mask: np.ndarray) -> np.ndarray:
    """المركّب المتصل بحافة الصورة داخل القناع (8-connectivity).

    الخلفية تُعرَّف بالاتصال بالحافة لا باللون وحده: رقعة الشطرنج تظهر كذلك
    **داخل** الشعار (تجاويف الإكليل)، وتلك يجب أن تُفتح أيضاً — لكن مربعاً
    رمادياً هو جزء من الرسم (حجر، فضّة) لا يجوز أن يُثقب. الاتصال بالحافة هو
    الفصل الوحيد القابل للقياس بين الحالتين.
    """
    height, width = mask.shape
    seen = np.zeros_like(mask)
    queue: deque[tuple[int, int]] = deque()

    for x in range(width):
        for y in (0, height - 1):
            if mask[y, x] and not seen[y, x]:
                seen[y, x] = True
                queue.append((y, x))
    for y in range(height):
        for x in (0, width - 1):
            if mask[y, x] and not seen[y, x]:
                seen[y, x] = True
                queue.append((y, x))

    while queue:
        y, x = queue.popleft()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                ny, nx = y + dy, x + dx
                if 0 <= ny < height and 0 <= nx < width and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    queue.append((ny, nx))
    return seen


def bleed_color_into_transparent(rgb: np.ndarray, opaque: np.ndarray) -> np.ndarray:
    """يمدّ لون البكسلات المعتمة داخل الشفافة، فلا هالة بيضاء عند التصغير."""
    filled = rgb.copy()
    known = opaque.copy()
    for _ in range(BLEED_ITERATIONS):
        if known.all():
            break
        total = np.zeros(rgb.shape, dtype=np.int32)
        count = np.zeros(known.shape, dtype=np.int32)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy == 0 and dx == 0:
                    continue
                shifted_rgb = np.roll(np.roll(filled, dy, axis=0), dx, axis=1)
                shifted_known = np.roll(np.roll(known, dy, axis=0), dx, axis=1)
                total += shifted_rgb.astype(np.int32) * shifted_known[..., None]
                count += shifted_known.astype(np.int32)
        target = (~known) & (count > 0)
        if not target.any():
            break
        safe = np.maximum(count, 1)[..., None]
        averaged = (total // safe).astype(np.uint8)
        filled[target] = averaged[target]
        known |= target
    return filled


def strip_printed_background(path: Path) -> dict:
    image = Image.open(path).convert("RGBA")
    data = np.array(image)
    alpha = data[:, :, 3]
    rgb = data[:, :, :3]

    if int(alpha.min()) < 255:
        # الملف يحمل شفافية فعلاً: إما أصلٌ سليم من البداية (egypt/japan/vikings)
        # أو ملفٌ عالجه هذا السكربت قبلاً. القياس يُسجَّل كما هو ولا يُفترض صفراً،
        # فيبقى التقرير صادقاً عند إعادة التشغيل بدل أن يُبلّغ نظافةً لم تُقَس.
        signed_existing = rgb.astype(np.int16)
        residue = (
            (alpha == 255)
            & (signed_existing.max(2) - signed_existing.min(2) <= NEUTRAL_SATURATION)
            & (signed_existing.min(2) >= NEUTRAL_MIN_VALUE)
        )
        return {
            "file": path.name,
            "action": "skipped",
            "reason": "already has an alpha channel with transparency",
            "transparentPixels": int((alpha == 0).sum()),
            "opaqueNeutralLightPixels": int(residue.sum()),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        }

    signed = rgb.astype(np.int16)
    neutral = (signed.max(2) - signed.min(2) <= NEUTRAL_SATURATION) & (
        signed.min(2) >= NEUTRAL_MIN_VALUE
    )
    background = border_connected(neutral)
    if not background.any():
        raise SystemExit(f"لا خلفية رمادية متصلة بالحافة في {path.name} — المعايير لا تنطبق")

    opaque = ~background
    filled_rgb = bleed_color_into_transparent(rgb, opaque)
    out = np.dstack([filled_rgb, np.where(background, 0, 255).astype(np.uint8)])
    Image.fromarray(out, mode="RGBA").save(path, format="PNG", optimize=True)

    verify = np.array(Image.open(path).convert("RGBA"))
    verify_alpha = verify[:, :, 3]
    verify_rgb = verify[:, :, :3].astype(np.int16)
    still_neutral = (
        (verify_alpha == 255)
        & (verify_rgb.max(2) - verify_rgb.min(2) <= NEUTRAL_SATURATION)
        & (verify_rgb.min(2) >= NEUTRAL_MIN_VALUE)
    )
    return {
        "file": path.name,
        "action": "stripped",
        "printedBackgroundPixels": int(background.sum()),
        "transparentPixels": int((verify_alpha == 0).sum()),
        "opaqueNeutralLightPixels": int(still_neutral.sum()),
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    }


def main() -> None:
    if not ICON_DIR.is_dir():
        raise SystemExit(f"مجلد الشعارات غير موجود: {ICON_DIR}")

    entries = []
    for civ in CIVS:
        path = ICON_DIR / f"icon_{civ}_runtime.png"
        if not path.exists():
            raise SystemExit(f"شعار مفقود: {path}")
        entry = strip_printed_background(path)
        entries.append(entry)
        print(
            f"  {entry['action']:<9} {entry['file']}"
            f"  transparent={entry['transparentPixels']}"
            f"  opaqueNeutralLight={entry['opaqueNeutralLightPixels']}"
        )

    report = {
        "task": "P24-T9",
        "criteria": {
            "neutralSaturation": NEUTRAL_SATURATION,
            "neutralMinValue": NEUTRAL_MIN_VALUE,
            "bleedIterations": BLEED_ITERATIONS,
        },
        "icons": entries,
    }
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"\nتقرير الشفافية: {REPORT_PATH.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
