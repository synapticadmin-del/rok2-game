#!/usr/bin/env python3
"""
P10-T7: مولّد الأصول البصرية للحانة والصناديق والمفاتيح والمنحوتات والمواد ومخططات الحداد (ROK2).

يولّد:
  1. موديلات 3D بصيغة GLB 2.0 (متسقة مع أسلوب KayKit low-poly):
     - game/client-unreal/Content/Art/Tavern/building_tavern.glb
     - game/client-unreal/Content/Art/Tavern/chest_silver.glb
     - game/client-unreal/Content/Art/Tavern/chest_gold.glb
     - game/client-unreal/Content/Art/Tavern/chest_equipment.glb
  2. أيقونات 2D بصيغة PNG شفافة بدقة 128x128:
     - 3 صناديق: chest_silver, chest_gold, chest_equipment
     - 6 مفاتيح: key_silver, key_gold, key_equipment, key_expedition, key_canyon, key_osiris
     - 4 منحوتات قادة: sculpture_legendary, sculpture_epic, sculpture_elite, sculpture_advanced
     - 4 مواد حداد: material_leather, material_iron, material_ebony, material_crystal
     - 6 مخططات معدات: blueprint_weapon, blueprint_helm, blueprint_chest, blueprint_gloves, blueprint_legs, blueprint_boots
  3. مؤثرات صوتية بصيغة WAV (16-bit PCM 44.1kHz):
     - game/client-unreal/Content/Audio/sfx/chest_open.wav
     - game/client-unreal/Content/Audio/sfx/wheel_spin.wav
"""
import json
import math
import os
import struct
import sys
import wave
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TAVERN_ART = os.path.join(ROOT, "game", "client-unreal", "Content", "Art", "Tavern")
SFX_AUDIO = os.path.join(ROOT, "game", "client-unreal", "Content", "Audio", "sfx")

# ----------------------------------------------------------------------
# 1. Geometry & GLB 2.0 Packing
# ----------------------------------------------------------------------

def box(cx, cy, cz, sx, sy, sz, color=(0.7, 0.7, 0.7)):
    """يولد مكعباً بـ 24 رأساً مع النورمالز المحددة لأوجه حادة low-poly."""
    x, y, z = cx, cy, cz
    hx, hy, hz = sx / 2, sy / 2, sz / 2
    faces = (
        ((x + hx, y - hy, z + hz), (1, 0, 0)), ((x + hx, y - hy, z - hz), (1, 0, 0)),
        ((x + hx, y + hy, z + hz), (1, 0, 0)), ((x + hx, y + hy, z - hz), (1, 0, 0)),
        ((x - hx, y - hy, z - hz), (-1, 0, 0)), ((x - hx, y - hy, z + hz), (-1, 0, 0)),
        ((x - hx, y + hy, z - hz), (-1, 0, 0)), ((x - hx, y + hy, z + hz), (-1, 0, 0)),
        ((x - hx, y + hy, z + hz), (0, 1, 0)), ((x - hx, y + hy, z - hz), (0, 1, 0)),
        ((x + hx, y + hy, z + hz), (0, 1, 0)), ((x + hx, y + hy, z - hz), (0, 1, 0)),
        ((x - hx, y - hy, z - hz), (0, -1, 0)), ((x - hx, y - hy, z + hz), (0, -1, 0)),
        ((x + hx, y - hy, z - hz), (0, -1, 0)), ((x + hx, y - hy, z + hz), (0, -1, 0)),
        ((x - hx, y - hy, z + hz), (0, 0, 1)), ((x + hx, y - hy, z + hz), (0, 0, 1)),
        ((x - hx, y + hy, z + hz), (0, 0, 1)), ((x + hx, y + hy, z + hz), (0, 0, 1)),
        ((x + hx, y - hy, z - hz), (0, 0, -1)), ((x - hx, y - hy, z - hz), (0, 0, -1)),
        ((x + hx, y + hy, z - hz), (0, 0, -1)), ((x - hx, y + hy, z - hz), (0, 0, -1)),
    )
    return [(tuple(p), tuple(n)) for p, n in faces]


def quad_indices(num_boxes):
    idx = []
    for i in range(num_boxes):
        base = i * 24
        for f in range(6):
            b = base + f * 4
            idx += [b, b + 1, b + 2, b + 1, b + 3, b + 2]
    return idx


def build_glb(vertices, indices, mesh_name="Model"):
    pos = []
    nor = []
    for v, n in vertices:
        pos.extend([v[0], v[1], v[2]])
        nor.extend(n)
    pos_b = bytes(b for f in pos for b in struct.pack("<f", f))
    nor_b = bytes(b for f in nor for b in struct.pack("<f", f))
    idx_b = bytes(b for i in indices for b in struct.pack("<I", i))

    acc_len = len(pos_b) + len(nor_b) + len(idx_b)
    json_obj = {
        "asset": {"version": "2.0", "generator": "rok2-p10-t7-generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "name": mesh_name,
            "primitives": [{
                "attributes": {"POSITION": 0, "NORMAL": 1},
                "indices": 2,
                "mode": 4,
            }]
        }],
        "buffers": [{"byteLength": acc_len}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(pos_b), "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_b), "byteLength": len(nor_b), "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_b) + len(nor_b), "byteLength": len(idx_b), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(vertices), "type": "VEC3",
             "max": [max(p[0] for p, _ in vertices), max(p[1] for p, _ in vertices), max(p[2] for p, _ in vertices)],
             "min": [min(p[0] for p, _ in vertices), min(p[1] for p, _ in vertices), min(p[2] for p, _ in vertices)]},
            {"bufferView": 1, "componentType": 5126, "count": len(vertices), "type": "VEC3"},
            {"bufferView": 2, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
        ],
    }
    json_bytes = json.dumps(json_obj, separators=(",", ":")).encode()
    json_pad = -len(json_bytes) % 4
    bin_pad = -acc_len % 4

    glb = bytearray()
    glb += b"glTF"
    glb += struct.pack("<I", 2)
    total = 12 + 8 + (len(json_bytes) + json_pad) + 8 + (acc_len + bin_pad)
    glb += struct.pack("<I", total)
    # JSON chunk
    glb += struct.pack("<I", len(json_bytes) + json_pad) + b"JSON"
    glb += json_bytes + b" " * json_pad
    # BIN chunk
    glb += struct.pack("<I", acc_len + bin_pad) + b"BIN\x00"
    glb += pos_b + nor_b + idx_b + b"\x00" * bin_pad
    return bytes(glb)


def make_tavern_mesh():
    parts = []
    # Base stone platform
    parts += box(0, 15, 0, 180, 30, 180)
    # First floor wooden structure
    parts += box(0, 75, 0, 150, 90, 150)
    # Second floor overhang
    parts += box(0, 145, 0, 170, 70, 170)
    # Pitched roof (stepped cubes)
    parts += box(0, 195, 0, 190, 30, 190)
    parts += box(0, 220, 0, 140, 30, 140)
    parts += box(0, 240, 0, 90, 20, 90)
    # Chimney
    parts += box(50, 210, 40, 30, 120, 30)
    # Front entrance door frame & barrel
    parts += box(0, 50, 78, 40, 60, 10)
    parts += box(55, 35, 75, 25, 40, 25)
    # Tavern Hanging Sign Post
    parts += box(-70, 80, 80, 8, 40, 8)
    parts += box(-70, 95, 95, 6, 6, 30)
    parts += box(-70, 85, 105, 4, 18, 22)
    return parts


def make_chest_mesh(chest_type="silver"):
    parts = []
    # Chest base container
    parts += box(0, 25, 0, 90, 50, 60)
    # Chest lid
    parts += box(0, 60, 0, 96, 24, 66)
    parts += box(0, 74, 0, 88, 12, 56)
    # Metal corner / band trims
    parts += box(-43, 30, 0, 6, 60, 62)
    parts += box(43, 30, 0, 6, 60, 62)
    parts += box(0, 30, 0, 12, 62, 62)
    # Lock latch
    parts += box(0, 45, 32, 16, 20, 6)
    if chest_type == "gold":
        # Gem atop the gold chest
        parts += box(0, 82, 0, 18, 14, 18)
    elif chest_type == "equipment":
        # Crossed sword ornament emblem
        parts += box(0, 68, 32, 28, 8, 4)
        parts += box(0, 68, 32, 8, 28, 4)
    return parts


# ----------------------------------------------------------------------
# 2. 2D Texture PNG Generation (Pillow)
# ----------------------------------------------------------------------

OUTLINE = (22, 24, 38, 255)
GOLD = (245, 192, 48, 255)
GOLD_HI = (255, 235, 130, 255)
SILVER = (195, 205, 218, 255)
SILVER_HI = (240, 246, 255, 255)
BRONZE = (185, 115, 65, 255)
WOOD = (120, 72, 38, 255)
WOOD_HI = (175, 115, 65, 255)
PURPLE = (165, 80, 235, 255)
PURPLE_HI = (215, 150, 255, 255)
BLUE = (50, 135, 240, 255)
BLUE_HI = (130, 200, 255, 255)
GREEN = (45, 185, 95, 255)
GREEN_HI = (120, 240, 160, 255)
RED = (220, 60, 60, 255)
IVORY = (250, 242, 220, 255)


def new_canvas(size=128):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def draw_chest_png(path, chest_type="gold"):
    img, d = new_canvas(128)
    # Background glow/shadow
    d.ellipse((20, 85, 108, 115), fill=(0, 0, 0, 80))

    main_col = GOLD if chest_type == "gold" else (SILVER if chest_type == "silver" else BRONZE)
    hi_col = GOLD_HI if chest_type == "gold" else (SILVER_HI if chest_type == "silver" else GOLD)

    # Base body (Wood)
    d.rectangle((24, 56, 104, 100), fill=OUTLINE)
    d.rectangle((28, 60, 100, 96), fill=WOOD)
    d.line([(32, 75), (96, 75)], fill=WOOD_HI, width=2)
    d.line([(32, 85), (96, 85)], fill=(80, 45, 22, 255), width=2)

    # Metal corner brackets
    for x in (24, 94):
        d.rectangle((x, 56, x + 10, 100), fill=OUTLINE)
        d.rectangle((x + 2, 58, x + 8, 98), fill=main_col)

    # Central metal band
    d.rectangle((58, 56, 70, 100), fill=OUTLINE)
    d.rectangle((60, 58, 68, 98), fill=main_col)

    # Lid (Arc / Polygon)
    d.polygon([(18, 56), (28, 30), (100, 30), (110, 56)], fill=OUTLINE)
    d.polygon([(22, 54), (31, 34), (97, 34), (106, 54)], fill=WOOD_HI)
    d.polygon([(34, 54), (40, 34), (88, 34), (94, 54)], fill=WOOD)

    # Lid bands
    for x_b, x_t in ((20, 29), (58, 59), (96, 92)):
        d.polygon([(x_b, 56), (x_t, 32), (x_t + 8, 32), (x_b + 10, 56)], fill=OUTLINE)
        d.polygon([(x_b + 2, 54), (x_t + 2, 34), (x_t + 6, 34), (x_b + 8, 54)], fill=main_col)

    # Lock Latch
    d.rectangle((54, 52, 74, 76), fill=OUTLINE)
    d.rectangle((56, 54, 72, 74), fill=main_col)
    d.ellipse((60, 60, 68, 68), fill=OUTLINE)
    d.ellipse((62, 62, 66, 66), fill=hi_col)

    if chest_type == "gold":
        # Gem ornament
        d.polygon([(64, 22), (74, 32), (64, 42), (54, 32)], fill=OUTLINE)
        d.polygon([(64, 25), (71, 32), (64, 39), (57, 32)], fill=RED)
    elif chest_type == "equipment":
        # Swords cross emblem on latch
        d.line([(50, 48), (78, 76)], fill=SILVER_HI, width=3)
        d.line([(78, 48), (50, 76)], fill=SILVER_HI, width=3)

    img.save(path, "PNG")


def draw_key_png(path, key_type="gold"):
    img, d = new_canvas(128)
    color_map = {
        "gold": (GOLD, GOLD_HI),
        "silver": (SILVER, SILVER_HI),
        "equipment": (BRONZE, GOLD_HI),
        "expedition": (GREEN, GREEN_HI),
        "canyon": (PURPLE, PURPLE_HI),
        "osiris": (BLUE, BLUE_HI),
    }
    col, hi = color_map.get(key_type, (GOLD, GOLD_HI))

    # Shadow
    d.ellipse((35, 35, 95, 95), fill=(0, 0, 0, 40))

    # Key Bow / Ring (Top-Left)
    d.ellipse((22, 22, 66, 66), fill=OUTLINE)
    d.ellipse((26, 26, 62, 62), fill=col)
    d.ellipse((36, 36, 52, 52), fill=OUTLINE)
    d.ellipse((40, 40, 48, 48), fill=(0, 0, 0, 0))

    # Key Shaft (Diagonal)
    d.line([(54, 54), (98, 98)], fill=OUTLINE, width=14)
    d.line([(54, 54), (98, 98)], fill=col, width=10)
    d.line([(52, 52), (96, 96)], fill=hi, width=3)

    # Key Bit / Teeth (Bottom-Right)
    d.polygon([(82, 82), (102, 62), (110, 70), (90, 90)], fill=OUTLINE)
    d.polygon([(84, 82), (100, 66), (106, 72), (90, 88)], fill=col)
    d.polygon([(92, 92), (106, 78), (114, 86), (100, 100)], fill=OUTLINE)
    d.polygon([(94, 92), (104, 82), (110, 88), (100, 98)], fill=col)

    # Center jewel / symbol on key head
    if key_type == "osiris":
        # Ankh / Eye of Horus detail
        d.line([(34, 44), (54, 44)], fill=OUTLINE, width=4)
        d.line([(44, 34), (44, 54)], fill=OUTLINE, width=4)
    else:
        d.ellipse((41, 41, 47, 47), fill=hi)

    img.save(path, "PNG")


def draw_sculpture_png(path, rarity="legendary"):
    img, d = new_canvas(128)
    color_map = {
        "legendary": (GOLD, GOLD_HI),
        "epic": (PURPLE, PURPLE_HI),
        "elite": (BLUE, BLUE_HI),
        "advanced": (GREEN, GREEN_HI),
    }
    col, hi = color_map.get(rarity, (GOLD, GOLD_HI))

    # Pedestal base
    d.polygon([(26, 114), (102, 114), (94, 98), (34, 98)], fill=OUTLINE)
    d.polygon([(29, 112), (99, 112), (92, 100), (36, 100)], fill=col)

    # Commander Bust / Statue
    # Head & Helmet
    d.ellipse((44, 20, 84, 60), fill=OUTLINE)
    d.ellipse((47, 23, 81, 57), fill=col)
    # Crest / plume
    d.polygon([(56, 10), (72, 10), (68, 24), (60, 24)], fill=OUTLINE)
    d.polygon([(58, 12), (70, 12), (66, 22), (62, 22)], fill=hi)
    # Visor
    d.rectangle((52, 38, 76, 44), fill=OUTLINE)
    d.rectangle((54, 40, 74, 42), fill=hi)

    # Shoulders & Armor Torso
    d.polygon([(28, 96), (100, 96), (88, 62), (40, 62)], fill=OUTLINE)
    d.polygon([(32, 94), (96, 94), (85, 65), (43, 65)], fill=col)
    # Collar / Chest emblem
    d.polygon([(64, 66), (78, 80), (64, 92), (50, 80)], fill=OUTLINE)
    d.polygon([(64, 69), (75, 80), (64, 89), (53, 80)], fill=hi)

    # Star / Sparkle
    d.polygon([(64, 74), (67, 80), (73, 80), (68, 84), (70, 90), (64, 86), (58, 90), (60, 84), (55, 80), (61, 80)], fill=IVORY)

    img.save(path, "PNG")


def draw_material_png(path, mat="iron"):
    img, d = new_canvas(128)
    if mat == "iron":
        # Metallic Iron Ingots Stack
        for offset_y, offset_x in ((20, 0), (0, -18), (0, 18)):
            d.polygon([(36 + offset_x, 70 + offset_y), (84 + offset_x, 70 + offset_y),
                       (96 + offset_x, 86 + offset_y), (24 + offset_x, 86 + offset_y)], fill=OUTLINE)
            d.polygon([(38 + offset_x, 72 + offset_y), (82 + offset_x, 72 + offset_y),
                       (93 + offset_x, 84 + offset_y), (27 + offset_x, 84 + offset_y)], fill=SILVER)
            d.line([(38 + offset_x, 73 + offset_y), (82 + offset_x, 73 + offset_y)], fill=SILVER_HI, width=3)
    elif mat == "leather":
        # Tanned Leather hide bundle
        d.polygon([(30, 36), (98, 36), (106, 92), (22, 92)], fill=OUTLINE)
        d.polygon([(34, 40), (94, 40), (102, 88), (26, 88)], fill=BRONZE)
        d.line([(30, 64), (98, 64)], fill=OUTLINE, width=6)
        d.line([(30, 64), (98, 64)], fill=GOLD, width=3)
    elif mat == "ebony":
        # Dark Ebony Wood Planks
        for y in (30, 56, 82):
            d.rectangle((24, y, 104, y + 20), fill=OUTLINE)
            d.rectangle((26, y + 2, 102, y + 18), fill=(60, 42, 30, 255))
            d.line([(30, y + 6), (98, y + 6)], fill=(90, 65, 48, 255), width=2)
    elif mat == "crystal":
        # Glowing Mana Crystal cluster
        d.polygon([(64, 18), (88, 54), (64, 108), (40, 54)], fill=OUTLINE)
        d.polygon([(64, 22), (84, 54), (64, 102), (44, 54)], fill=BLUE)
        d.polygon([(64, 22), (76, 54), (64, 102)], fill=BLUE_HI)
        # Side crystals
        d.polygon([(30, 44), (48, 62), (36, 96), (20, 72)], fill=OUTLINE)
        d.polygon([(32, 48), (45, 62), (35, 92), (23, 72)], fill=PURPLE)
        d.polygon([(98, 44), (108, 72), (92, 96), (80, 62)], fill=OUTLINE)
        d.polygon([(96, 48), (105, 72), (93, 92), (83, 62)], fill=PURPLE_HI)

    img.save(path, "PNG")


def draw_blueprint_png(path, item="weapon"):
    img, d = new_canvas(128)
    # Scroll / Parchment background
    d.rectangle((22, 20, 106, 108), fill=OUTLINE)
    d.rectangle((26, 24, 102, 104), fill=(35, 65, 115, 255))  # Blueprint Cyan/Blue
    # Grid lines on blueprint
    for g in range(34, 100, 12):
        d.line([(26, g), (102, g)], fill=(48, 85, 145, 255), width=1)
        d.line([(g, 24), (g, 104)], fill=(48, 85, 145, 255), width=1)

    # Technical White/Cyan Sketch of the item
    if item == "weapon":
        # Sword diagram
        d.line([(36, 92), (88, 40)], fill=SILVER_HI, width=4)
        d.line([(52, 70), (70, 88)], fill=SILVER_HI, width=3)
        d.line([(32, 96), (40, 88)], fill=SILVER_HI, width=3)
    elif item == "helm":
        # Helmet diagram
        d.arc((42, 40, 86, 84), 180, 360, fill=SILVER_HI, width=3)
        d.rectangle((42, 62, 86, 86), outline=SILVER_HI, width=3)
    elif item == "chest":
        # Cuirass diagram
        d.polygon([(40, 42), (88, 42), (80, 88), (48, 88)], outline=SILVER_HI, fill=None)
    elif item == "gloves":
        # Gauntlet diagram
        d.polygon([(46, 44), (82, 44), (78, 86), (50, 86)], outline=SILVER_HI, fill=None)
    elif item == "legs":
        # Greaves diagram
        d.polygon([(42, 42), (58, 42), (54, 90), (44, 90)], outline=SILVER_HI, fill=None)
        d.polygon([(70, 42), (86, 42), (84, 90), (74, 90)], outline=SILVER_HI, fill=None)
    elif item == "boots":
        # Boots diagram
        d.polygon([(40, 48), (62, 48), (62, 80), (78, 80), (78, 92), (40, 92)], outline=SILVER_HI, fill=None)

    # Seal at bottom corner
    d.ellipse((84, 84, 102, 102), fill=GOLD)
    d.ellipse((87, 87, 99, 99), fill=RED)

    img.save(path, "PNG")


# ----------------------------------------------------------------------
# 3. Audio SFX Generation (16-bit PCM 44.1kHz)
# ----------------------------------------------------------------------

def generate_wav(path, duration, synth_fn, sr=44100):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    num_samples = int(duration * sr)
    samples = []
    for i in range(num_samples):
        t = i / sr
        val = synth_fn(t, duration)
        val = max(-1.0, min(1.0, val))
        samples.append(int(val * 32767))

    pcm = struct.pack(f"<{len(samples)}h", *samples)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(pcm)


def chest_open_sfx(t, dur):
    # Creak (low saw) + magical chime harmonics
    envelope = math.exp(-2.5 * t)
    creak = 0.35 * math.sin(2 * math.pi * (120 + 30 * t) * t) * (1.0 if t < 0.25 else math.exp(-6 * (t - 0.25)))
    # Chimes
    chime = 0.0
    for f in (523.25, 659.25, 783.99, 1046.5): # C5, E5, G5, C6 arpeggio
        chime += 0.2 * math.sin(2 * math.pi * f * t) * math.exp(-4 * t)
    sparkle = 0.1 * math.sin(2 * math.pi * 2093 * t) * math.exp(-8 * t)
    return (creak + chime + sparkle) * envelope


def wheel_spin_sfx(t, dur):
    # Rapid clicking ticks that gradually decelerate
    envelope = math.exp(-1.5 * t)
    # Frequency drops from 40 clicks/sec to 4 clicks/sec
    click_freq = 35.0 * (1.0 - (t / dur) ** 0.6)
    click = math.sin(2 * math.pi * click_freq * t * 8.0)
    click_pulse = math.sin(2 * math.pi * click_freq * t) ** 16
    chime = 0.15 * math.sin(2 * math.pi * 880 * t) * (1.0 - t / dur)
    return (click * click_pulse * 0.7 + chime) * envelope


# ----------------------------------------------------------------------
# 4. Main Entry Point
# ----------------------------------------------------------------------

def main():
    os.makedirs(TAVERN_ART, exist_ok=True)
    os.makedirs(SFX_AUDIO, exist_ok=True)

    print("Generating 3D Models (.glb)...")
    # Tavern GLB
    t_verts = make_tavern_mesh()
    t_indices = quad_indices(len(t_verts) // 24)
    t_glb = build_glb(t_verts, t_indices, "TavernBuilding")
    with open(os.path.join(TAVERN_ART, "building_tavern.glb"), "wb") as f:
        f.write(t_glb)

    # 3 Chests GLBs
    for c_type in ("silver", "gold", "equipment"):
        c_verts = make_chest_mesh(c_type)
        c_indices = quad_indices(len(c_verts) // 24)
        c_glb = build_glb(c_verts, c_indices, f"Chest_{c_type.capitalize()}")
        with open(os.path.join(TAVERN_ART, f"chest_{c_type}.glb"), "wb") as f:
            f.write(c_glb)

    print("Generating 2D Icons (.png)...")
    # 3 Chest Icons
    for c_type in ("silver", "gold", "equipment"):
        draw_chest_png(os.path.join(TAVERN_ART, f"chest_{c_type}.png"), c_type)

    # 6 Keys Icons
    for k_type in ("silver", "gold", "equipment", "expedition", "canyon", "osiris"):
        draw_key_png(os.path.join(TAVERN_ART, f"key_{k_type}.png"), k_type)

    # 4 Commander Sculptures Icons
    for r_type in ("legendary", "epic", "elite", "advanced"):
        draw_sculpture_png(os.path.join(TAVERN_ART, f"sculpture_{r_type}.png"), r_type)

    # 4 Equipment Material Icons
    for m_type in ("leather", "iron", "ebony", "crystal"):
        draw_material_png(os.path.join(TAVERN_ART, f"material_{m_type}.png"), m_type)

    # 6 Equipment Blueprint Icons
    for b_type in ("weapon", "helm", "chest", "gloves", "legs", "boots"):
        draw_blueprint_png(os.path.join(TAVERN_ART, f"blueprint_{b_type}.png"), b_type)

    print("Generating Audio SFX (.wav)...")
    generate_wav(os.path.join(SFX_AUDIO, "chest_open.wav"), 1.6, chest_open_sfx)
    generate_wav(os.path.join(SFX_AUDIO, "wheel_spin.wav"), 2.2, wheel_spin_sfx)

    print("\n[OK] P10-T7 Visual & Audio Assets successfully generated in:")
    print(f"  - Art: {TAVERN_ART}")
    print(f"  - SFX: {SFX_AUDIO}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
