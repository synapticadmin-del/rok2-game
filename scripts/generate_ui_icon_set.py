#!/usr/bin/env python3
"""ROK2 — توليد أيقونات الواجهة المتبقية كصور PNG حقيقية.

`URok2IconLibrary` يسجّل 62 معرّفاً، لكن 20 فقط كانت تملك صورة مستوردة في
`Content/Art/UIIcons`؛ الباقي كان يسقط إلى الراسم الإجرائي الذي يرسم على شبكة
32×32 بالبكسل ثم يُعرض بحجم 14–24px — دوائر وخطوط لا أيقونات. هذا السكربت
يرسم الخمسين المتبقية بنفس أسلوب المجموعة الأصلية (Pillow، هندسة بدائية،
حدود داكنة، تكبير NEAREST) فتبقى المجموعة متسقة بصرياً.

المعرّفات مأخوذة من `KnownIds` في `Rok2IconLibrary.cpp` — لا تُخترع هنا.
التشغيل قابل للتكرار: نفس المدخلات تنتج نفس الملفات.

    python scripts/generate_ui_icon_set.py
"""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ICONS = ROOT / "game/client-unreal/Content/Art/UIIcons"
ICON_SIZE = 64
SCALE = 8
TRANSPARENT = (0, 0, 0, 0)

# نفس لوحة `generate_city_map_ui_assets.py` — المجموعتان تظهران جنباً إلى جنب
# في نفس الشريط، فاختلاف اللوحة يُقرأ كخلل لا كتنويع.
OUTLINE = (20, 24, 37, 255)
IVORY = (246, 237, 210, 255)
GOLD = (236, 187, 57, 255)
GOLD_HI = (255, 224, 117, 255)
WOOD = (137, 82, 43, 255)
WOOD_HI = (201, 132, 65, 255)
STONE = (141, 155, 170, 255)
STONE_HI = (204, 215, 226, 255)
BLUE = (54, 151, 214, 255)
BLUE_HI = (120, 210, 255, 255)
GREEN = (70, 178, 104, 255)
GREEN_HI = (141, 235, 158, 255)
RED = (190, 68, 62, 255)
RED_HI = (240, 125, 110, 255)
PURPLE = (139, 92, 218, 255)
FLESH = (222, 173, 133, 255)
DARK = (54, 42, 43, 255)


def px(draw, points, fill, width=1):
    """خط بحدّ داكن — يعطي الشكل قراءةً عند التصغير."""
    draw.line(points, fill=OUTLINE, width=width + 2, joint="curve")
    draw.line(points, fill=fill, width=width, joint="curve")


def polygon(draw, points, fill, shrink=0.85):
    draw.polygon(points, fill=OUTLINE)
    cx = sum(x for x, _ in points) / len(points)
    cy = sum(y for _, y in points) / len(points)
    draw.polygon(
        [(round(cx + (x - cx) * shrink), round(cy + (y - cy) * shrink)) for x, y in points],
        fill=fill,
    )


def disc(draw, cx, cy, r, fill):
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=OUTLINE)
    draw.ellipse((cx - r + 2, cy - r + 2, cx + r - 2, cy + r - 2), fill=fill)


def plate(draw, box, radius, fill):
    draw.rounded_rectangle(box, radius=radius, fill=OUTLINE)
    x0, y0, x1, y1 = box
    draw.rounded_rectangle((x0 + 3, y0 + 3, x1 - 3, y1 - 3), radius=max(1, radius - 2), fill=fill)


def star(draw, cx, cy, r, fill, points=5):
    pts = []
    for i in range(points * 2):
        ang = -math.pi / 2 + i * math.pi / points
        rad = r if i % 2 == 0 else r * 0.44
        pts.append((round(cx + rad * math.cos(ang)), round(cy + rad * math.sin(ang))))
    polygon(draw, pts, fill, shrink=0.9)


# ---------------------------------------------------------------------------
# الرسّامون — واحد لكل معرّف في KnownIds
# ---------------------------------------------------------------------------

def ap(draw):
    polygon(draw, [(34, 8), (17, 34), (30, 34), (26, 55), (49, 27), (35, 27)], PURPLE)
    px(draw, [(30, 16), (24, 30)], (196, 160, 255, 255), 2)


def sword(draw):
    polygon(draw, [(31, 8), (37, 14), (37, 40), (31, 46), (25, 40), (25, 14)], STONE_HI)
    draw.rectangle((16, 40, 47, 46), fill=OUTLINE)
    draw.rectangle((18, 41, 45, 44), fill=GOLD)
    draw.rectangle((28, 45, 35, 56), fill=OUTLINE)
    draw.rectangle((30, 46, 33, 55), fill=WOOD)
    disc(draw, 31, 55, 5, GOLD_HI)


def shield(draw):
    polygon(draw, [(32, 8), (52, 17), (48, 42), (32, 56), (16, 42), (12, 17)], BLUE)
    polygon(draw, [(32, 17), (43, 23), (40, 40), (32, 48), (24, 40), (21, 23)], BLUE_HI)
    px(draw, [(32, 18), (32, 47)], GOLD, 2)
    px(draw, [(22, 30), (42, 30)], GOLD, 2)


def helmet(draw):
    polygon(draw, [(14, 40), (14, 26), (24, 13), (40, 13), (50, 26), (50, 40)], STONE)
    draw.rectangle((13, 39, 51, 47), fill=OUTLINE)
    draw.rectangle((16, 41, 48, 45), fill=STONE_HI)
    draw.rectangle((29, 15, 35, 40), fill=OUTLINE)
    draw.rectangle((30, 17, 34, 39), fill=GOLD)
    for x in (20, 40):
        draw.rectangle((x, 27, x + 5, 36), fill=OUTLINE)


def banner(draw):
    px(draw, [(16, 8), (16, 57)], WOOD, 3)
    polygon(draw, [(19, 12), (52, 12), (52, 38), (36, 31), (19, 38)], RED)
    star(draw, 35, 22, 8, GOLD_HI)


def edit(draw):
    polygon(draw, [(12, 52), (18, 36), (42, 12), (52, 22), (28, 46)], WOOD_HI)
    polygon(draw, [(40, 14), (50, 24), (44, 30), (34, 20)], STONE_HI)
    polygon(draw, [(12, 52), (20, 49), (15, 44)], IVORY)


def lock(draw):
    draw.arc((21, 10, 43, 34), 180, 360, fill=OUTLINE, width=9)
    draw.arc((24, 13, 40, 32), 180, 360, fill=STONE_HI, width=4)
    plate(draw, (15, 28, 49, 55), 6, GOLD)
    draw.rectangle((29, 36, 35, 48), fill=OUTLINE)


def calendar(draw):
    plate(draw, (11, 16, 53, 55), 5, IVORY)
    draw.rectangle((12, 17, 52, 27), fill=RED)
    for x in (20, 42):
        draw.rectangle((x, 8, x + 5, 20), fill=OUTLINE)
        draw.rectangle((x + 1, 9, x + 4, 19), fill=STONE_HI)
    for row in (33, 42):
        for col in (18, 29, 40):
            draw.rectangle((col, row, col + 6, row + 5), fill=WOOD)


def hourglass(draw):
    draw.rectangle((15, 8, 49, 14), fill=OUTLINE)
    draw.rectangle((15, 50, 49, 56), fill=OUTLINE)
    draw.rectangle((17, 9, 47, 12), fill=WOOD_HI)
    draw.rectangle((17, 51, 47, 54), fill=WOOD_HI)
    polygon(draw, [(19, 15), (45, 15), (34, 32), (45, 49), (19, 49), (30, 32)], IVORY, shrink=0.94)
    polygon(draw, [(23, 18), (41, 18), (32, 30)], GOLD)
    polygon(draw, [(28, 44), (36, 44), (39, 47), (25, 47)], GOLD)


def flask(draw):
    draw.rectangle((26, 8, 38, 18), fill=OUTLINE)
    draw.rectangle((28, 9, 36, 17), fill=STONE_HI)
    polygon(draw, [(27, 17), (37, 17), (51, 50), (13, 50)], IVORY, shrink=0.93)
    polygon(draw, [(21, 36), (43, 36), (49, 48), (15, 48)], PURPLE, shrink=0.95)
    disc(draw, 27, 42, 3, BLUE_HI)


def cross(draw):
    plate(draw, (14, 14, 50, 50), 8, IVORY)
    draw.rectangle((28, 19, 36, 45), fill=RED)
    draw.rectangle((19, 28, 45, 36), fill=RED)


def scout(draw):
    disc(draw, 27, 26, 15, BLUE_HI)
    draw.ellipse((18, 17, 36, 35), outline=IVORY, width=2)
    px(draw, [(38, 37), (54, 53)], STONE, 5)
    px(draw, [(20, 22), (26, 18)], IVORY, 2)


def close(draw):
    px(draw, [(17, 17), (47, 47)], IVORY, 6)
    px(draw, [(47, 17), (17, 47)], IVORY, 6)


def star_icon(draw):
    star(draw, 32, 32, 24, GOLD)
    star(draw, 32, 30, 11, GOLD_HI)


def skull(draw):
    polygon(draw, [(16, 30), (18, 16), (32, 9), (46, 16), (48, 30), (42, 41), (22, 41)], IVORY)
    for x in (23, 35):
        disc(draw, x + 3, 27, 6, OUTLINE)
    polygon(draw, [(29, 33), (35, 33), (32, 40)], OUTLINE, shrink=1.0)
    draw.rectangle((23, 41, 41, 52), fill=OUTLINE)
    for x in (25, 31, 37):
        draw.rectangle((x, 43, x + 4, 50), fill=IVORY)


def blood(draw):
    polygon(draw, [(32, 8), (48, 34), (44, 50), (32, 56), (20, 50), (16, 34)], RED)
    polygon(draw, [(32, 20), (40, 36), (32, 46), (24, 36)], RED_HI)


def bandage(draw):
    draw.rounded_rectangle((8, 26, 56, 40), radius=7, fill=OUTLINE)
    draw.rounded_rectangle((11, 28, 53, 38), radius=5, fill=IVORY)
    plate(draw, (23, 22, 41, 44), 4, (232, 214, 178, 255))
    for dx, dy in ((-4, -4), (4, -4), (-4, 4), (4, 4)):
        draw.rectangle((32 + dx - 1, 33 + dy - 1, 32 + dx + 1, 33 + dy + 1), fill=WOOD)


def trophy(draw):
    polygon(draw, [(20, 10), (44, 10), (42, 30), (32, 38), (22, 30)], GOLD)
    draw.arc((6, 12, 24, 32), 90, 270, fill=OUTLINE, width=6)
    draw.arc((40, 12, 58, 32), 270, 90, fill=OUTLINE, width=6)
    draw.rectangle((28, 37, 36, 46), fill=OUTLINE)
    draw.rectangle((17, 46, 47, 54), fill=OUTLINE)
    draw.rectangle((20, 48, 44, 52), fill=WOOD_HI)
    star(draw, 32, 21, 7, GOLD_HI)


def handshake(draw):
    draw.rectangle((8, 28, 30, 38), fill=OUTLINE)
    draw.rectangle((10, 30, 30, 36), fill=FLESH)
    draw.rectangle((34, 28, 56, 38), fill=OUTLINE)
    draw.rectangle((34, 30, 54, 36), fill=(198, 148, 108, 255))
    plate(draw, (24, 24, 42, 43), 4, GOLD_HI)
    px(draw, [(32, 27), (32, 40)], WOOD, 1)


def refresh(draw):
    draw.arc((13, 13, 51, 51), 40, 320, fill=OUTLINE, width=11)
    draw.arc((16, 16, 48, 48), 40, 320, fill=BLUE_HI, width=5)
    polygon(draw, [(44, 6), (56, 18), (38, 22)], BLUE)


def gift(draw):
    plate(draw, (12, 26, 52, 55), 4, RED)
    draw.rectangle((10, 20, 54, 30), fill=OUTLINE)
    draw.rectangle((12, 22, 52, 28), fill=RED_HI)
    draw.rectangle((28, 21, 36, 55), fill=GOLD)
    disc(draw, 24, 16, 7, GOLD_HI)
    disc(draw, 40, 16, 7, GOLD_HI)


def wheat(draw):
    px(draw, [(32, 56), (32, 20)], WOOD_HI, 3)
    for side in (-1, 1):
        for i, y in enumerate((24, 32, 40)):
            polygon(
                draw,
                [(32, y), (32 + side * 14, y - 3), (32 + side * 11, y + 7), (32, y + 6)],
                GOLD if i % 2 == 0 else GOLD_HI,
            )
    polygon(draw, [(32, 14), (37, 22), (32, 26), (27, 22)], GOLD_HI)


def box(draw):
    polygon(draw, [(10, 22), (32, 12), (54, 22), (32, 32)], WOOD_HI, shrink=0.97)
    polygon(draw, [(10, 22), (32, 32), (32, 55), (10, 45)], WOOD, shrink=0.97)
    polygon(draw, [(54, 22), (54, 45), (32, 55), (32, 32)], (109, 66, 34, 255), shrink=0.97)
    px(draw, [(32, 32), (32, 55)], GOLD, 2)


def cart(draw):
    plate(draw, (13, 22, 47, 40), 3, WOOD_HI)
    for x in (17, 26, 35):
        draw.rectangle((x, 24, x + 6, 38), fill=WOOD)
    px(draw, [(46, 30), (56, 22)], STONE, 3)
    disc(draw, 20, 47, 8, DARK)
    disc(draw, 40, 47, 8, DARK)
    disc(draw, 20, 47, 3, STONE_HI)
    disc(draw, 40, 47, 3, STONE_HI)


def horse(draw):
    polygon(draw, [(16, 30), (28, 22), (44, 22), (50, 30), (46, 42), (20, 42)], WOOD_HI)
    polygon(draw, [(40, 24), (54, 10), (58, 18), (48, 28)], WOOD)
    polygon(draw, [(50, 6), (56, 12), (52, 14)], WOOD_HI)
    for x in (22, 40):
        draw.rectangle((x, 41, x + 6, 56), fill=OUTLINE)
        draw.rectangle((x + 1, 42, x + 5, 55), fill=WOOD)
    px(draw, [(16, 28), (8, 40)], (109, 66, 34, 255), 3)


def bow(draw):
    draw.arc((16, 6, 52, 58), 100, 260, fill=OUTLINE, width=8)
    draw.arc((19, 9, 49, 55), 100, 260, fill=WOOD_HI, width=3)
    px(draw, [(22, 12), (22, 52)], IVORY, 2)
    px(draw, [(22, 32), (52, 32)], STONE_HI, 2)
    polygon(draw, [(52, 32), (44, 27), (44, 37)], STONE)


def tent(draw):
    polygon(draw, [(32, 8), (54, 52), (10, 52)], IVORY, shrink=0.95)
    polygon(draw, [(32, 22), (42, 52), (22, 52)], WOOD)
    px(draw, [(32, 8), (32, 4)], GOLD, 2)
    px(draw, [(14, 50), (50, 50)], WOOD_HI, 2)


def tower(draw):
    draw.rectangle((20, 20, 44, 56), fill=OUTLINE)
    draw.rectangle((23, 22, 41, 55), fill=STONE)
    draw.rectangle((16, 14, 48, 22), fill=OUTLINE)
    for x in (18, 26, 34, 42):
        draw.rectangle((x, 8, x + 5, 16), fill=OUTLINE)
        draw.rectangle((x + 1, 9, x + 4, 15), fill=STONE_HI)
    draw.rectangle((28, 30, 36, 42), fill=OUTLINE)
    draw.rectangle((29, 32, 35, 41), fill=DARK)


def castle(draw):
    for x in (10, 26, 42):
        draw.rectangle((x, 22, x + 12, 56), fill=OUTLINE)
        draw.rectangle((x + 2, 24, x + 10, 55), fill=STONE)
        for cx in (x + 1, x + 7):
            draw.rectangle((cx, 17, cx + 4, 24), fill=OUTLINE)
    polygon(draw, [(20, 30), (32, 14), (44, 30)], RED)
    draw.rectangle((27, 40, 37, 56), fill=OUTLINE)
    draw.rectangle((29, 43, 35, 56), fill=WOOD)
    star(draw, 32, 10, 5, GOLD_HI)


def bricks(draw):
    rows = ((16, 0), (26, 1), (36, 0), (46, 1))
    for y, off in rows:
        for i in range(-1, 4):
            x = 10 + i * 15 + off * 7
            draw.rectangle((x, y, x + 13, y + 8), fill=OUTLINE)
            draw.rectangle((x + 1, y + 1, x + 12, y + 7), fill=STONE if (i + off) % 2 else STONE_HI)


def rock(draw):
    polygon(draw, [(10, 46), (16, 26), (30, 16), (48, 24), (54, 44), (42, 54), (20, 54)], STONE)
    polygon(draw, [(22, 36), (32, 22), (42, 34), (34, 44)], STONE_HI)
    px(draw, [(16, 44), (26, 48)], (108, 120, 134, 255), 2)


def beer(draw):
    plate(draw, (16, 20, 42, 56), 3, GOLD)
    draw.rectangle((17, 20, 41, 30), fill=IVORY)
    draw.arc((38, 26, 56, 46), 270, 90, fill=OUTLINE, width=7)
    draw.arc((41, 29, 53, 43), 270, 90, fill=STONE_HI, width=3)
    for cx, cy in ((22, 16), (30, 13), (37, 17)):
        disc(draw, cx, cy, 5, IVORY)


def scale(draw):
    px(draw, [(32, 12), (32, 52)], WOOD, 3)
    px(draw, [(12, 20), (52, 20)], WOOD_HI, 3)
    draw.rectangle((22, 51, 42, 57), fill=OUTLINE)
    draw.rectangle((24, 52, 40, 55), fill=WOOD_HI)
    for cx in (14, 50):
        px(draw, [(cx, 20), (cx, 30)], STONE, 1)
        polygon(draw, [(cx - 10, 30), (cx + 10, 30), (cx + 6, 40), (cx - 6, 40)], GOLD)
    disc(draw, 32, 12, 5, GOLD_HI)


def crown(draw):
    polygon(draw, [(10, 46), (14, 16), (24, 32), (32, 12), (40, 32), (50, 16), (54, 46)], GOLD, shrink=0.94)
    draw.rectangle((9, 45, 55, 55), fill=OUTLINE)
    draw.rectangle((12, 47, 52, 53), fill=GOLD_HI)
    for cx, col in ((20, RED), (32, BLUE_HI), (44, GREEN_HI)):
        disc(draw, cx, 39, 4, col)


def builder(draw):
    polygon(draw, [(12, 30), (18, 16), (32, 10), (46, 16), (52, 30)], GOLD)
    draw.rectangle((9, 29, 55, 37), fill=OUTLINE)
    draw.rectangle((12, 31, 52, 35), fill=GOLD_HI)
    draw.rectangle((22, 38, 42, 56), fill=OUTLINE)
    draw.rectangle((24, 40, 40, 55), fill=BLUE)
    px(draw, [(14, 44), (22, 40)], WOOD_HI, 3)


def conn(draw):
    disc(draw, 32, 32, 14, GREEN)
    disc(draw, 32, 32, 6, GREEN_HI)
    draw.arc((10, 10, 54, 54), 0, 360, fill=IVORY, width=3)


def governor(draw):
    disc(draw, 32, 24, 13, FLESH)
    polygon(draw, [(17, 16), (22, 6), (32, 11), (42, 6), (47, 16)], GOLD)
    polygon(draw, [(10, 56), (14, 42), (24, 36), (40, 36), (50, 42), (54, 56)], BLUE)
    px(draw, [(32, 38), (32, 56)], GOLD_HI, 2)


def stats(draw):
    px(draw, [(12, 54), (54, 54)], IVORY, 2)
    px(draw, [(12, 54), (12, 12)], IVORY, 2)
    for x, h, col in ((18, 18, BLUE), (28, 30, GREEN), (38, 24, GOLD), (46, 38, PURPLE)):
        draw.rectangle((x, 52 - h, x + 8, 52), fill=OUTLINE)
        draw.rectangle((x + 1, 53 - h, x + 7, 51), fill=col)


def move(draw):
    px(draw, [(32, 12), (32, 52)], IVORY, 4)
    px(draw, [(12, 32), (52, 32)], IVORY, 4)
    for pts in (
        [(32, 4), (40, 16), (24, 16)],
        [(32, 60), (24, 48), (40, 48)],
        [(4, 32), (16, 24), (16, 40)],
        [(60, 32), (48, 40), (48, 24)],
    ):
        polygon(draw, pts, GOLD_HI, shrink=1.0)


def sparkle(draw):
    star(draw, 30, 28, 20, GOLD, points=4)
    star(draw, 30, 28, 9, GOLD_HI, points=4)
    star(draw, 50, 48, 9, IVORY, points=4)
    star(draw, 14, 50, 6, IVORY, points=4)


def combat(draw):
    px(draw, [(12, 52), (46, 14)], STONE_HI, 5)
    px(draw, [(52, 52), (18, 14)], STONE_HI, 5)
    px(draw, [(10, 20), (24, 8)], GOLD, 3)
    px(draw, [(54, 20), (40, 8)], GOLD, 3)
    star(draw, 32, 32, 9, RED_HI, points=4)


def ring(draw):
    draw.ellipse((18, 26, 46, 54), fill=OUTLINE)
    draw.ellipse((21, 29, 43, 51), fill=GOLD)
    draw.ellipse((27, 35, 37, 45), fill=TRANSPARENT)
    draw.ellipse((26, 34, 38, 46), fill=(40, 30, 12, 255))
    draw.ellipse((28, 36, 36, 44), fill=TRANSPARENT)
    polygon(draw, [(32, 6), (44, 20), (32, 30), (20, 20)], BLUE_HI)


def boots(draw):
    polygon(draw, [(18, 12), (34, 12), (34, 40), (52, 44), (52, 54), (18, 54)], WOOD_HI)
    draw.rectangle((15, 51, 55, 58), fill=OUTLINE)
    draw.rectangle((17, 52, 53, 56), fill=DARK)
    for y in (20, 28, 36):
        px(draw, [(19, y), (33, y)], GOLD, 1)


def arrow(draw):
    px(draw, [(10, 54), (48, 16)], WOOD_HI, 3)
    polygon(draw, [(56, 8), (56, 26), (38, 26)], STONE_HI, shrink=0.9)
    for dx in (0, 7):
        px(draw, [(10 + dx, 54 - dx), (4 + dx, 44 - dx)], IVORY, 2)


def skillup(draw):
    disc(draw, 32, 34, 22, PURPLE)
    polygon(draw, [(32, 16), (46, 34), (38, 34), (38, 50), (26, 50), (26, 34), (18, 34)], GOLD_HI)


def pickaxe(draw):
    px(draw, [(16, 54), (46, 18)], WOOD, 4)
    draw.arc((10, 6, 58, 34), 190, 350, fill=OUTLINE, width=9)
    draw.arc((13, 9, 55, 31), 190, 350, fill=STONE_HI, width=4)
    disc(draw, 34, 20, 4, GOLD)


def clock(draw):
    disc(draw, 32, 34, 24, IVORY)
    draw.ellipse((13, 15, 51, 53), outline=GOLD, width=3)
    px(draw, [(32, 34), (32, 20)], OUTLINE, 3)
    px(draw, [(32, 34), (43, 40)], RED, 3)
    disc(draw, 32, 34, 3, GOLD)
    draw.rectangle((28, 6, 36, 12), fill=OUTLINE)


def art(draw):
    polygon(draw, [(10, 34), (18, 16), (40, 12), (54, 24), (48, 44), (26, 52)], IVORY, shrink=0.95)
    for cx, cy, col in ((22, 26, RED), (33, 21, BLUE), (43, 27, GREEN), (38, 38, GOLD)):
        disc(draw, cx, cy, 5, col)
    draw.ellipse((16, 38, 28, 50), fill=OUTLINE)
    draw.ellipse((18, 40, 26, 48), fill=TRANSPARENT)


def monument(draw):
    polygon(draw, [(26, 8), (38, 8), (42, 44), (22, 44)], STONE_HI, shrink=0.96)
    draw.rectangle((16, 43, 48, 51), fill=OUTLINE)
    draw.rectangle((19, 45, 45, 49), fill=STONE)
    draw.rectangle((11, 50, 53, 58), fill=OUTLINE)
    draw.rectangle((14, 52, 50, 56), fill=STONE_HI)
    for y in (18, 26, 34):
        px(draw, [(27, y), (37, y)], GOLD, 1)


def wrench(draw):
    px(draw, [(18, 50), (44, 22)], STONE, 7)
    polygon(draw, [(40, 10), (54, 16), (52, 28), (38, 24)], STONE_HI)
    draw.rectangle((43, 15, 50, 22), fill=OUTLINE)
    disc(draw, 18, 50, 7, GOLD)


PAINTERS = {
    "ap": ap,
    "sword": sword,
    "shield": shield,
    "helmet": helmet,
    "banner": banner,
    "edit": edit,
    "lock": lock,
    "calendar": calendar,
    "hourglass": hourglass,
    "flask": flask,
    "cross": cross,
    "scout": scout,
    "close": close,
    "star": star_icon,
    "skull": skull,
    "blood": blood,
    "bandage": bandage,
    "trophy": trophy,
    "handshake": handshake,
    "refresh": refresh,
    "gift": gift,
    "wheat": wheat,
    "box": box,
    "cart": cart,
    "horse": horse,
    "bow": bow,
    "tent": tent,
    "tower": tower,
    "castle": castle,
    "bricks": bricks,
    "rock": rock,
    "beer": beer,
    "scale": scale,
    "crown": crown,
    "builder": builder,
    "conn": conn,
    "governor": governor,
    "stats": stats,
    "move": move,
    "sparkle": sparkle,
    "combat": combat,
    "ring": ring,
    "boots": boots,
    "arrow": arrow,
    "skillup": skillup,
    "pickaxe": pickaxe,
    "clock": clock,
    "art": art,
    "monument": monument,
    "wrench": wrench,
}


def main() -> None:
    ICONS.mkdir(parents=True, exist_ok=True)
    for name, painter in PAINTERS.items():
        img = Image.new("RGBA", (ICON_SIZE, ICON_SIZE), TRANSPARENT)
        painter(ImageDraw.Draw(img))
        out = ICONS / f"icon_{name}.png"
        img.resize((ICON_SIZE * SCALE, ICON_SIZE * SCALE), Image.Resampling.NEAREST).save(out)
    print(f"Created {len(PAINTERS)} icons in {ICONS}")


if __name__ == "__main__":
    main()
