# Run inside the editor (-run=pythonscript -script="refgen_expected.py <out file>").
# Computes reference values of the generated RefGen assets with the engine itself, for parser tests, as JSON:
#   {"engine": ..., "assets": {object path: class}, "values": {kind: {object path: value | {key: value}}}, "notes": [...]}
# Vectors are [x, y, z], quaternions [x, y, z, w], colors [r, g, b, a], transforms {"t", "r", "s"}, objects their path.
# Anything the running engine version cannot compute goes to "notes", never guessed.
import json
import sys
import unreal

import os

# the output file: the script's argument, or REFGEN_EXPECTED_OUT (a space in -script="..." breaks UE's command line parsing)
OUT = sys.argv[-1] if len(sys.argv) > 1 else os.environ.get("REFGEN_EXPECTED_OUT", "expected.json")
values = {}
notes = []


def fmt(v):
    if isinstance(v, (bool, int, str)):
        return v
    if isinstance(v, float):
        return float("%.7g" % v)
    if isinstance(v, unreal.Vector):
        return [fmt(v.x), fmt(v.y), fmt(v.z)]
    if isinstance(v, unreal.Vector2D):
        return [fmt(v.x), fmt(v.y)]
    if isinstance(v, unreal.Quat):
        return [fmt(v.x), fmt(v.y), fmt(v.z), fmt(v.w)]
    if isinstance(v, unreal.Rotator):
        return [fmt(v.pitch), fmt(v.yaw), fmt(v.roll)]
    if isinstance(v, unreal.LinearColor):
        return [fmt(v.r), fmt(v.g), fmt(v.b), fmt(v.a)]
    if isinstance(v, unreal.Transform):
        rotation = v.rotation.quaternion() if hasattr(v.rotation, "quaternion") else v.rotation
        return {"t": fmt(v.translation), "r": fmt(rotation), "s": fmt(v.scale3d)}
    if isinstance(v, unreal.Object):
        return v.get_path_name()
    if isinstance(v, unreal.EnumBase):
        return v.name
    if isinstance(v, unreal.StructBase) and hasattr(v, "export_text"):
        return v.export_text()
    if isinstance(v, (list, tuple, unreal.Array, unreal.Set)):
        return [fmt(x) for x in v]
    if isinstance(v, dict):
        return {str(k): fmt(x) for k, x in v.items()}
    if isinstance(v, unreal.Map):
        return {str(k): fmt(x) for k, x in v.items()}
    return str(v)


def emit(kind, path, key, *items):
    value = fmt(items[0]) if len(items) == 1 else [fmt(x) for x in items]
    if key == "-":
        values.setdefault(kind, {})[path] = value
    else:
        values.setdefault(kind, {}).setdefault(path, {})[str(key)] = value


def note(text):
    notes.append(str(text))


def guarded(what, fn, *args):
    try:
        fn(*args)
    except Exception as e:
        note("%s: %s: %s" % (what, type(e).__name__, e))


def class_name(data):
    path = getattr(data, "asset_class_path", None)
    return str(path.asset_name) if path is not None else str(data.asset_class)


def object_path(data):
    return "%s.%s" % (data.package_name, data.asset_name)


def times(length):
    return [0.0, length * 0.25, length * 0.5, length * 0.75, length]


def bone_names(seq):
    skeleton = seq.get_editor_property("skeleton")
    names = []
    if hasattr(unreal, "AnimPoseExtensions"):
        pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
        names = list(unreal.AnimPoseExtensions.get_bone_names(pose))
    if not names:
        names = list(unreal.AnimationLibrary.get_animation_track_names(seq))
    return names


def do_sequence(path, seq):
    length = seq.get_play_length() if hasattr(seq, "get_play_length") else seq.get_editor_property("sequence_length")
    emit("anim.length", path, "-", float(length))
    names = bone_names(seq)
    for t in times(length):
        poses = unreal.AnimationLibrary.get_bone_poses_for_time(seq, names, t, False)
        for name, pose in zip(names, poses):
            emit("anim.pose", path, "%s@%s" % (name, fmt(t)), pose)
    for curve in unreal.AnimationLibrary.get_animation_curve_names(seq, unreal.RawCurveTrackTypes.RCT_FLOAT):
        for t in times(length):
            value = unreal.AnimationLibrary.get_float_value_at_time(seq, curve, t) if hasattr(unreal.AnimationLibrary, "get_float_value_at_time") else None
            if value is not None:
                emit("anim.curve", path, "%s@%s" % (curve, fmt(t)), value)


def do_montage(path, montage):
    emit("montage.length", path, "-", float(montage.get_play_length()))
    for i in range(montage.get_num_sections()):
        name = montage.get_section_name(i)
        emit("montage.section", path, i, name)


def do_curve(path, curve):
    for t in (-1.0, 0.0, 0.25, 0.5, 0.75, 1.0, 2.0):
        if isinstance(curve, unreal.CurveFloat):
            emit("curve.float", path, fmt(t), curve.get_float_value(t))
        elif isinstance(curve, unreal.CurveVector):
            emit("curve.vector", path, fmt(t), curve.get_vector_value(t))
        elif isinstance(curve, unreal.CurveLinearColor):
            emit("curve.color", path, fmt(t), curve.get_unadjusted_linear_color_value(t))


def do_curve_table(path, table):
    lib = unreal.DataTableFunctionLibrary
    for row in lib.get_curve_table_row_names(table) if hasattr(lib, "get_curve_table_row_names") else []:
        for x in (0.0, 1.0, 2.5, 5.0, 10.0):
            result = lib.evaluate_curve_table_row(table, row, x, "")
            value = result[-1] if isinstance(result, tuple) else result
            emit("curvetable", path, "%s@%s" % (row, fmt(x)), value)


def do_data_table(path, table):
    lib = unreal.DataTableFunctionLibrary
    rows = [str(r) for r in lib.get_data_table_row_names(table)]
    emit("datatable.rows", path, "-", *rows)
    for column in lib.get_data_table_column_names(table):
        values = lib.get_data_table_column_as_string(table, column)
        emit("datatable.column", path, column, *values)


def engine_version():
    major, minor = unreal.SystemLibrary.get_engine_version().split("-")[0].split(".")[:2]
    return int(major), int(minor)


def overrides_static_switch(material, name):
    while isinstance(material, unreal.MaterialInstance):
        parameters = material.get_editor_property("static_parameters")
        for switch in parameters.get_editor_property("static_switch_parameters"):
            info = switch.get_editor_property("parameter_info")
            if str(info.get_editor_property("name")) == str(name) and switch.get_editor_property("override"):
                return True
        material = material.get_editor_property("parent")
    return False


# UE 4.25 to 5.1 keep a material's static switch defaults only in the editor: the cooked shader map id holds a hash of them, and the
# cached expression data leaves them out. Up to 5.0 a cooked value exists only where a material instance of the chain overrides the
# switch, in its StaticParameters. UE 5.1 keeps the instance's switches in editor-only data too, so its cook has no switch values.
def cooks_static_switch(material, name):
    version = engine_version()
    if not (4, 25) <= version < (5, 2):
        return True
    if version == (5, 1):
        return False
    try:
        return overrides_static_switch(material, name)
    except Exception as e:
        note("material.switch %s %s: cannot tell whether an instance overrides it: %s: %s" % (material.get_path_name(), name, type(e).__name__, e))
        return False


def do_material(path, material):
    lib = unreal.MaterialEditingLibrary
    instance = isinstance(material, unreal.MaterialInstance)
    for name in lib.get_scalar_parameter_names(material):
        value = lib.get_material_instance_scalar_parameter_value(material, name) if instance else lib.get_material_default_scalar_parameter_value(material, name)
        emit("material.scalar", path, name, value)
    for name in lib.get_vector_parameter_names(material):
        value = lib.get_material_instance_vector_parameter_value(material, name) if instance else lib.get_material_default_vector_parameter_value(material, name)
        emit("material.vector", path, name, value)
    for name in lib.get_texture_parameter_names(material):
        value = lib.get_material_instance_texture_parameter_value(material, name) if instance else lib.get_material_default_texture_parameter_value(material, name)
        emit("material.texture", path, name, value)
    for name in lib.get_static_switch_parameter_names(material):
        if not cooks_static_switch(material, name):
            continue
        value = lib.get_material_instance_static_switch_parameter_value(material, name) if instance else lib.get_material_default_static_switch_parameter_value(material, name)
        emit("material.switch", path, name, value)
    if instance:
        emit("material.parent", path, "-", material.get_editor_property("parent"))
    else:
        emit("material.blend", path, "-", material.get_editor_property("blend_mode"))
        emit("material.domain", path, "-", material.get_editor_property("material_domain"))


def do_static_mesh(path, mesh):
    bounds = mesh.get_bounds()
    emit("mesh.bounds", path, "-", bounds.origin, bounds.box_extent, float(bounds.sphere_radius))
    emit("mesh.lods", path, "-", mesh.get_num_lods())
    for lod in range(mesh.get_num_lods()):
        counts = {"sections": mesh.get_num_sections(lod)}
        if hasattr(mesh, "get_num_vertices"):
            counts.update(vertices=mesh.get_num_vertices(lod), triangles=mesh.get_num_triangles(lod))
        elif lod == 0:
            note("%s: this engine's Python has no static mesh vertex counts" % path)
        emit("mesh.lod", path, lod, counts)
    if hasattr(mesh, "get_num_nanite_triangles"):
        emit("mesh.nanite", path, "-", mesh.get_num_nanite_vertices(), mesh.get_num_nanite_triangles())
    emit("mesh.materials", path, "-", *[m.get_editor_property("material_interface") for m in mesh.get_editor_property("static_materials")])


def do_skeletal_mesh(path, mesh):
    bounds = mesh.get_bounds()
    emit("skmesh.bounds", path, "-", bounds.origin, bounds.box_extent, float(bounds.sphere_radius))
    emit("skmesh.skeleton", path, "-", mesh.get_editor_property("skeleton"))
    emit("skmesh.materials", path, "-", *[m.get_editor_property("material_interface") for m in mesh.get_editor_property("materials")])


def do_skeleton(path, skeleton):
    if hasattr(unreal, "AnimPoseExtensions"):
        pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
        for name in unreal.AnimPoseExtensions.get_bone_names(pose):
            emit("skeleton.bone", path, name, unreal.AnimPoseExtensions.get_ref_bone_pose(pose, name, unreal.AnimPoseSpaces.LOCAL))


def do_texture(path, texture):
    if isinstance(texture, unreal.Texture2D):
        emit("texture.size", path, "-", texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    emit("texture.settings", path, "-", texture.get_editor_property("compression_settings"), texture.get_editor_property("srgb"))


def do_sound(path, sound):
    emit("sound.duration", path, "-", float(sound.get_editor_property("duration")))
    if isinstance(sound, unreal.SoundWave):
        for key in ("num_channels", "sample_rate", "total_samples"):
            try:
                emit("sound." + key, path, "-", sound.get_editor_property(key))
            except Exception:
                pass


BP_VARIABLES = ("Health", "Ammo", "Label", "Title", "Id", "Offset", "Spawn", "Tint", "State", "Data", "NativeData",
                "MeshAsset", "SoftMesh", "SpawnClass", "Scores", "RefTags", "Weights", "Row")


def do_blueprint(path, data):
    generated = unreal.load_object(None, "%s.%s_C" % (data.package_name, data.asset_name))
    if generated is None:
        return
    cdo = unreal.get_default_object(generated)
    for name in BP_VARIABLES:
        try:
            emit("bp.default", path + "_C", name, cdo.get_editor_property(name))
        except Exception:
            pass


def world():
    if hasattr(unreal, "UnrealEditorSubsystem"):
        return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    return unreal.EditorLevelLibrary.get_editor_world()  # UE4


def do_function_library(path, data):
    generated = unreal.load_object(None, "%s.%s_C" % (data.package_name, data.asset_name))
    cdo = unreal.get_default_object(generated)
    for value, factor in ((3.0, 2.5), (-1.5, 4.0), (0.0, 7.0)):
        emit("bp.call", path + "_C", "ScaleValue(%s,%s)" % (fmt(value), fmt(factor)), cdo.call_method("ScaleValue", (value, factor, world())))


def do_level(path, data):
    unreal.EditorLoadingAndSavingUtils.load_map(str(data.package_name))
    if hasattr(unreal, "EditorActorSubsystem"):
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    else:
        actors = unreal.EditorLevelLibrary.get_all_level_actors()  # UE4
    for actor in sorted(actors, key=lambda a: a.get_name()):
        emit("level.actor", path, actor.get_name(), actor.get_class().get_path_name(), actor.get_actor_transform())


HANDLERS = {
    "AnimSequence": do_sequence, "AnimMontage": do_montage, "CurveFloat": do_curve, "CurveVector": do_curve,
    "CurveLinearColor": do_curve, "CurveTable": do_curve_table, "DataTable": do_data_table, "Material": do_material,
    "MaterialInstanceConstant": do_material, "StaticMesh": do_static_mesh, "SkeletalMesh": do_skeletal_mesh,
    "Skeleton": do_skeleton, "Texture2D": do_texture, "TextureCube": do_texture, "TextureRenderTarget2D": do_texture,
    "SoundWave": do_sound, "SoundCue": do_sound,
}
DATA_HANDLERS = {"Blueprint": do_blueprint, "World": do_level}

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets_out = {}
assets = sorted(registry.get_assets_by_path("/Game", True), key=lambda d: str(d.package_name))
for data in assets:
    if "/__ExternalActors__/" in str(data.package_name) or "/__ExternalObjects__/" in str(data.package_name):
        continue
    assets_out[object_path(data)] = class_name(data)
levels = []
for data in assets:
    kind, path = class_name(data), object_path(data)
    if kind == "World":
        levels.append(data)
        continue
    if kind == "Blueprint":
        guarded(path, do_blueprint, path, data)
        if str(data.asset_name).startswith("BFL_"):
            guarded(path, do_function_library, path, data)
        continue
    handler = HANDLERS.get(kind)
    if handler is not None:
        obj = unreal.load_asset(str(data.package_name))
        guarded(path, handler, path, obj)
for data in levels:
    if str(data.asset_name) == "L_Ref":
        guarded(object_path(data), do_level, object_path(data), data)

with open(OUT, "w") as f:
    json.dump({"engine": unreal.SystemLibrary.get_engine_version(), "assets": assets_out, "values": values, "notes": notes},
              f, indent=1, sort_keys=True)
print("RefGen expected values: %d kinds, %d notes -> %s" % (len(values), len(notes), OUT))
