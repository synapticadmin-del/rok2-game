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
# استيراد الأصول (GLB/WAV/PNG) — مُفوَّض إلى import_assets.py
# ذلك السكربت يعمل headless أيضاً، ويحافظ على شجرة المجلدات التي يتوقعها
# الكود (/Game/Audio/<civ>/music و /Game/Audio/sfx/<name>). المنطق القديم
# هنا كان يسطّح الشجرة فتتصادم ملفات music.wav الستة.
# ---------------------------------------------------------------------------
print(">>> Step 1b: Importing raw assets via import_assets.py...")
import os
_SCRIPT_DIR = os.path.dirname(os.path.abspath(
    globals().get("__file__", r"C:/Users/kayf/Desktop/rok2/game/client-unreal/setup_level.py")))
_importer = os.path.join(_SCRIPT_DIR, "import_assets.py")
if os.path.isfile(_importer):
    try:
        exec(compile(open(_importer, encoding="utf-8").read(), _importer, "exec"),
             {"__file__": _importer, "__name__": "rok2_importer"})
    except SystemExit:
        print("    [!] الاستيراد أبلغ عن فشل جزئي — راجع اسطر [ROK2] اعلاه.")
else:
    print("    [!] import_assets.py غير موجود — سيُستخدم الاحتياطي الهندسي.")

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
            dyn = unreal.KismetMaterialLibrary.create_dynamic_material_instance(plane, flat_mat)
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
