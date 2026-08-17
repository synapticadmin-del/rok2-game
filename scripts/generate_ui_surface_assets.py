#!/usr/bin/env python3
"""ROK2 — توليد أسطح الواجهة: نسيج ورق/جلد وإطارات مزخرفة 9-slice.

المشكلة التي يحلّها: `Rok2Surface` كان يبني كل سطح في اللعبة بـ
`FSlateRoundedBoxBrush` بلون واحد مسطح — لوحات وبطاقات وحبات ودوائر وأشرطة
تقدم. مستطيلات ملوّنة بلا نسيج ولا إطار ولا ظل، وهو أصل الإحساس بأن الشاشات
«صفحات جرداء».

المخرجات (كلها 9-slice قابلة للتمدد بهامش 0.25 كما يتوقع
`ESlateBrushDrawType::Box`):

  panel_parchment   لوحات كبيرة — رِقّ دافئ بحواف داكنة
  panel_leather     أوراق سفلية (Bottom Sheets) — جلد مدبوغ
  card_stone        بطاقات — حجر فاتح
  frame_ornate      إطار ذهبي مزخرف بزوايا — يُركَّب فوق اللوحة
  bar_wood          الشريط العلوي — خشب أفقي بحد ذهبي
  divider_gold      فاصل أفقي رقيق
  pill_bronze       حبّات الأزرار — برونز مستدير

النسيج مولَّد إجرائياً بضجيج قيمي (value noise) قابل للتكرار بنفس البذرة، فلا
اعتماد على صور خارجية ولا ترخيص جديد.

    python scripts/generate_ui_surface_assets.py
"""
from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "game/client-unreal/Content/Art/UISurfaces"

# نفس لوحة بقية الأصول — الأسطح تحمل الأيقونات فوقها، فاختلاف الحرارة يُقرأ خللاً.
OUTLINE = (20, 24, 37, 255)
GOLD = (201, 162, 39, 255)
GOLD_HI = (255, 224, 117, 255)
GOLD_DK = (120, 92, 18, 255)
PARCHMENT = (58, 44, 26, 255)
PARCHMENT_HI = (86, 66, 38, 255)
LEATHER = (44, 31, 20, 255)
LEATHER_HI = (72, 51, 33, 255)
STONE = (54, 48, 38, 255)
STONE_HI = (78, 70, 56, 255)
WOOD = (38, 26, 16, 255)
WOOD_HI = (64, 44, 26, 255)
BRONZE = (96, 68, 30, 255)
BRONZE_HI = (146, 106, 48, 255)


def value_noise(size, cells, seed):
    """ضجيج قيمي ناعم: شبكة عشوائية + تمويه. قابل للتكرار بنفس البذرة."""
    rng = random.Random(seed)
    w, h = size
    small = Image.new("L", (cells, cells))
    small.putdata([rng.randrange(256) for _ in range(cells * cells)])
    return small.resize((w, h), Image.Resampling.BICUBIC).filter(ImageFilter.GaussianBlur(1.2))


def textured_plate(size, base, highlight, cells, seed, strength=0.30):
    """قاعدة ملوّنة + نسيج ضجيج — الوحدة الأساسية لكل الأسطح."""
    w, h = size
    noise = value_noise(size, cells, seed)
    img = Image.new("RGBA", size)
    npx = noise.load()
    out = img.load()
    for y in range(h):
        for x in range(w):
            t = (npx[x, y] / 255.0 - 0.5) * 2.0 * strength
            out[x, y] = (
                max(0, min(255, int(base[0] + (highlight[0] - base[0]) * 0.5 + t * 90))),
                max(0, min(255, int(base[1] + (highlight[1] - base[1]) * 0.5 + t * 80))),
                max(0, min(255, int(base[2] + (highlight[2] - base[2]) * 0.5 + t * 62))),
                255,
            )
    return img


def vignette(img, depth=52):
    """تعتيم الحواف — يفصل اللوح عن العالم خلفه بلا إطار إضافي."""
    w, h = img.size
    px = img.load()
    edge = max(6, min(w, h) // 8)
    for y in range(h):
        for x in range(w):
            d = min(x, y, w - 1 - x, h - 1 - y)
            if d >= edge:
                continue
            k = 1.0 - (d / edge)
            r, g, b, a = px[x, y]
            px[x, y] = (
                max(0, int(r - depth * k)),
                max(0, int(g - depth * k)),
                max(0, int(b - depth * k)),
                a,
            )
    return img


def overlay(img, paint):
    """يرسم على طبقة شفافة ثم يدمجها.

    `ImageDraw` يكتب البكسل كما هو ولا يمزجه، فلونٌ بشفافية 26 يثقب اللوح
    بدل أن يعتّمه — وهو ما كان يجعل ألياف الرِقّ تظهر خطوطاً بيضاء ساطعة.
    الدمج عبر `alpha_composite` هو المزج الصحيح.
    """
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    paint(ImageDraw.Draw(layer))
    return Image.alpha_composite(img, layer)


def rim(draw, size, radius, color, width):
    w, h = size
    draw.rounded_rectangle((0, 0, w - 1, h - 1), radius=radius, outline=OUTLINE, width=width + 2)
    draw.rounded_rectangle(
        (width, width, w - 1 - width, h - 1 - width), radius=max(1, radius - width), outline=color, width=width
    )


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    img.save(OUT / f"{name}.png")


def panel_parchment():
    size = (256, 256)
    img = vignette(textured_plate(size, PARCHMENT, PARCHMENT_HI, 22, 1101))

    # خطوط أفقية باهتة تحاكي ألياف الرِقّ — تُقرأ عند التمدد الرأسي بلا تكرار ظاهر.
    def fibers(d):
        for y in range(10, 246, 9):
            d.line((12, y, 243, y), fill=(0, 0, 0, 30), width=1)
            d.line((12, y + 1, 243, y + 1), fill=(255, 236, 190, 12), width=1)

    img = overlay(img, fibers)

    draw = ImageDraw.Draw(img)
    rim(draw, size, 22, GOLD, 3)
    for cx, cy in ((22, 22), (233, 22), (22, 233), (233, 233)):
        draw.ellipse((cx - 5, cy - 5, cx + 5, cy + 5), fill=OUTLINE)
        draw.ellipse((cx - 3, cy - 3, cx + 3, cy + 3), fill=GOLD_HI)
    return img


def panel_leather():
    size = (256, 256)
    img = vignette(textured_plate(size, LEATHER, LEATHER_HI, 34, 2203, strength=0.38), depth=64)

    # خياطة داخلية — علامة «ورقة سفلية» في وثيقة الهوية.
    def stitches(d):
        for x in range(24, 232, 12):
            d.line((x, 16, x + 6, 16), fill=(214, 190, 140, 150), width=2)
            d.line((x, 239, x + 6, 239), fill=(214, 190, 140, 150), width=2)
        for y in range(24, 232, 12):
            d.line((16, y, 16, y + 6), fill=(214, 190, 140, 150), width=2)
            d.line((239, y, 239, y + 6), fill=(214, 190, 140, 150), width=2)

    img = overlay(img, stitches)
    rim(ImageDraw.Draw(img), size, 26, GOLD_DK, 3)
    return img


def card_stone():
    size = (128, 128)
    img = vignette(textured_plate(size, STONE, STONE_HI, 16, 3307, strength=0.26), depth=34)
    # وميض أعلى: ضوء يسقط من فوق، فتُقرأ البطاقة كسطح لا كمربّع.
    img = overlay(img, lambda d: d.line((14, 10, 113, 10), fill=(255, 240, 200, 60), width=3))
    rim(ImageDraw.Draw(img), size, 14, (150, 122, 52, 255), 2)
    return img


def frame_ornate():
    """إطار شفاف الوسط — يُركَّب فوق اللوحة كطبقة ثانية."""
    size = (256, 256)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle((2, 2, 253, 253), radius=24, outline=OUTLINE, width=7)
    draw.rounded_rectangle((6, 6, 249, 249), radius=21, outline=GOLD, width=4)
    draw.rounded_rectangle((13, 13, 242, 242), radius=16, outline=GOLD_DK, width=2)
    draw.rounded_rectangle((18, 18, 237, 237), radius=13, outline=(255, 236, 176, 120), width=1)

    # زوايا مورّقة: قوسان متقابلان + حبّة — الزخرفة كلها في الهامش الثابت
    # للـ9-slice فلا تتمدد ولا تتشوّه مع حجم اللوحة.
    for ox, oy, sx, sy in ((0, 0, 1, 1), (255, 0, -1, 1), (0, 255, 1, -1), (255, 255, -1, -1)):
        def P(x, y):
            return (ox + sx * x, oy + sy * y)

        def Box(x0, y0, x1, y1):
            """صندوق مرتّب: الانعكاس بـsx/sy يقلب الحدود، وPillow يرفض x1<x0."""
            a, b = P(x0, y0)
            c, d = P(x1, y1)
            return (min(a, c), min(b, d), max(a, c), max(b, d))

        draw.line([P(10, 34), P(34, 10)], fill=OUTLINE, width=8)
        draw.line([P(11, 33), P(33, 11)], fill=GOLD_HI, width=4)
        draw.line([P(20, 46), P(46, 20)], fill=OUTLINE, width=5)
        draw.line([P(21, 45), P(45, 21)], fill=GOLD, width=2)
        draw.ellipse(Box(15, 15, 29, 29), fill=OUTLINE)
        draw.ellipse(Box(18, 18, 26, 26), fill=GOLD_HI)

    # وسط أضلاع الإطار: معيّن صغير على كل ضلع
    for cx, cy in ((128, 9), (128, 246), (9, 128), (246, 128)):
        pts = [(cx, cy - 8), (cx + 8, cy), (cx, cy + 8), (cx - 8, cy)]
        draw.polygon(pts, fill=OUTLINE)
        pts = [(cx, cy - 5), (cx + 5, cy), (cx, cy + 5), (cx - 5, cy)]
        draw.polygon(pts, fill=GOLD_HI)
    return img


def bar_wood():
    size = (256, 96)
    img = textured_plate(size, WOOD, WOOD_HI, 26, 4409, strength=0.34)

    # ألياف أفقية: الشريط يتمدد أفقياً، فالألياف الأفقية وحدها لا تُشوّه.
    def grain(d):
        for y in range(6, 92, 7):
            d.line((0, y, 255, y), fill=(0, 0, 0, 34), width=1)
        d.line((0, 3, 255, 3), fill=(255, 232, 170, 55), width=3)

    img = overlay(img, grain)
    draw = ImageDraw.Draw(img)
    draw.rectangle((0, 88, 255, 95), fill=OUTLINE)
    draw.rectangle((0, 90, 255, 93), fill=GOLD)
    return img


def divider_gold():
    size = (128, 12)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rectangle((0, 4, 127, 7), fill=GOLD_DK)
    draw.line((0, 5, 127, 5), fill=GOLD_HI, width=1)
    for cx in (16, 64, 112):
        pts = [(cx, 0), (cx + 6, 6), (cx, 11), (cx - 6, 6)]
        draw.polygon(pts, fill=OUTLINE)
        pts = [(cx, 2), (cx + 4, 6), (cx, 9), (cx - 4, 6)]
        draw.polygon(pts, fill=GOLD_HI)
    return img


def pill_bronze():
    size = (128, 64)
    img = textured_plate(size, BRONZE, BRONZE_HI, 12, 5511, strength=0.22)
    # الشكل نفسه حبّة كاملة الاستدارة: نقنّع بقناع مستدير فتبقى الحواف شفافة.
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, 127, 63), radius=31, fill=255)
    img.putalpha(mask)
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle((0, 0, 127, 63), radius=31, outline=OUTLINE, width=4)
    draw.rounded_rectangle((3, 3, 124, 60), radius=28, outline=GOLD, width=2)
    return overlay(img, lambda d: d.line((16, 8, 111, 8), fill=(255, 236, 180, 80), width=3))


BUILDERS = {
    "panel_parchment": panel_parchment,
    "panel_leather": panel_leather,
    "card_stone": card_stone,
    "frame_ornate": frame_ornate,
    "bar_wood": bar_wood,
    "divider_gold": divider_gold,
    "pill_bronze": pill_bronze,
}


def main() -> None:
    for name, build in BUILDERS.items():
        save(build(), name)
    print(f"Created {len(BUILDERS)} UI surface textures in {OUT}")


if __name__ == "__main__":
    main()
