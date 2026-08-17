#!/usr/bin/env python3
"""Generate tiny procedural Tavern/Chest GLB fixtures for ROK2.

The generator intentionally uses only Python's standard library.  It produces
low-poly, untextured GLB 2.0 meshes so the license audit can reproduce the
asset provenance without downloading or invoking external art tools.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
from pathlib import Path


def box(vertices: list[tuple[float, float, float]], indices: list[int], cx: float, cy: float, cz: float, sx: float, sy: float, sz: float) -> None:
    base = len(vertices)
    for x, y, z in ((-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1), (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)):
        vertices.append((cx + x * sx, cy + y * sy, cz + z * sz))
    for face in ((0, 1, 2, 0, 2, 3), (4, 6, 5, 4, 7, 6), (0, 4, 5, 0, 5, 1), (3, 2, 6, 3, 6, 7), (1, 5, 6, 1, 6, 2), (0, 3, 7, 0, 7, 4)):
        indices.extend(base + i for i in face)


def make_mesh(kind: str) -> tuple[list[tuple[float, float, float]], list[int]]:
    vertices: list[tuple[float, float, float]] = []
    indices: list[int] = []
    if kind == "tavern":
        box(vertices, indices, 0, 1.6, 0, 2.4, 1.6, 2.0)
        box(vertices, indices, 0, 3.9, 0, 2.8, 0.25, 2.3)
        box(vertices, indices, -1.4, 0.2, 0, 0.25, 0.8, 1.7)
        box(vertices, indices, 1.4, 0.2, 0, 0.25, 0.8, 1.7)
    else:
        box(vertices, indices, 0, 0.7, 0, 1.1, 0.7, 0.8)
        box(vertices, indices, 0, 1.55, 0, 1.2, 0.12, 0.9)
        box(vertices, indices, 0, 1.8, 0, 0.12, 0.18, 0.12)
    return vertices, indices


def write_glb(path: Path, kind: str) -> None:
    vertices, indices = make_mesh(kind)
    positions = b"".join(struct.pack("<3f", *point) for point in vertices)
    normals = b"".join(struct.pack("<3f", 0.0, 1.0, 0.0) for _ in vertices)
    index_bytes = b"".join(struct.pack("<H", index) for index in indices)
    pos_offset = 0
    normal_offset = len(positions)
    index_offset = normal_offset + len(normals)
    binary = positions + normals + index_bytes
    binary += b"\0" * (-len(binary) % 4)
    document = {
        "asset": {"version": "2.0", "generator": "rok2-p12-t4-tavern-generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"name": kind.title(), "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2}]}],
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": pos_offset, "byteLength": len(positions), "target": 34962},
            {"buffer": 0, "byteOffset": normal_offset, "byteLength": len(normals), "target": 34962},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(index_bytes), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(vertices), "type": "VEC3"},
            {"bufferView": 1, "componentType": 5126, "count": len(vertices), "type": "VEC3"},
            {"bufferView": 2, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
        ],
    }
    json_chunk = json.dumps(document, separators=(",", ":")).encode()
    json_chunk += b" " * (-len(json_chunk) % 4)
    total = 12 + 8 + len(json_chunk) + 8 + len(binary)
    with path.open("wb") as output:
        output.write(struct.pack("<4sII", b"glTF", 2, total))
        output.write(struct.pack("<II", len(json_chunk), 0x4E4F534A))
        output.write(json_chunk)
        output.write(struct.pack("<II", len(binary), 0x004E4942))
        output.write(binary)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path("game/client-unreal/Content/Art/Generated/Tavern"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for kind in ("tavern", "chest"):
        write_glb(args.output_dir / f"{kind}.glb", kind)
        print(f"generated {args.output_dir / (kind + '.glb')}")


if __name__ == "__main__":
    main()
