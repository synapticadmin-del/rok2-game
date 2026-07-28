"""
ROK2 Level Setup Script — Fully Cleans Level & Kills Viewport Noise via Live Console Commands.

HOW TO RUN IN UNREAL EDITOR:
  1. Open Output Log at bottom.
  2. Select "Python" tab (not Cmd).
  3. Run:
     exec(open("C:/Users/kayf/Desktop/rok2/game/client-unreal/setup_level.py").read())
"""
import unreal

editor_subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_subsys = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

world = unreal.EditorLevelLibrary.get_editor_world()

print(">>> Step 1: Executing GPU Noise-Fix Console Commands...")
# Turn off all post process, TAA, TSR, and shadow map dithering live in the editor viewport
console_cmds = [
    "r.ShadowQuality 0",
    "r.Shadow.Virtual.Enable 0",
    "r.Shadow.CSM.MaxCascades 0",
    "r.DefaultFeature.AntiAliasing 0",
    "r.PostProcessAAQuality 0",
    "r.TonemapperQuality 0",
    "r.DefaultFeature.AutoExposure 0",
    "r.EyeAdaptationQuality 0",
    "r.BloomQuality 0",
    "r.MotionBlurQuality 0",
    "r.DepthOfFieldQuality 0",
    "r.SSR.Quality 0",
    "r.SSGI.Quality 0",
    "r.VolumetricFog 0",
    "r.SceneColorFormat 3",
    "viewmode lit"
]

for cmd in console_cmds:
    if world:
        unreal.SystemLibrary.execute_console_command(world, cmd)

# ---------------------------------------------------------------------------
# P2-T7: استيراد أصول KayKit (GLB) إلى /Game/Art/kaykit كـ uasset
# — يتخطى الموجود، ولا يؤثر على البناء بدون استيراد (fallback هندسي).
# ---------------------------------------------------------------------------
print(">>> Step 1b: Importing KayKit GLB art assets (P2-T7)...")
import os, base64
ART_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Content", "Art", "kaykit")
ART_DST = "/Game/Art/kaykit"

def _ensure_binary_glb(path):
    """يفك ترميز GLB المخزّن نصياً (base64) إلى binary قبل الاستيراد — P2-T7 fix.
    بعض البيئات تخزّن GLB كنص base64؛ Unreal يحتاج binary حقيقي يبدأ بـ glTF."""
    try:
        with open(path, "rb") as fh:
            head = fh.read(4)
        if head == b"glTF":
            return path  # binary سليم
        with open(path, "rb") as fh:
            raw = fh.read()
        decoded = base64.b64decode(raw, validate=True)
        if decoded[:4] != b"glTF":
            return path  # ليس base64 GLB — اتركه كما هو
        fixed = path + ".bin.glb"
        with open(fixed, "wb") as fh:
            fh.write(decoded)
        return fixed
    except Exception:
        return path

if os.path.isdir(ART_SRC):
    tasks = []
    for fname in sorted(os.listdir(ART_SRC)):
        if not fname.lower().endswith(".glb"):
            continue
        dest_name = fname[:-4]
        # تخطَّ المستورد مسبقاً
        if unreal.EditorAssetLibrary.does_asset_exist(f"{ART_DST}/{dest_name}.{dest_name}"):
            continue
        task = unreal.AssetImportTask()
        task.filename = _ensure_binary_glb(os.path.join(ART_SRC, fname))
        task.destination_path = ART_DST
        task.destination_name = dest_name
        task.replace_existing = False
        task.automated = True
        task.save = True
        tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    print(f"    imported {len(tasks)} GLB assets (skipped existing).")
else:
    print("    no kaykit folder found — geometric fallback stays active.")

# ---------------------------------------------------------------------------
# P4-T2: فك ترميز الأصول الثنائية المخزنة base64 (WAV/PNG) ثم استيرادها:
#   Content/Audio/<civ>/music.wav + Content/Audio/sfx/*.wav  → /Game/Audio/...
#   Content/Art/Commanders/<id>.png                          → /Game/Art/Commanders
# ---------------------------------------------------------------------------
print(">>> Step 1c: Decoding + importing audio (WAV) and commander portraits (PNG) — P4-T2...")
CONTENT_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Content")

def _ensure_binary(path, magic):
    """يفك base64 إن لزم (نفس نمط _ensure_binary_glb) ويعيد مساراً binary صالحاً."""
    try:
        with open(path, "rb") as fh:
            head = fh.read(len(magic))
        if head == magic:
            return path
        with open(path, "rb") as fh:
            raw = fh.read()
        decoded = base64.b64decode(raw, validate=True)
        if decoded[: len(magic)] != magic:
            return path
        fixed = path + ".bin"
        with open(fixed, "wb") as fh:
            fh.write(decoded)
        return fixed
    except Exception:
        return path

def _import_tree(src_root, dst_root, exts_magics):
    """يستورد كل ملفات exts من src_root إلى dst_root (متكرر، يتخطى الموجود)."""
    imported = 0
    if not os.path.isdir(src_root):
        return 0
    for dirpath, _dirs, files in os.walk(src_root):
        for fname in sorted(files):
            ext = os.path.splitext(fname)[1].lower()
            magic = exts_magics.get(ext)
            if not magic:
                continue
            dest_name = os.path.splitext(fname)[0]
            if unreal.EditorAssetLibrary.does_asset_exist(f"{dst_root}/{dest_name}.{dest_name}"):
                continue
            task = unreal.AssetImportTask()
            task.filename = _ensure_binary(os.path.join(dirpath, fname), magic)
            task.destination_path = dst_root
            task.destination_name = dest_name
            task.replace_existing = False
            task.automated = True
            task.save = True
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            imported += 1
    return imported

_n_audio = _import_tree(os.path.join(CONTENT_ROOT, "Audio"), "/Game/Audio", {".wav": b"RIFF"})
_n_portraits = _import_tree(os.path.join(CONTENT_ROOT, "Art", "Commanders"), "/Game/Art/Commanders", {".png": b"\x89PNG"})
print(f"    imported {_n_audio} WAV + {_n_portraits} commander portraits (skipped existing).")

print(">>> Step 2: Cleaning ALL old duplicated actors from level...")
all_actors = editor_subsys.get_all_level_actors()
deleted_count = 0
for actor in all_actors:
    label = actor.get_actor_label()
    if label.startswith("ROK2_") or "Cube" in label or "Plane" in label:
        editor_subsys.destroy_actor(actor)
        deleted_count += 1
print(f"    OK: Deleted {deleted_count} old duplicate actors.")

# --- Step 3: Spawn single Directional Light with ZERO shadows ---
print(">>> Step 3: Spawning Directional Light (No Shadows)...")
sun = editor_subsys.spawn_actor_from_class(
    unreal.DirectionalLight,
    unreal.Vector(0, 0, 1000)
)
if sun:
    sun.set_actor_rotation(unreal.Rotator(-60, -30, 0), False)
    light_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if light_comp:
        light_comp.set_intensity(5.0)
        light_comp.set_light_color(unreal.LinearColor(1.0, 0.98, 0.92, 1.0))
        # Turn off shadows completely to eliminate Intel HD 530 shadow dithering
        light_comp.set_editor_property("cast_shadows", False)
    sun.set_actor_label("ROK2_Sun")
    print("    OK: Sun created (shadows disabled).")

# --- Step 4: Spawn Ambient Sky Light ---
print(">>> Step 4: Spawning Sky Light...")
sky = editor_subsys.spawn_actor_from_class(
    unreal.SkyLight,
    unreal.Vector(0, 0, 1200)
)
if sky:
    sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp:
        sky_comp.set_intensity(3.0)
        sky_comp.set_editor_property("cast_shadows", False)
    sky.set_actor_label("ROK2_SkyLight")
    print("    OK: Sky Light created.")

# --- Step 5: Spawn Ground Plane ---
print(">>> Step 5: Spawning Ground Surface...")
plane = editor_subsys.spawn_actor_from_class(
    unreal.StaticMeshActor,
    unreal.Vector(0, 0, -50)
)
if plane:
    mesh_comp = plane.get_component_by_class(unreal.StaticMeshComponent)
    if mesh_comp:
        cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
        if cube_mesh:
            mesh_comp.set_static_mesh(cube_mesh)
        mesh_comp.set_world_scale3d(unreal.Vector(1500, 1500, 0.1))
        mesh_comp.set_editor_property("cast_shadow", False)
        mesh_comp.set_editor_property("receives_decals", False)
        
        # Apply simple solid green material
        flat_mat = unreal.load_asset("/Engine/EngineMaterials/DefaultMaterial")
        if flat_mat:
            dyn = unreal.MaterialInstanceDynamic.create(flat_mat, plane)
            if dyn:
                dyn.set_vector_parameter_value("BaseColor", unreal.LinearColor(0.15, 0.45, 0.18, 1.0))
                mesh_comp.set_material(0, dyn)
    plane.set_actor_label("ROK2_Ground")
    print("    OK: Clean Ground Plane created.")

# --- Step 6: Spawn Player Start ---
print(">>> Step 6: Spawning Player Start...")
ps = editor_subsys.spawn_actor_from_class(
    unreal.PlayerStart,
    unreal.Vector(0, 0, 100)
)
if ps:
    ps.set_actor_label("ROK2_PlayerStart")
    print("    OK: Player Start created.")

# --- Step 7: Save level ---
print(">>> Step 7: Saving level...")
level_subsys.save_current_level()

print("")
print("=" * 60)
print("=== SUCCESS: GPU CONSOLE FIX APPLIED & LEVEL READY! ===")
print("=== PRESS PLAY ▶️ (زر التشغيل في الأعلى) TO TEST! ===")
print("=" * 60)
