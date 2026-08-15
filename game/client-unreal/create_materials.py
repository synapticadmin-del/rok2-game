"""
ROK2 — توليد مواد المشروع الحقيقية (.uasset) بلا واجهة رسومية.

لماذا هذا السكربت موجود؟
  كل مباني اللعبة والسور والأرض كانت تُلوَّن عبر UMaterialInstanceDynamic
  مبنيّ فوق مادة محرك (DefaultMaterial / WorldGridMaterial). هاتان الماداتان
  لا تملكان أي VectorParameter، فـ SetVectorParameterValue("Color") لا يفعل
  شيئاً بصمت — وهذا سبب ظهور القلعة والسور بلا ألوان (رمادي/شبكة).
  الحل الوحيد الصحيح: مادة مشروع تملك بارامترات حقيقية.

المواد المولَّدة:
  /Game/Art/Materials/M_Rok2Base     — مادة مضاءة ببارامترات Color/Roughness/
                                        Metallic/EmissiveColor
  /Game/Art/Materials/M_Rok2Unlit    — نسخة Unlit للأرضية والأيقونات العالمية،
                                        تضمن ظهور اللون حتى لو انهارت الإضاءة
                                        على أجهزة أندرويد الضعيفة

التشغيل:
  UnrealEditor-Cmd.exe <project>.uproject -run=pythonscript
      -script="create_materials.py" -nullrhi -unattended -nosplash
"""
import unreal

PACKAGE_PATH = "/Game/Art/Materials"

log = unreal.log
warn = unreal.log_warning

mat_lib = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def create_material(name):
    """ينشئ مادة فارغة. لو كانت موجودة تُحذف أولاً كي لا تتضاعف التعبيرات
    عند إعادة تشغيل السكربت (idempotent بإعادة البناء لا بالتجاهل)."""
    full = "%s/%s" % (PACKAGE_PATH, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        log("[ROK2] المادة موجودة — سيُعاد بناؤها: %s" % full)
        unreal.EditorAssetLibrary.delete_asset(full)

    return asset_tools.create_asset(
        asset_name=name,
        package_path=PACKAGE_PATH,
        asset_class=unreal.Material,
        factory=unreal.MaterialFactoryNew(),
    )


def add_vector_param(mat, param_name, default, x, y):
    node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionVectorParameter, x, y
    )
    node.set_editor_property("parameter_name", param_name)
    node.set_editor_property("default_value", default)
    return node


def add_scalar_param(mat, param_name, default, x, y):
    node = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, x, y
    )
    node.set_editor_property("parameter_name", param_name)
    node.set_editor_property("default_value", default)
    return node


def build_lit(mat):
    """Color -> BaseColor، Roughness/Metallic قياسيان، EmissiveColor للإبراز."""
    color = add_vector_param(mat, "Color", unreal.LinearColor(0.8, 0.8, 0.8, 1.0), -400, -200)
    rough = add_scalar_param(mat, "Roughness", 0.65, -400, 60)
    metal = add_scalar_param(mat, "Metallic", 0.0, -400, 160)
    emis = add_vector_param(mat, "EmissiveColor", unreal.LinearColor(0, 0, 0, 1), -400, 280)

    mat_lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mat_lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mat_lib.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    mat_lib.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def build_unlit(mat):
    """Unlit: اللون يذهب إلى Emissive مباشرة — يظهر بلا أي إضاءة في المستوى."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = add_vector_param(mat, "Color", unreal.LinearColor(0.8, 0.8, 0.8, 1.0), -400, -100)
    mat_lib.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def apply_usage_flags(mat):
    """بدون هذه الأعلام يستبدل المحرك المادة بـ DefaultMaterial عند الاستخدام
    مع ISM/HISM في بناء مُطبَّق (packaged) — والسور والأرض والخريطة كلها HISM."""
    for flag in (
        "used_with_instanced_static_meshes",
        "used_with_static_lighting",
        "used_with_skeletal_mesh",
    ):
        try:
            mat.set_editor_property(flag, True)
        except Exception as exc:  # علم غير موجود في هذه النسخة — ليس قاتلاً
            warn("[ROK2] تعذّر ضبط %s: %s" % (flag, exc))


def main():
    unreal.EditorAssetLibrary.make_directory(PACKAGE_PATH)

    specs = [
        ("M_Rok2Base", build_lit),
        ("M_Rok2Unlit", build_unlit),
    ]

    created = []
    for name, builder in specs:
        mat = create_material(name)
        if not mat:
            warn("[ROK2] فشل إنشاء %s" % name)
            continue
        builder(mat)
        apply_usage_flags(mat)
        mat_lib.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
        created.append(name)
        log("[ROK2] أُنشئت المادة %s/%s" % (PACKAGE_PATH, name))

    log("=" * 58)
    log("[ROK2] مواد جاهزة: %s" % ", ".join(created))
    log("=" * 58)


main()
