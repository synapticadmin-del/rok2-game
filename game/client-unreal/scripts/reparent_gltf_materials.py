"""ROK2 — يفكّ ارتباط مواد KayKit المستوردة بمحتوى ملحق Interchange.

المشكلة التي يحلّها:
  مستورد glTF يربط المادة المستوردة (hexagons_medieval1) بوالد داخل الملحق:
  /Interchange/gltf/MaterialInstances/MI_Default_Opaque. وذلك الملحق معلن
  SupportedTargetPlatforms = Win64/Linux/Mac فقط، فمحتواه غير قابل للكوك على
  أندرويد، ويفشل الكوك بـ:
    LogCook: Error: Content is missing from cook ... MI_Default_Opaque
  أي أن كل مجسمات KayKit (18 مبنى وعلماً وجبلاً) تسقط من حزمة الأندرويد.

الحل:
  مادة مشروع /Game/Art/Materials/M_Rok2Gltf تكرّر البارامترات التي يضبطها
  المستورد بنفس الأسماء (BaseColorTexture / BaseColorFactor / MetallicFactor /
  RoughnessFactor)، ثم نعيد ربط كل MaterialInstanceConstant تحت /Game والدها
  خارج /Game إليها. البارامترات تُطابَق بالاسم فيبقى النسيج ويبقى
  URok2ProceduralAssets::TintExistingMaterialOn قادراً على الصبغ عبر
  BaseColorFactor.

التشغيل:
  UnrealEditor-Cmd.exe Rok2.uproject -run=pythonscript
      -script="scripts/reparent_gltf_materials.py" -nullrhi -unattended -nosplash
"""
import os
import unreal

PACKAGE_PATH = "/Game/Art/Materials"
MASTER_NAME = "M_Rok2Gltf"
MASTER_PATH = "%s/%s" % (PACKAGE_PATH, MASTER_NAME)
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "reparent_gltf_report.txt")

_lines = []
mat_lib = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def report(text):
    _lines.append(text)
    unreal.log(text)


def build_master():
    """مادة مضاءة: BaseColorTexture × BaseColorFactor -> BaseColor، مع
    MetallicFactor و RoughnessFactor بنفس أسماء مستورد glTF."""
    if unreal.EditorAssetLibrary.does_asset_exist(MASTER_PATH):
        unreal.EditorAssetLibrary.delete_asset(MASTER_PATH)

    unreal.EditorAssetLibrary.make_directory(PACKAGE_PATH)
    material = asset_tools.create_asset(
        asset_name=MASTER_NAME,
        package_path=PACKAGE_PATH,
        asset_class=unreal.Material,
        factory=unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError("تعذّر إنشاء %s" % MASTER_PATH)

    texture = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -700, -220)
    texture.set_editor_property("parameter_name", "BaseColorTexture")
    # نسيج افتراضي من المحرك: بدونه تكون العينة بلا نسيج فترفض المادة الترجمة.
    default_texture = unreal.EditorAssetLibrary.load_asset(
        "/Engine/EngineResources/DefaultTexture.DefaultTexture")
    if default_texture:
        texture.set_editor_property("texture", default_texture)

    factor = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -700, 40)
    factor.set_editor_property("parameter_name", "BaseColorFactor")
    factor.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    multiply = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -380, -120)
    mat_lib.connect_material_expressions(texture, "RGB", multiply, "A")
    mat_lib.connect_material_expressions(factor, "", multiply, "B")
    mat_lib.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)

    metallic = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -380, 160)
    metallic.set_editor_property("parameter_name", "MetallicFactor")
    metallic.set_editor_property("default_value", 0.0)
    mat_lib.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

    roughness = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -380, 260)
    roughness.set_editor_property("parameter_name", "RoughnessFactor")
    roughness.set_editor_property("default_value", 0.5)
    mat_lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # بدون هذه الأعلام يستبدل المحرك المادة بـ DefaultMaterial على HISM في بناء
    # مُطبَّق، وكل مجسمات الخريطة والمدينة تُرسم كـ HISM.
    for flag in ("used_with_instanced_static_meshes", "used_with_static_lighting"):
        try:
            material.set_editor_property(flag, True)
        except Exception as exc:
            report("[ROK2] تعذّر ضبط %s: %s" % (flag, exc))

    mat_lib.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    report("[ROK2] مادة الأساس جاهزة: %s" % MASTER_PATH)
    return material


def reparent_instances(master):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Game"], force_rescan=True)

    changed = 0
    for data in registry.get_assets_by_path("/Game", recursive=True):
        asset = data.get_asset()
        if not isinstance(asset, unreal.MaterialInstanceConstant):
            continue
        parent = asset.get_editor_property("parent")
        parent_path = parent.get_path_name() if parent else ""
        if parent_path.startswith("/Game") or parent_path.startswith("/Engine"):
            continue

        asset.set_editor_property("parent", master)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
        changed += 1
        report("[ROK2] أُعيد ربط %s: %s -> %s" % (
            asset.get_path_name(), parent_path or "<none>", MASTER_PATH))

    report("[ROK2] مواد أُعيد ربطها: %d" % changed)
    return changed


def verify():
    """يتحقق أن لا مادة ولا فتحة ميش تشير إلى محتوى خارج /Game و /Engine."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    offenders = []
    for data in registry.get_assets_by_path("/Game", recursive=True):
        asset = data.get_asset()
        if isinstance(asset, unreal.MaterialInstanceConstant):
            parent = asset.get_editor_property("parent")
            path = parent.get_path_name() if parent else ""
            if path and not (path.startswith("/Game") or path.startswith("/Engine")):
                offenders.append("MI %s -> %s" % (asset.get_path_name(), path))
        elif isinstance(asset, unreal.StaticMesh):
            for index, entry in enumerate(asset.get_editor_property("static_materials")):
                mat = entry.get_editor_property("material_interface")
                if mat is None:
                    continue
                path = mat.get_path_name()
                if not (path.startswith("/Game") or path.startswith("/Engine")):
                    offenders.append("MESH %s slot %d -> %s" % (
                        asset.get_path_name(), index, path))

    for line in offenders:
        report("[ROK2] ما زال خارج نطاق الكوك: %s" % line)
    report("[ROK2] مراجع غير قابلة للكوك متبقية: %d" % len(offenders))
    return offenders


master = build_master()
reparent_instances(master)
remaining = verify()

with open(REPORT, "w", encoding="utf-8") as handle:
    handle.write(os.linesep.join(_lines))
unreal.log("[ROK2] التقرير: %s" % REPORT)
