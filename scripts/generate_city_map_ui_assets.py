#!/usr/bin/env python3
"""Create original pixel-art-inspired UI icon and button PNGs for ROK2.

This generator intentionally uses only primitive geometry and local Pillow APIs.  It
produces transparent Texture2D-ready PNG files and can be rerun deterministically.
"""
from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ICONS = ROOT / "game/client-unreal/Content/Art/UIIcons"
BUTTONS = ROOT / "game/client-unreal/Content/Art/UIButtons"
CITY_BUILDINGS = ROOT / "game/client-unreal/Content/Art/CityBuildingIcons"
ICON_SIZE = 64
SCALE = 8
TRANSPARENT = (0, 0, 0, 0)
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
PURPLE = (139, 92, 218, 255)


def px(draw: ImageDraw.ImageDraw, points, fill, width: int = 1) -> None:
    draw.line(points, fill=OUTLINE, width=width + 2, joint="curve")
    draw.line(points, fill=fill, width=width, joint="curve")


def icon_canvas() -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGBA", (ICON_SIZE, ICON_SIZE), TRANSPARENT)
    return img, ImageDraw.Draw(img)


def polygon(draw, points, fill, outline=OUTLINE):
    draw.polygon(points, fill=outline)
    inset = []
    cx = sum(x for x, _ in points) / len(points)
    cy = sum(y for _, y in points) / len(points)
    for x, y in points:
        inset.append((round(cx + (x - cx) * 0.85), round(cy + (y - cy) * 0.85)))
    draw.polygon(inset, fill=fill)


def food(draw):
    for x, top in ((25, 17), (31, 12), (37, 17)):
        px(draw, [(x, 47), (x, top)], GOLD, 2)
        for offset, y in ((-5, top + 7), (5, top + 12), (-4, top + 17), (4, top + 22)):
            draw.rectangle((x + min(0, offset), y, x + max(0, offset), y + 3), fill=OUTLINE)
            draw.rectangle((x + min(0, offset) + 1, y + 1, x + max(0, offset) - 1, y + 2), fill=GOLD_HI)
    draw.rectangle((17, 45, 45, 51), fill=OUTLINE)
    draw.rectangle((19, 46, 43, 49), fill=WOOD)


def wood(draw):
    polygon(draw, [(12, 34), (23, 21), (51, 27), (40, 43)], WOOD)
    draw.ellipse((35, 26, 48, 39), fill=OUTLINE)
    draw.ellipse((37, 28, 46, 37), fill=WOOD_HI)
    draw.ellipse((40, 31, 44, 35), fill=WOOD)
    px(draw, [(18, 43), (43, 17)], STONE_HI, 2)


def stone(draw):
    polygon(draw, [(12, 41), (18, 24), (33, 15), (50, 25), (52, 42), (40, 50), (21, 50)], STONE)
    polygon(draw, [(20, 31), (32, 19), (36, 33), (29, 41)], STONE_HI)


def gold(draw):
    for x, y in ((18, 32), (29, 22), (39, 32)):
        draw.ellipse((x - 9, y - 9, x + 9, y + 9), fill=OUTLINE)
        draw.ellipse((x - 6, y - 6, x + 6, y + 6), fill=GOLD)
        draw.rectangle((x - 1, y - 4, x + 1, y + 4), fill=GOLD_HI)


def gems(draw):
    polygon(draw, [(32, 10), (49, 29), (32, 52), (15, 29)], PURPLE)
    polygon(draw, [(32, 14), (42, 28), (32, 37), (22, 28)], BLUE_HI)
    px(draw, [(32, 14), (32, 46)], IVORY, 1)


def build(draw):
    px(draw, [(18, 46), (43, 21)], WOOD_HI, 5)
    polygon(draw, [(38, 12), (53, 18), (48, 27), (42, 25), (37, 31), (30, 24)], STONE_HI)
    draw.rectangle((13, 42, 25, 52), fill=OUTLINE)
    draw.rectangle((16, 45, 23, 50), fill=WOOD)


def train(draw):
    polygon(draw, [(13, 34), (18, 20), (31, 13), (46, 19), (52, 33), (47, 42), (18, 42)], STONE)
    draw.rectangle((15, 34, 49, 43), fill=OUTLINE)
    draw.rectangle((18, 35, 46, 40), fill=BLUE)
    draw.rectangle((29, 27, 35, 38), fill=GOLD)


def map_icon(draw):
    polygon(draw, [(12, 18), (27, 13), (39, 18), (52, 13), (52, 47), (39, 52), (27, 47), (12, 52)], IVORY)
    px(draw, [(27, 15), (27, 46)], WOOD, 1)
    px(draw, [(39, 19), (39, 50)], WOOD, 1)
    px(draw, [(17, 38), (24, 30), (31, 35), (42, 25), (48, 31)], RED, 1)


def reports(draw):
    draw.rounded_rectangle((16, 10, 48, 53), radius=4, fill=OUTLINE)
    draw.rounded_rectangle((19, 12, 45, 50), radius=3, fill=IVORY)
    for y in (21, 29, 37):
        draw.rectangle((24, y, 40, y + 3), fill=WOOD)
    draw.ellipse((19, 41, 33, 55), fill=OUTLINE)
    draw.ellipse((21, 42, 31, 52), fill=RED)


def alliance(draw):
    polygon(draw, [(32, 10), (50, 18), (47, 41), (32, 53), (17, 41), (14, 18)], BLUE)
    polygon(draw, [(32, 20), (42, 27), (38, 40), (32, 44), (26, 40), (22, 27)], GOLD)
    draw.rectangle((29, 22, 35, 41), fill=GOLD_HI)


def research(draw):
    draw.ellipse((22, 9, 42, 29), fill=OUTLINE)
    draw.ellipse((25, 12, 39, 26), fill=BLUE_HI)
    draw.rectangle((27, 25, 37, 38), fill=OUTLINE)
    polygon(draw, [(21, 35), (43, 35), (52, 51), (12, 51)], PURPLE)
    draw.rectangle((27, 39, 37, 43), fill=BLUE_HI)


def heal(draw):
    draw.rounded_rectangle((16, 16, 48, 48), radius=7, fill=OUTLINE)
    draw.rounded_rectangle((19, 19, 45, 45), radius=5, fill=GREEN)
    draw.rectangle((28, 22, 36, 42), fill=IVORY)
    draw.rectangle((22, 28, 42, 36), fill=IVORY)


def speed(draw):
    polygon(draw, [(33, 8), (18, 33), (29, 33), (25, 54), (48, 26), (37, 26)], GOLD)


def settings(draw):
    for angle in range(0, 360, 45):
        import math
        x = round(32 + math.cos(math.radians(angle)) * 18)
        y = round(32 + math.sin(math.radians(angle)) * 18)
        draw.rectangle((x - 4, y - 4, x + 4, y + 4), fill=OUTLINE)
        draw.rectangle((x - 2, y - 2, x + 2, y + 2), fill=STONE)
    draw.ellipse((18, 18, 46, 46), fill=OUTLINE)
    draw.ellipse((21, 21, 43, 43), fill=STONE)
    draw.ellipse((27, 27, 37, 37), fill=OUTLINE)
    draw.ellipse((29, 29, 35, 35), fill=GOLD)


def bell(draw):
    polygon(draw, [(19, 39), (23, 22), (29, 16), (35, 16), (42, 22), (46, 39)], GOLD)
    draw.rectangle((16, 38, 49, 44), fill=OUTLINE)
    draw.rectangle((19, 39, 46, 42), fill=GOLD)
    draw.ellipse((28, 43, 36, 51), fill=OUTLINE)
    draw.ellipse((30, 45, 34, 49), fill=GOLD_HI)


def bag(draw):
    draw.rounded_rectangle((16, 22, 48, 50), radius=6, fill=OUTLINE)
    draw.rounded_rectangle((19, 25, 45, 47), radius=4, fill=WOOD)
    draw.arc((23, 11, 41, 31), 180, 360, fill=OUTLINE, width=4)
    draw.arc((25, 13, 39, 30), 180, 360, fill=GOLD_HI, width=2)
    draw.rectangle((28, 32, 36, 37), fill=GOLD)


def save_icon(name: str, painter) -> None:
    img, draw = icon_canvas()
    painter(draw)
    img.resize((ICON_SIZE * SCALE, ICON_SIZE * SCALE), Image.Resampling.NEAREST).save(ICONS / f"icon_{name}.png")


def city_building(kind: str):
    """Return a stylized, text-free building portrait for city queues and cards."""
    def paint(draw):
        # Common stone footing and warm ground shadow make the set legible at small sizes.
        draw.ellipse((10, 44, 54, 57), fill=OUTLINE)
        draw.ellipse((13, 46, 51, 54), fill=(49, 71, 60, 255))
        draw.rectangle((16, 37, 48, 49), fill=OUTLINE)
        draw.rectangle((19, 38, 45, 47), fill=STONE)
        if kind == "castle":
            for x in (16, 28, 40):
                draw.rectangle((x, 19, x + 9, 43), fill=OUTLINE)
                draw.rectangle((x + 2, 20, x + 7, 40), fill=STONE_HI)
                draw.rectangle((x + 2, 17, x + 7, 21), fill=OUTLINE)
            draw.polygon([(22, 31), (32, 20), (42, 31)], fill=OUTLINE)
            draw.polygon([(25, 30), (32, 23), (39, 30)], fill=RED)
            draw.rectangle((28, 37, 36, 48), fill=OUTLINE)
            draw.rectangle((30, 39, 34, 48), fill=WOOD)
        elif kind == "barracks":
            draw.polygon([(14, 35), (31, 16), (50, 35)], fill=OUTLINE)
            draw.polygon([(18, 34), (31, 20), (46, 34)], fill=RED)
            for x in (21, 34):
                draw.rectangle((x, 35, x + 6, 48), fill=WOOD_HI)
            px(draw, [(11, 42), (52, 42)], GOLD, 2)
        elif kind == "archery_range":
            for x in (19, 43, 31):
                px(draw, [(x, 48), (x + 8, 19)], WOOD_HI, 3)
                px(draw, [(x - 2, 25), (x + 10, 25)], WOOD, 1)
            draw.arc((30, 20, 51, 43), 110, 250, fill=IVORY, width=3)
        elif kind == "smithy":
            draw.polygon([(14, 37), (29, 19), (50, 37)], fill=OUTLINE)
            draw.polygon([(18, 35), (29, 23), (46, 35)], fill=STONE_HI)
            draw.rectangle((25, 34, 38, 48), fill=OUTLINE)
            draw.rectangle((28, 38, 35, 48), fill=(54, 42, 43, 255))
            draw.ellipse((29, 39, 35, 45), fill=RED)
            px(draw, [(39, 20), (48, 12)], STONE_HI, 2)
        elif kind == "lumbermill":
            draw.polygon([(14, 36), (31, 22), (50, 36)], fill=OUTLINE)
            draw.polygon([(18, 35), (31, 26), (46, 35)], fill=WOOD_HI)
            draw.ellipse((18, 32, 37, 51), fill=OUTLINE)
            draw.ellipse((21, 35, 34, 48), fill=WOOD)
            px(draw, [(27, 29), (27, 52)], IVORY, 1)
            px(draw, [(18, 41), (36, 41)], IVORY, 1)
        elif kind == "quarry":
            polygon(draw, [(13, 45), (19, 30), (31, 20), (47, 30), (52, 46)], STONE)
            polygon(draw, [(24, 40), (32, 27), (40, 40)], STONE_HI)
            px(draw, [(13, 18), (35, 47)], WOOD_HI, 3)
            polygon(draw, [(11, 15), (22, 15), (19, 24), (10, 24)], STONE_HI)
        elif kind == "farm":
            draw.polygon([(13, 38), (31, 21), (50, 38)], fill=OUTLINE)
            draw.polygon([(17, 37), (31, 25), (46, 37)], fill=WOOD_HI)
            for x in (18, 26, 34, 42):
                px(draw, [(x, 50), (x + 2, 33)], GOLD_HI, 2)
        elif kind == "market":
            draw.rectangle((15, 31, 49, 48), fill=OUTLINE)
            draw.rectangle((18, 34, 46, 47), fill=WOOD)
            for x, color in ((16, RED), (25, GOLD), (34, BLUE), (43, GREEN)):
                draw.rectangle((x, 25, x + 8, 35), fill=OUTLINE)
                draw.rectangle((x + 1, 27, x + 7, 34), fill=color)
        elif kind == "tavern":
            draw.polygon([(14, 37), (31, 21), (50, 37)], fill=OUTLINE)
            draw.polygon([(18, 35), (31, 25), (46, 35)], fill=WOOD_HI)
            draw.rectangle((21, 36, 42, 48), fill=WOOD)
            draw.ellipse((28, 38, 36, 48), fill=OUTLINE)
            draw.ellipse((30, 40, 34, 47), fill=GOLD)
            px(draw, [(14, 19), (14, 34)], IVORY, 1)
        elif kind == "academy":
            for x in (16, 27, 38):
                draw.rectangle((x, 30, x + 6, 48), fill=OUTLINE)
                draw.rectangle((x + 2, 31, x + 4, 47), fill=IVORY)
            draw.polygon([(12, 30), (31, 15), (52, 30)], fill=OUTLINE)
            draw.polygon([(17, 28), (31, 19), (47, 28)], fill=BLUE)
            draw.ellipse((28, 10, 34, 16), fill=GOLD_HI)
        else:
            raise ValueError(f"Unknown city building: {kind}")
    return paint


def save_city_building(name: str) -> None:
    img, draw = icon_canvas()
    city_building(name)(draw)
    img.resize((ICON_SIZE * SCALE, ICON_SIZE * SCALE), Image.Resampling.NEAREST).save(CITY_BUILDINGS / f"building_{name}.png")


def rounded_rect(draw, box, radius, fill, outline=None, width=0):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def button(name: str, base, highlight, shadow, rim) -> None:
    width, height = 192, 128
    img = Image.new("RGBA", (width, height), TRANSPARENT)
    draw = ImageDraw.Draw(img)
    rounded_rect(draw, (5, 9, 187, 121), 20, OUTLINE)
    rounded_rect(draw, (9, 8, 183, 112), 16, shadow)
    rounded_rect(draw, (11, 7, 181, 102), 15, base)
    rounded_rect(draw, (18, 14, 174, 92), 11, None, outline=rim, width=3)
    draw.rectangle((29, 18, 163, 23), fill=highlight)
    draw.rectangle((38, 84, 154, 88), fill=shadow)
    img.resize((width * SCALE, height * SCALE), Image.Resampling.NEAREST).save(BUTTONS / f"button_{name}.png")


def main() -> None:
    ICONS.mkdir(parents=True, exist_ok=True)
    BUTTONS.mkdir(parents=True, exist_ok=True)
    CITY_BUILDINGS.mkdir(parents=True, exist_ok=True)
    painters = {
        "food": food, "wood": wood, "stone": stone, "gold": gold, "gems": gems,
        "build": build, "train": train, "map": map_icon, "reports": reports,
        "alliance": alliance, "research": research, "heal": heal, "speed": speed,
        "settings": settings, "bell": bell, "bag": bag,
    }
    for name, painter in painters.items():
        save_icon(name, painter)
    city_buildings = [
        "castle", "barracks", "archery_range", "smithy", "lumbermill",
        "quarry", "farm", "market", "tavern", "academy",
    ]
    for name in city_buildings:
        save_city_building(name)
    button("primary_gold", GOLD, GOLD_HI, (135, 83, 26, 255), IVORY)
    button("secondary_blue", BLUE, BLUE_HI, (24, 76, 128, 255), IVORY)
    button("success_green", GREEN, GREEN_HI, (27, 103, 58, 255), IVORY)
    button("danger_red", RED, (240, 125, 110, 255), (112, 33, 36, 255), IVORY)
    print(f"Created {len(painters)} icons in {ICONS}")
    print(f"Created {len(city_buildings)} city building portraits in {CITY_BUILDINGS}")
    print(f"Created 4 button skins in {BUTTONS}")


if __name__ == "__main__":
    main()
