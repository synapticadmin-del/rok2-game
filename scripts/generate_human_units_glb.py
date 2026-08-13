#!/usr/bin/env python3
"""
P8-T8: مولّد موديلات الوحدات البشرية 3D (procedural GLB low-poly) — متسق مع أسلوب KayKit CC0.

يولّد 17 موديلًا في game/client-unreal/Content/Art/HumanUnits/:
  infantry_t1..t5, archer_t1..t5, cavalry_t1..t5 (15)
  siege_arcuballista_t1, siege_mangonel_t2 (2) — T3/T4/T5 siege تُعاد استخدام
  موديلات Kenney Castle Kit الحالية (Ballista/Trebuchet/Catapult).

التمثيل: كبسولة/مكعبات low-poly ملونة بتدرج tier (t1 أخضر فاتح → t5 ذهبي مزخرف)،
مقياس سنتيمتر UE (ارتفاع المشاة ~175cm). الوحدات الخاصة الحضارية الست تشترك مع
موديلات فرعها tier 4 (legionary/huskarl/khopesh_guard/samurai=infantry_t4,
chu_ko_nu=archer_t4, desert_rider=cavalry_t4) — يُوثّق في الدليل.

التوليد GLB خام (header + JSON chunk + BIN chunk) بدون مكتبات خارجية.
"""
import json
import math
import os
import struct
import sys

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "game", "client-unreal", "Content", "Art", "HumanUnits")

BRANCHES = ("infantry", "archer", "cavalry")

# تدرج ألوان tier: t1 أخضر فاتح → t5 ذهبي مزخرف (أسلوب عسكري متدرج)
TIER_COLORS = {
    1: (0.55, 0.70, 0.42),   # تجنيد فاتح
    2: (0.45, 0.60, 0.38),   # عسكري
    3: (0.38, 0.50, 0.34),   # محترف داكن
    4: (0.50, 0.45, 0.32),   # ملكي برونزي
    5: (0.75, 0.62, 0.25),   # إمبراطوري ذهبي
}

SKIN = (0.85, 0.70, 0.55)
ACCENT = (0.35, 0.25, 0.15)


def add_vertices(verts, normals, indices, shape_fn):
    """يلحق مضلعات shape_fn() بالقوائم ويعيد فهرس أول قمة جديدة."""
    base = len(verts)
    for v, n in shape_fn():
        verts.extend(v)
        normals.extend(n)
    return base


def box(cx, cy, cz, sx, sy, sz, color):
    """يولد مكعبًا بـ 24 قمة (مسطحات منفصلة لإضاءة حادة low-poly) ويوجهه."""
    x, y, z = cx, cy, cz
    hx, hy, hz = sx / 2, sy / 2, sz / 2
    faces = (
        # (+x, -x, +y, -y, +z, -z) كل وجه 4 قمم + طبيعته
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
    return [(tuple(p), tuple(n)) for (p, n), c in zip(faces, [color] * 12)]


def quad_indices(base):
    """فهرسة مكعب من قمتين لكل مثلث (12 مثلث × 3)."""
    idx = []
    for f in range(6):
        b = base + f * 4
        idx += (b, b + 1, b + 2, b + 1, b + 3, b + 2)
    return idx


def humanoid_mesh(tier, pose):
    """يولّد قائمة (vertex, normal) لموديل إنسان low-poly حسب الفرع والمرحلة.
    pose في {'infantry','archer','cavalry'} يغيّر وضعية الذراعين/الساقين والحجم."""
    c = TIER_COLORS[tier]
    verts = []
    # الجسم: جذع + رأس + ذراعان + ساقان (كل جزء مكعب منخفض المضلعات)
    parts = []
    if pose == "cavalry":
        # فارس فوق حصان مبسط
        parts += box(0, 135, 0, 40, 50, 30, (0.65, 0.45, 0.30))      # حصان جذع
        parts += box(0, 205, 0, 22, 26, 22, SKIN)                     # راكب جذع
        parts += box(0, 245, 0, 16, 16, 16, SKIN)                     # راكب رأس
        parts += box(-8, 212, -24, 8, 44, 8, c)                       # ذراع أمامية بمطرقة
        parts += box(8, 212, -20, 8, 40, 8, c)                        # ذراع خلفية
        parts += box(-22, 90, -30, 12, 90, 14, ACCENT)                # ساق حصان
        parts += box(22, 90, -30, 12, 90, 14, ACCENT)                 # ساق حصان
        parts += box(-22, 90, 30, 12, 90, 14, ACCENT)                 # ساق حصان
        parts += box(22, 90, 30, 12, 90, 14, ACCENT)                  # ساق حصان
    elif pose == "archer":
        parts += box(0, 120, 0, 34, 55, 26, c)                        # جذع
        parts += box(0, 170, 0, 16, 17, 16, SKIN)                     # رأس
        parts += box(-22, 135, -8, 9, 48, 9, SKIN)                    # ذراع رامية (خلف)
        parts += box(22, 135, -8, 9, 48, 9, SKIN)                     # ذراع أمامية
        parts += box(0, 162, -40, 6, 70, 8, ACCENT)                   # قوس أمامي
        parts += box(-10, 55, 0, 14, 62, 14, ACCENT)                  # ساق
        parts += box(10, 55, 0, 14, 62, 14, ACCENT)                   # ساق
    else:  # infantry
        parts += box(0, 120, 0, 38, 58, 28, c)                        # جذع
        parts += box(0, 172, 0, 17, 18, 17, SKIN)                     # رأس (خوذة بلون tier)
        parts += box(-24, 132, -6, 10, 50, 10, SKIN)                  # ذراع بسيف
        parts += box(24, 132, -6, 10, 50, 10, SKIN)                   # ذرع درع
        parts += box(0, 128, -30, 6, 55, 9, (0.85, 0.85, 0.88))       # سيف
        parts += box(0, 130, 22, 10, 42, 5, (0.75, 0.75, 0.78))       # درع أمامي
        parts += box(-11, 55, 0, 15, 64, 15, ACCENT)                  # ساق
        parts += box(11, 55, 0, 15, 64, 15, ACCENT)                   # ساق
    return parts


def siege_mesh(model):
    """مقذوفات حصار tier 1-2 المكملة لموديلات Kenney T3-T5."""
    parts = []
    if model == "arcuballista":
        parts += box(0, 60, 0, 120, 30, 50, (0.45, 0.35, 0.25))       # قاعدة خشبية
        parts += box(0, 85, -10, 14, 40, 12, (0.40, 0.30, 0.20))      # ذراع نطاط
        parts += box(0, 62, -45, 6, 20, 120, (0.35, 0.28, 0.18))      # منصة إطلاق
    else:  # mangonel
        parts += box(0, 55, 0, 140, 34, 56, (0.45, 0.35, 0.25))       # قاعدة خشبية
        parts += box(0, 90, 0, 12, 60, 14, (0.40, 0.30, 0.20))        # ذراع قاذف
        parts += box(0, 125, 20, 18, 18, 18, (0.50, 0.42, 0.30))      # دلو الحجر
    return parts


def build_glb(vertices, indices, scale):
    """يحزم مصفوفة رؤوس/فهارس في GLB (بدون مكتبات خارجية). المقياس بالسنتيمتر UE."""
    pos = []
    nor = []
    for v, n in vertices:
        pos.extend([v[0] * scale, v[1] * scale, v[2] * scale])
        nor.extend(n)
    pos_b = bytes(b for f in pos for b in struct.pack("<f", f))
    nor_b = bytes(b for f in nor for b in struct.pack("<f", f))
    idx_b = bytes(b for i in indices for b in struct.pack("<I", i))

    acc_len = len(pos_b) + len(nor_b) + len(idx_b)
    json_obj = {
        "asset": {"version": "2.0", "generator": "rok2-p8-t8-generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "name": "HumanUnit",
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
             "max": [max(p[0] for p, _ in vertices) * scale, max(p[1] for p, _ in vertices) * scale, max(p[2] for p, _ in vertices) * scale],
             "min": [min(p[0] for p, _ in vertices) * scale, min(p[1] for p, _ in vertices) * scale, min(p[2] for p, _ in vertices) * scale]},
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


def main():
    os.makedirs(OUT, exist_ok=True)
    written = []
    for branch in BRANCHES:
        for tier in range(1, 6):
            name = f"{branch}_t{tier}"
            verts = humanoid_mesh(tier, branch)
            idx = quad_indices(0)
            # المشاة/الرماة بمقياس 175cm، الفرسان أكبر (حصان)
            scale = 1.9 if branch == "cavalry" else 1.75
            data = build_glb(verts, idx, scale)
            path = os.path.join(OUT, f"{name}.glb")
            with open(path, "wb") as f:
                f.write(data)
            written.append(path)
    for model in ("arcuballista", "mangonel"):
        name = f"siege_{model}"
        verts = siege_mesh(model)
        idx = quad_indices(0)
        data = build_glb(verts, idx, 1.0)
        path = os.path.join(OUT, f"{name}.glb")
        with open(path, "wb") as f:
            f.write(data)
        written.append(path)
    print(f"P8-T8: {len(written)} human unit GLBs written to {OUT}")
    for p in written:
        print("  ", os.path.basename(p), os.path.getsize(p), "bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
