#!/usr/bin/env python3
import csv
import os
import sys
from PIL import Image

CSV_PATH = "Map_nodes.csv"
TILE_DIR = os.path.expanduser("~/national/8")
OUT_DIR = "geographical"
TILE_SIZE = 256
ZOOM = 8
TILES_WIDE = 22
TILES_HIGH = 8
VIEWPORT_W = TILES_WIDE * TILE_SIZE
VIEWPORT_H = TILES_HIGH * TILE_SIZE

os.makedirs(OUT_DIR, exist_ok=True)

missing_tiles = []

with open(CSV_PATH, "r", encoding="utf-8-sig") as f:
    reader = csv.reader(f)
    header = next(reader)
    sub_header = next(reader)
    openstreetmapx_idx = header.index("openstreetmapx")
    openstreetmapy_idx = header.index("openstreetmapy")
    en_us_idx = sub_header.index("en_US")

    for row in reader:
        try:
            union_id = int(row[0])
        except (ValueError, IndexError):
            continue

        if union_id < 1 or union_id > 86:
            continue

        ox_str = row[openstreetmapx_idx].strip()
        oy_str = row[openstreetmapy_idx].strip()
        if not ox_str or not oy_str:
            print(f"Map {union_id}: missing OSM coordinates, skipping")
            continue

        ox = int(ox_str)
        oy = int(oy_str)
        name = row[en_us_idx].strip().replace(" ", "_")

        out_path = os.path.join(OUT_DIR, f"map{union_id}_{name}.png")
        if os.path.exists(out_path):
            print(f"Map {union_id} ({name}): already exists, skipping")
            continue

        canvas = Image.new("RGB", (VIEWPORT_W, VIEWPORT_H))

        all_found = True
        for ty in range(TILES_HIGH):
            for tx in range(TILES_WIDE):
                tile_x = (ox + tx) % 256
                tile_y = oy + ty
                tile_path = os.path.join(TILE_DIR, f"{tile_x}_{tile_y}.png")
                if not os.path.exists(tile_path):
                    all_found = False
                    missing_tiles.append(f"{tile_x}_{tile_y} (map{union_id})")
                    continue
                tile_img = Image.open(tile_path).convert("RGB")
                canvas.paste(tile_img, (tx * TILE_SIZE, ty * TILE_SIZE))

        if not all_found:
            print(f"Map {union_id} ({name}): some tiles missing, saved best-effort")

        canvas.save(out_path, "PNG")
        print(f"Map {union_id} ({name}): saved ({ox},{oy}) -> {out_path}")

if missing_tiles:
    print(f"\nMissing {len(missing_tiles)} tiles:")
    for mt in missing_tiles[:20]:
        print(f"  {mt}")
    if len(missing_tiles) > 20:
        print(f"  ... and {len(missing_tiles) - 20} more")
else:
    print("\nAll tiles found.")
