"""
ROK2 - 2.5D Isometric Sprite & Normal Map Processor.
Pipeline C: Converts 2.5D building renders into UE5-ready Albedo (RGBA), Normal Maps, and Emissive Masks.
"""

import os
import math
from PIL import Image, ImageFilter, ImageOps
import numpy as np

def generate_normal_map(img_rgb, strength=2.5):
    """
    Computes a Tangent Space Normal Map from an RGB image using Sobel gradient filters.
    Output: RGB image where (128, 128, 255) is flat, and channels encode surface slopes.
    """
    gray = img_rgb.convert('L')
    gray_arr = np.asarray(gray, dtype=np.float32) / 255.0

    # Sobel kernels
    sobel_x = np.array([[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]], dtype=np.float32)
    sobel_y = np.array([[-1, -2, -1], [0, 0, 0], [1, 2, 1]], dtype=np.float32)

    # Convolve
    from scipy.ndimage import convolve
    dx = convolve(gray_arr, sobel_x) * strength
    dy = convolve(gray_arr, sobel_y) * strength
    dz = np.ones_like(dx)

    # Normalize vectors (dx, dy, dz)
    norm = np.sqrt(dx**2 + dy**2 + dz**2)
    nx = dx / norm
    ny = dy / norm
    nz = dz / norm

    # Map from [-1, 1] to [0, 255] (Unreal Engine expects Inverted Green for DirectX if needed)
    r = ((nx * 0.5 + 0.5) * 255).astype(np.uint8)
    g = (((-ny) * 0.5 + 0.5) * 255).astype(np.uint8)
    b = ((nz * 0.5 + 0.5) * 255).astype(np.uint8)

    normal_rgb = np.stack([r, g, b], axis=-1)
    return Image.fromarray(normal_rgb)

def generate_emissive_mask(img_rgb, threshold=220):
    """
    Extracts bright spots (torches, glowing crystals, lanterns) as an Emissive Mask.
    """
    r, g, b = img_rgb.split()
    r_arr = np.asarray(r, dtype=np.uint8)
    g_arr = np.asarray(g, dtype=np.uint8)
    b_arr = np.asarray(b, dtype=np.uint8)

    # Detect warm glows (High R, medium-high G, lower B) or bright cyan crystals
    is_torch = (r_arr > threshold) & (g_arr > 140) & (b_arr < 180)
    is_cyan = (b_arr > threshold) & (g_arr > 180)

    emissive_arr = np.zeros_like(r_arr, dtype=np.uint8)
    emissive_arr[is_torch | is_cyan] = 255

    return Image.fromarray(emissive_arr)

def remove_white_background(img_rgb, tolerance=15):
    """
    Converts solid white/near-white backgrounds into transparent Alpha channel.
    """
    img_rgba = img_rgb.convert('RGBA')
    data = np.array(img_rgba)
    r, g, b, a = data[:, :, 0], data[:, :, 1], data[:, :, 2], data[:, :, 3]

    # White mask
    is_white = (r >= 255 - tolerance) & (g >= 255 - tolerance) & (b >= 255 - tolerance)
    data[:, :, 3] = np.where(is_white, 0, 255)

    return Image.fromarray(data)

def process_building_asset(source_image_path, output_dir, asset_name):
    """
    Processes a cropped building image into Albedo (RGBA), Normal Map, and Emissive Mask.
    """
    os.makedirs(output_dir, exist_ok=True)
    img = Image.open(source_image_path)
    img_rgb = img.convert('RGB')

    # 1. Albedo with Alpha
    albedo = remove_white_background(img_rgb)
    albedo_path = os.path.join(output_dir, f"T_{asset_name}_D.png")
    albedo.save(albedo_path, "PNG")

    # 2. Normal Map (try with scipy, fallback to gradient if scipy not installed)
    try:
        normal = generate_normal_map(img_rgb)
        # Apply alpha mask from albedo
        normal_rgba = normal.convert('RGBA')
        normal_data = np.array(normal_rgba)
        normal_data[:, :, 3] = np.array(albedo)[:, :, 3]
        normal_final = Image.fromarray(normal_data)
        normal_path = os.path.join(output_dir, f"T_{asset_name}_N.png")
        normal_final.save(normal_path, "PNG")
    except Exception as e:
        print(f"Normal map fallback for {asset_name}: {e}")

    # 3. Emissive Mask
    emissive = generate_emissive_mask(img_rgb)
    emissive_path = os.path.join(output_dir, f"T_{asset_name}_E.png")
    emissive.save(emissive_path, "PNG")

    print(f"✓ Processed {asset_name} -> Albedo, Normal, Emissive")

if __name__ == '__main__':
    print("ROK2 2.5D Isometric Asset Processor ready.")
