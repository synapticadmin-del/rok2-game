#!/usr/bin/env python3
"""تقطيع صفائح معالم خريطة العالم إلى sprites مفردة (P24-T6).

المشكلة: 24 ملفاً في `Content/Art/WorldMapIcons/T_world_*.png` ليست sprites
مفردة بل **صفائح**:

  • `T_world_resource_nodes_quad` يحمل أربع عقد موارد في شبكة 2×2،
    وتحت كل واحدة **نص عربي مطبوع داخل الصورة** («حقل قمح»، «معسكر أخشاب»،
    «مقلع حجارة»، «منجم ذهب»).
  • `T_world_stone_gold_quarry_mine` يحمل منشأتين: منجماً كبيراً وبرجاً
    صغيراً مقصوصاً على الحافة اليسرى.
  • خمس صفائح أخرى تحمل جسماً رئيسياً واحداً **زائد قصاصة** من الجسم المجاور
    في الصفيحة الأصلية (نتيجة تقطيع P16 بشبكة ثابتة لا بحدود الأجسام).

فرسمها كما هي على الخريطة يعني: عقدة قمح تحمل ثلاث عقد أخرى ونصاً عربياً
مقلوباً، أو ممراً بجواره ربع جبل معلّق في الهواء.

الحل: تحليل مركّبات الشفافية المتصلة (connected components على قناة alpha)
لكل صفيحة، ثم قصّ كل مركّب يستحق أن يكون sprite. المركّبات الصغيرة (النص
العربي) والملاصقة لحافة الصورة (قصاصات الجيران) تُستبعد بمعيارين قابلين
للفحص لا بحكم بصري.

**المصادر داخل المستودع** — لا مسارات محلية مؤقتة كما في
`slice_and_process_all_assets.py` (الذي يقرأ من `.gemini/antigravity/brain/...`
فلا يمكن إعادة تشغيله على جهاز آخر). وخرائط N/E تُقصّ من صفائح N/E نفسها بنفس
الإحداثيات، فلا يُعاد توليدها ولا يضيع أصلها.

الاستخدام:
    python scripts/slice_world_feature_sprites.py            # يكتب الأصول
    python scripts/slice_world_feature_sprites.py --dry-run  # تقرير بلا كتابة
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import deque

try:
    import numpy as np
    from PIL import Image
except ImportError as exc:  # pragma: no cover
    sys.exit(f"يتطلب Pillow وnumpy: {exc}")

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = os.path.join(REPO, "game", "client-unreal", "Content", "Art", "WorldMapIcons")
OUT_DIR = os.path.join(REPO, "game", "client-unreal", "Content", "Art", "WorldFeatures")

# عتبة الشفافية التي تُحتسب معها البكسل «جسماً». 24 لا 0: حواف الرسم مصفّاة
# (anti-aliased) فتترك هالة شفافة تصل المركّبات المنفصلة لو أخذنا أي قيمة > 0.
ALPHA_THRESHOLD = 24

# أصغر مساحة تُحتسب sprite. النصوص العربية المطبوعة في صفيحة الموارد مساحتها
# 180–210 بكسل، وأصغر جسم حقيقي 1400+ — فالحد 900 يفصلهما بهامش واسع.
MIN_SPRITE_AREA = 900

# مركّب يلمس حافة الصورة **وهو أصغر بكثير** من الجسم الرئيسي هو على الأرجح
# قصاصة من جسم مجاور قُطع في الصفيحة الأصلية.
#
# لمس الحافة وحده لا يكفي حكماً: في صفيحة الموارد تصل عقدتان حقيقيتان
# (معسكر الأخشاب ومنجم الذهب) إلى الحدّ الأيمن تماماً، فاستبعاد كل ما يلمس
# الحافة كان يحذف نصف المحتوى المطلوب.
#
# النسبة المقيسة تفصل الحالتين بهامش واسع: القصاصات 6.7%–15.6% من مساحة الجسم
# الرئيسي (سليفر المنجم 1412/21084، والمعبد 3190/20426، والممر 2233/28241)،
# بينما العقد الحقيقية في صفيحة الموارد 77%–79% منه.
EDGE_TOUCH_MARGIN = 1
FRAGMENT_AREA_RATIO = 0.30

# هامش شفّاف حول كل sprite. البِلبورد يرسم النسيج كاملاً، وبلا هامش تلتصق
# الحافة المصفّاة بحدّ النسيج فتظهر خطوطاً عند التصغير.
PAD = 6


def load_alpha(path: str) -> np.ndarray:
    return np.array(Image.open(path).convert("RGBA"))[:, :, 3]


def connected_components(alpha: np.ndarray) -> tuple[list[dict], np.ndarray]:
    """مركّبات متصلة (8-connectivity) مرتّبة بالمساحة تنازلياً + خريطة تسمية.

    خريطة التسمية ليست ترفاً: القصّ بمستطيل وحده يسحب معه ما يقع داخله من
    أجسام أخرى — ونصوص صفيحة الموارد تقع على بعد 2 بكسل تحت قاعدة العقدة، فأي
    هامش يعيدها. الطمس بالمركّب نفسه يجعل الاستبعاد قاطعاً لا تقريبياً.
    """
    height, width = alpha.shape
    solid = alpha > ALPHA_THRESHOLD
    labels = np.zeros((height, width), dtype=np.int32)
    found: list[dict] = []
    next_label = 0

    for start_y in range(height):
        row = solid[start_y]
        for start_x in range(width):
            if not row[start_x] or labels[start_y, start_x]:
                continue

            next_label += 1
            queue = deque([(start_y, start_x)])
            labels[start_y, start_x] = next_label
            min_x = max_x = start_x
            min_y = max_y = start_y
            area = 0

            while queue:
                y, x = queue.popleft()
                area += 1
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < height and 0 <= nx < width and not labels[ny, nx] and solid[ny, nx]:
                            labels[ny, nx] = next_label
                            queue.append((ny, nx))

            found.append(
                {
                    "label": next_label,
                    "x": min_x,
                    "y": min_y,
                    "w": max_x - min_x + 1,
                    "h": max_y - min_y + 1,
                    "area": area,
                }
            )

    found.sort(key=lambda c: -c["area"])
    return found, labels


def touches_edge(comp: dict, width: int, height: int) -> bool:
    return (
        comp["x"] <= EDGE_TOUCH_MARGIN
        or comp["y"] <= EDGE_TOUCH_MARGIN
        or comp["x"] + comp["w"] >= width - EDGE_TOUCH_MARGIN
        or comp["y"] + comp["h"] >= height - EDGE_TOUCH_MARGIN
    )


# ---------------------------------------------------------------------------
# خطة القصّ.
#
# `expect` عدد المركّبات المقبولة المتوقّعة، و`names` أسماء الأجسام مرتّبة
# **مكانياً** (أعلى-لأسفل ثم يسار-ليمين) لا بالمساحة — الترتيب المكاني هو ما
# يطابق ما رآه الإنسان في الصورة، وثباته يجعل الأسماء صحيحة مهما تغيّرت
# مساحة جسم بعد إعادة توليد.
#
# الأسماء مقروءة من الصور بالفحص البصري (مسجّل في
# game/docs/P24_T6_WORLD_FEATURE_SPRITES.md) لا مخترعة من اسم الملف.
# ---------------------------------------------------------------------------
PLAN = [
    {
        "sheet": "T_world_resource_nodes_quad",
        "expect": 4,
        "names": ["farm_field", "lumber_camp", "stone_quarry", "gold_mine"],
    },
    {
        "sheet": "T_world_stone_gold_quarry_mine",
        "expect": 1,
        "names": ["gold_mine_large"],
    },
    {
        "sheet": "T_world_barbarian_fort_camp",
        "expect": 1,
        "names": ["barbarian_camp"],
    },
    {
        "sheet": "T_world_barbarian_keep_outpost",
        "expect": 1,
        "names": ["barbarian_keep"],
    },
    {
        "sheet": "T_world_mountain_pass_fortress",
        "expect": 1,
        "names": ["pass_fortress"],
    },
    {
        "sheet": "T_world_lost_temple_throne_core",
        "expect": 1,
        "names": ["throne_temple"],
    },
    {
        "sheet": "T_world_holy_shrine_altar",
        "expect": 1,
        "names": ["holy_shrine"],
    },
    {
        "sheet": "T_world_mountain_ridge_barrier",
        "expect": 1,
        "names": ["mountain_ridge"],
    },
]

MAPS = ("D", "N", "E")


def accept_components(comps: list[dict], width: int, height: int, expect: int) -> list[dict]:
    """يقبل المركّبات التي تستحق sprite، بترتيب مكاني."""
    kept = [c for c in comps if c["area"] >= MIN_SPRITE_AREA]

    # قصاصات الجيران: تلمس الحافة **وهي صغيرة** نسبةً للجسم الرئيسي. المعيار
    # مزدوج لأن أجساماً حقيقية تلمس الحافة (عقدتان في صفيحة الموارد تصلان
    # الحدّ الأيمن)، والحجم وحده لا يفصل كذلك (سليفر المعبد 3190 بكسل أكبر من
    # حدّ النص بكثير).
    if kept:
        largest_area = kept[0]["area"]
        kept = [
            c
            for c in kept
            if c["area"] == largest_area
            or not touches_edge(c, width, height)
            or c["area"] >= largest_area * FRAGMENT_AREA_RATIO
        ]

    kept = kept[:expect]

    # ترتيب مكاني: صفوف من الأعلى، وداخل الصف من اليسار.
    #
    # التجميع بـ**مركز** الجسم لا بحدّه الأعلى: في صفيحة الموارد يبدأ معسكر
    # الأخشاب عند y=8 وحقل القمح عند y=33 وهما في الصف نفسه بصرياً (الأشجار
    # أعلى من سنابل القمح)، فالتجميع بالحدّ الأعلى يضعهما في صفّين ويقلب
    # الاسمين. المراكز 64.5 و70.5 متجاورة، والصف الثاني عند 168 و171.
    ROW_BUCKET = 60.0
    kept.sort(key=lambda c: (round((c["y"] + c["h"] / 2.0) / ROW_BUCKET), c["x"]))
    return kept


def main() -> int:
    parser = argparse.ArgumentParser(description="تقطيع صفائح معالم العالم إلى sprites مفردة")
    parser.add_argument("--dry-run", action="store_true", help="تقرير بلا كتابة أي ملف")
    args = parser.parse_args()

    if not os.path.isdir(SRC_DIR):
        print(f"[FAIL] مجلد المصادر غير موجود: {SRC_DIR}")
        return 1

    if not args.dry_run:
        os.makedirs(OUT_DIR, exist_ok=True)

    manifest: dict[str, dict] = {}
    problems: list[str] = []
    written = 0

    for entry in PLAN:
        sheet = entry["sheet"]
        expect = entry["expect"]
        names = entry["names"]

        sources = {m: os.path.join(SRC_DIR, f"{sheet}_{m}.png") for m in MAPS}
        missing = [m for m, p in sources.items() if not os.path.isfile(p)]
        if missing:
            problems.append(f"{sheet}: خرائط ناقصة {missing}")
            continue

        images = {m: Image.open(p).convert("RGBA") for m, p in sources.items()}
        width, height = images["D"].size

        # كل الخرائط الثلاث يجب أن تكون بالأبعاد نفسها، وإلا فالقصّ بإحداثيات
        # الألبيدو يقطع موضعاً آخر من خريطة العمق.
        for m in ("N", "E"):
            if images[m].size != (width, height):
                problems.append(f"{sheet}: أبعاد {m} ({images[m].size}) ≠ D ({(width, height)})")
        if problems and problems[-1].startswith(sheet):
            continue

        comps, labels = connected_components(load_alpha(sources["D"]))
        kept = accept_components(comps, width, height, expect)

        if len(kept) != expect:
            problems.append(
                f"{sheet}: مركّبات مقبولة {len(kept)} ≠ متوقّع {expect} "
                f"(كل المركّبات: {len(comps)})"
            )
            continue

        for name, comp in zip(names, kept):
            x0 = max(0, comp["x"] - PAD)
            y0 = max(0, comp["y"] - PAD)
            x1 = min(width, comp["x"] + comp["w"] + PAD)
            y1 = min(height, comp["y"] + comp["h"] + PAD)

            manifest[name] = {
                "sheet": sheet,
                "crop": [x0, y0, x1, y1],
                "size": [x1 - x0, y1 - y0],
                "area": comp["area"],
            }

            if args.dry_run:
                continue

            # قناع المركّب داخل نافذة القصّ: ما لا ينتمي إليه يُطمس. بلا هذا
            # كانت نافذة عقدة القمح تسحب معها سطر «حقل قمح» المطبوع تحتها
            # (على بعد 2 بكسل) وطرف العقدة المجاورة.
            window = labels[y0:y1, x0:x1] == comp["label"]
            mask = Image.fromarray((window * 255).astype(np.uint8), "L")

            for m in MAPS:
                crop = images[m].crop((x0, y0, x1, y1))
                pixels = np.array(crop)
                # نضرب الشفافية بالقناع بدل استبدالها: الحواف المصفّاة تحفظ
                # تدرّجها فلا تصير حدوداً مسنّنة.
                pixels[:, :, 3] = (pixels[:, :, 3] * (np.array(mask) / 255.0)).astype(np.uint8)
                out_path = os.path.join(OUT_DIR, f"T_feat_{name}_{m}.png")
                Image.fromarray(pixels, "RGBA").save(out_path, "PNG")
                written += 1

        print(f"[OK] {sheet} → {', '.join(names)}")

    if problems:
        print("\n[FAIL] مشاكل في التقطيع:")
        for p in problems:
            print(f"  - {p}")
        return 1

    if not args.dry_run:
        manifest_path = os.path.join(OUT_DIR, "sprites.json")
        with open(manifest_path, "w", encoding="utf-8") as fh:
            json.dump(
                {
                    "alphaThreshold": ALPHA_THRESHOLD,
                    "minSpriteArea": MIN_SPRITE_AREA,
                    "padding": PAD,
                    "sprites": manifest,
                },
                fh,
                ensure_ascii=False,
                indent=2,
            )
            fh.write("\n")
        print(f"\n[OK] {written} ملفاً في {OUT_DIR}")
        print(f"[OK] بيان الإحداثيات: {manifest_path}")
    else:
        print(f"\n[dry-run] {len(manifest)} sprite سيُكتب ×3 خرائط")

    return 0


if __name__ == "__main__":
    sys.exit(main())
