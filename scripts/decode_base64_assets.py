#!/usr/bin/env python3
"""
Decode base64-encoded binary assets in the rok2-game repo (in-place).

Why: WAV/PNG/GLB assets were committed as base64 TEXT (starts with e.g. 'UklGR...'
instead of binary 'RIFF'). Unreal cannot import base64 text files. The project's
setup_level.py works around this at import time by decoding to a side file with a
'.bin' suffix — which Unreal then rejects ('unknown extension bin'). This script
fixes the root cause: it decodes each base64 file IN PLACE, keeping the original
filename/extension, so plain imports work and no runtime workaround is needed.

Usage:
  python3 decode_base64_assets.py <content_root> [--dry-run]

Example (on the user's machine after git pull):
  python3 decode_base64_assets.py "C:/Users/kayf/Desktop/rok2/game/client-unreal/Content"

Safety:
  - Only touches files whose decoded bytes start with a known magic (RIFF, PNG, glTF, OggS).
  - Skips files that are already valid binary.
  - Creates a one-time .b64bak backup next to each decoded file unless --no-backup.
"""
import argparse
import base64
import binascii
import os
import sys

MAGICS = {
    b"RIFF": "wav",
    b"\x89PNG": "png",
    b"glTF": "glb/gltf-binary",
    b"OggS": "ogg",
    b"\xff\xd8\xff": "jpg",
    b"ID3": "mp3",
    b"fLaC": "flac",
}


def sniff_magic(data: bytes):
    for magic, name in MAGICS.items():
        if data.startswith(magic):
            return magic, name
    return None, None


def looks_like_base64(raw: bytes) -> bool:
    """Heuristic: ASCII-only, plausible b64 alphabet, length multiple of 4."""
    sample = raw[:4096].strip()
    if not sample:
        return False
    try:
        sample.decode("ascii")
    except UnicodeDecodeError:
        return False
    allowed = set(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=\r\n")
    if any(c not in allowed for c in sample):
        return False
    return True


def process_file(path: str, dry_run: bool, backup: bool) -> str:
    with open(path, "rb") as fh:
        raw = fh.read()

    # Already valid binary? leave it.
    magic, name = sniff_magic(raw)
    if magic:
        return f"skip (already {name} binary): {path}"

    if not looks_like_base64(raw):
        return f"skip (not base64-looking): {path}"

    try:
        decoded = base64.b64decode(raw, validate=False)
    except (binascii.Error, ValueError):
        return f"skip (b64 decode failed): {path}"

    magic, name = sniff_magic(decoded)
    if not magic:
        return f"skip (decoded has no known magic): {path}"

    if dry_run:
        return f"WOULD decode ({name}, {len(raw)} -> {len(decoded)} bytes): {path}"

    if backup:
        bak = path + ".b64bak"
        if not os.path.exists(bak):
            with open(bak, "wb") as fh:
                fh.write(raw)

    with open(path, "wb") as fh:
        fh.write(decoded)
    return f"decoded ({name}, {len(raw)} -> {len(decoded)} bytes): {path}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", help="Content root to scan recursively")
    ap.add_argument("--dry-run", action="store_true", help="Report only, don't write")
    ap.add_argument("--no-backup", action="store_true", help="Don't write .b64bak backups")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2

    exts = {".wav", ".png", ".glb", ".gltf", ".ogg", ".jpg", ".jpeg", ".mp3", ".flac", ".bin"}
    n_decoded = n_skipped = 0
    for dirpath, _dirs, files in os.walk(root):
        for fname in sorted(files):
            if fname.endswith(".b64bak"):
                continue
            ext = os.path.splitext(fname)[1].lower()
            # check extension, but also allow extensionless files if they sniff as b64
            if ext and ext not in exts:
                continue
            result = process_file(os.path.join(dirpath, fname), args.dry_run, not args.no_backup)
            print(result)
            if result.startswith("decoded") or result.startswith("WOULD"):
                n_decoded += 1
            else:
                n_skipped += 1

    print(f"\nDone. decoded={n_decoded} skipped={n_skipped}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
