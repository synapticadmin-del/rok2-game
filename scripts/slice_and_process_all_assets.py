import os
from PIL import Image, ImageFilter
import numpy as np

SHEET1_PATH = r"C:\Users\kayf\.gemini\antigravity\brain\ef02bab5-5a7f-40e0-af78-a152e918b133\.user_uploaded\media_1786668688162.png"
SHEET2_PATH = r"C:\Users\kayf\.gemini\antigravity\brain\ef02bab5-5a7f-40e0-af78-a152e918b133\.user_uploaded\media_1786669407542.png"

OUT_CITY_DIR = r"c:\Users\kayf\Desktop\rok2\game\client-unreal\Content\Art\CityBuildingIcons"
OUT_WORLD_DIR = r"c:\Users\kayf\Desktop\rok2\game\client-unreal\Content\Art\WorldMapIcons"

os.makedirs(OUT_CITY_DIR, exist_ok=True)
os.makedirs(OUT_WORLD_DIR, exist_ok=True)

def remove_white_bg(img, tolerance=25):
    img = img.convert("RGBA")
    data = np.array(img, dtype=np.float32)
    r, g, b, a = data[:, :, 0], data[:, :, 1], data[:, :, 2], data[:, :, 3]
    
    # Distance from pure white (255, 255, 255)
    dist = np.sqrt((255 - r)**2 + (255 - g)**2 + (255 - b)**2)
    
    # Alpha fade for smooth anti-aliased edge
    alpha = np.clip((dist - 10) / (tolerance + 1), 0.0, 1.0) * 255.0
    data[:, :, 3] = alpha.astype(np.uint8)
    
    res = Image.fromarray(data.astype(np.uint8), "RGBA")
    
    # Auto-crop tight bounding box
    bbox = res.getbbox()
    if bbox:
        # Add slight padding
        w, h = res.size
        pad = 8
        bbox = (max(0, bbox[0]-pad), max(0, bbox[1]-pad), min(w, bbox[2]+pad), min(h, bbox[3]+pad))
        res = res.crop(bbox)
    return res

def make_normal_map(img_rgba, strength=2.2):
    rgb = img_rgba.convert("RGB")
    gray = np.asarray(rgb.convert("L"), dtype=np.float32) / 255.0
    
    # Sobel kernels
    kernel_x = np.array([[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]], dtype=np.float32)
    kernel_y = np.array([[-1, -2, -1], [0, 0, 0], [1, 2, 1]], dtype=np.float32)
    
    # Fast padding convolution
    padded = np.pad(gray, 1, mode='edge')
    dx = np.zeros_like(gray)
    dy = np.zeros_like(gray)
    
    for i in range(3):
        for j in range(3):
            dx += padded[i:i+gray.shape[0], j:j+gray.shape[1]] * kernel_x[i, j]
            dy += padded[i:i+gray.shape[0], j:j+gray.shape[1]] * kernel_y[i, j]
            
    dx *= strength
    dy *= strength
    dz = np.ones_like(dx)
    
    norm = np.sqrt(dx**2 + dy**2 + dz**2)
    nx = dx / norm
    ny = dy / norm
    nz = dz / norm
    
    r = ((nx * 0.5 + 0.5) * 255).astype(np.uint8)
    g = (((-ny) * 0.5 + 0.5) * 255).astype(np.uint8)
    b = ((nz * 0.5 + 0.5) * 255).astype(np.uint8)
    
    alpha = np.array(img_rgba)[:, :, 3]
    normal_rgba = np.stack([r, g, b, alpha], axis=-1)
    return Image.fromarray(normal_rgba, "RGBA")

def make_emissive_mask(img_rgba, threshold=220):
    data = np.array(img_rgba)
    r, g, b, a = data[:, :, 0], data[:, :, 1], data[:, :, 2], data[:, :, 3]
    
    is_warm = (r > threshold) & (g > 130) & (b < 180)
    is_cyan = (b > threshold) & (g > 180)
    is_gold = (r > 230) & (g > 200) & (b < 120)
    
    emissive = np.zeros_like(r)
    emissive[(is_warm | is_cyan | is_gold) & (a > 50)] = 255
    
    # Save as RGBA
    emissive_rgba = np.stack([emissive, emissive, emissive, a], axis=-1)
    return Image.fromarray(emissive_rgba, "RGBA")

# 1. Process Sheet 1 (16 City Buildings: 4 rows x 4 cols)
CITY_NAMES = [
    # Row 0
    "city_hall_base_tier1",
    "barracks_base_tier1",
    "farm_base_tier1",
    "hospital_base_tier1",
    # Row 1
    "wall_base_tier1",
    "archery_range_base_tier1",
    "stable_base_tier1",
    "siege_workshop_base_tier1",
    # Row 2
    "academy_base_tier1",
    "storehouse_base_tier1",
    "civ_rome_hall_tier4",
    "civ_china_hall_tier4",
    # Row 3
    "civ_arabia_hall_tier4",
    "civ_egypt_hall_tier4",
    "civ_vikings_hall_tier4",
    "civ_japan_hall_tier4"
]

print("=== Processing Sheet 1 (Castle Buildings) ===")
sheet1 = Image.open(SHEET1_PATH)
s1_w, s1_h = sheet1.size
cell_w = s1_w / 4.0
cell_h = s1_h / 4.0

for row in range(4):
    for col in range(4):
        idx = row * 4 + col
        name = CITY_NAMES[idx]
        x0 = int(col * cell_w)
        y0 = int(row * cell_h)
        x1 = int((col + 1) * cell_w)
        y1 = int((row + 1) * cell_h)
        
        crop_img = sheet1.crop((x0, y0, x1, y1))
        albedo = remove_white_bg(crop_img)
        normal = make_normal_map(albedo)
        emissive = make_emissive_mask(albedo)
        
        albedo.save(os.path.join(OUT_CITY_DIR, f"T_{name}_D.png"), "PNG")
        normal.save(os.path.join(OUT_CITY_DIR, f"T_{name}_N.png"), "PNG")
        emissive.save(os.path.join(OUT_CITY_DIR, f"T_{name}_E.png"), "PNG")
        print(f"[OK] Saved Castle Building: {name} (Albedo, Normal, Emissive)")

# 2. Process Sheet 2 (8 World Map Assets: 2 rows x 4 cols)
WORLD_NAMES = [
    # Row 0
    "world_mountain_ridge_barrier",
    "world_mountain_pass_fortress",
    "world_resource_nodes_quad",
    "world_barbarian_fort_camp",
    # Row 1
    "world_lost_temple_throne_core",
    "world_holy_shrine_altar",
    "world_stone_gold_quarry_mine",
    "world_barbarian_keep_outpost"
]

print("\n=== Processing Sheet 2 (World Map Infrastructure) ===")
sheet2 = Image.open(SHEET2_PATH)
s2_w, s2_h = sheet2.size
w_cell_w = s2_w / 4.0
w_cell_h = s2_h / 2.0

for row in range(2):
    for col in range(4):
        idx = row * 4 + col
        name = WORLD_NAMES[idx]
        x0 = int(col * w_cell_w)
        y0 = int(row * w_cell_h)
        x1 = int((col + 1) * w_cell_w)
        y1 = int((row + 1) * w_cell_h)
        
        # Trim off bottom label text area (lower 12%)
        crop_h = int((y1 - y0) * 0.88)
        crop_img = sheet2.crop((x0, y0, x1, y0 + crop_h))
        albedo = remove_white_bg(crop_img)
        normal = make_normal_map(albedo)
        emissive = make_emissive_mask(albedo)
        
        albedo.save(os.path.join(OUT_WORLD_DIR, f"T_{name}_D.png"), "PNG")
        normal.save(os.path.join(OUT_WORLD_DIR, f"T_{name}_N.png"), "PNG")
        emissive.save(os.path.join(OUT_WORLD_DIR, f"T_{name}_E.png"), "PNG")
        print(f"[OK] Saved World Asset: {name} (Albedo, Normal, Emissive)")

print("\n=== Slicing, Alpha Extraction, Normal & Emissive Map Generation Completed! ===")
