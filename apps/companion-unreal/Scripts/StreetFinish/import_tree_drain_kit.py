"""Explicit native import of original tree/drain assets; never modifies a street map.

Run through the existing PID-bound bridge only after unrelated dirty packages
are resolved. No auto collision hulls: they would fill open drain interiors.
"""
import hashlib
import json
import os
from pathlib import Path

import unreal as u


PROJECT = Path("C:/dev/cribtest-game/apps/companion-unreal")
SOURCE_ROOT = PROJECT.parents[1] / "output/unreal-street-detail-20261014/tree-drain-kit"
DESTINATION = "/Game/BaddieGeneratedWorld/StreetFinish_20261014/TreeDrainKit"
OWNER = "Baddie.TreeDrainKit.20261014"


def run(expected_pid, source_directory):
    source = Path(source_directory).resolve()
    if os.getpid() != expected_pid or Path(u.Paths.project_dir()).resolve() != PROJECT:
        raise RuntimeError("Wrong editor identity")
    if not source.is_relative_to(SOURCE_ROOT.resolve()):
        raise ValueError("Source directory escapes tree/drain kit")
    if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError("Stop PIE before native import")
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError("Resolve unrelated dirty maps/materials first; importer will not save them")
    manifest = json.loads((source / "manifest.json").read_text())
    if (source / "native-import-receipt.json").exists():
        raise RuntimeError("Native import receipt already exists")
    if hashlib.sha256((source / "geometry.json").read_bytes()).hexdigest() != manifest["geometrySha256"]:
        raise ValueError("Geometry source digest mismatch")
    for asset in manifest["assets"]:
        package = DESTINATION + "/Meshes/" + asset["id"]
        if u.EditorAssetLibrary.does_asset_exist(package):
            raise RuntimeError("Immutable native asset exists; do not overwrite " + package)
        for lod in asset["lods"]:
            path = (source / lod["file"]).resolve()
            if path.parent != source or hashlib.sha256(path.read_bytes()).hexdigest() != lod["sha256"]:
                raise ValueError("Changed or escaping OBJ input")
    for name in manifest["materials"]:
        if u.EditorAssetLibrary.does_asset_exist(DESTINATION + "/Materials/M_TD_" + name):
            raise RuntimeError("Material already exists; immutable import requires a new namespace")
    tools = u.AssetToolsHelpers.get_asset_tools()
    mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    materials = {}
    for name, recipe in manifest["materials"].items():
        identifier = "M_TD_" + name
        if u.EditorAssetLibrary.does_asset_exist(DESTINATION + "/Materials/" + identifier):
            raise RuntimeError("Material already exists; immutable import requires a new namespace")
        material = tools.create_asset(identifier, DESTINATION + "/Materials", u.Material, u.MaterialFactoryNew())
        if material is None:
            raise RuntimeError("Material creation failed")
        material.set_editor_property("two_sided", recipe.get("twoSided", False))
        color = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionConstant3Vector, -300, 0)
        color.set_editor_property("constant", u.LinearColor(*recipe["color"], 1))
        u.MaterialEditingLibrary.connect_material_property(color, "", u.MaterialProperty.MP_BASE_COLOR)
        for offset, key, prop in ((150, "roughness", u.MaterialProperty.MP_ROUGHNESS),
                                  (270, "metallic", u.MaterialProperty.MP_METALLIC)):
            node = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionConstant, -300, offset)
            node.set_editor_property("r", recipe[key])
            u.MaterialEditingLibrary.connect_material_property(node, "", prop)
        u.EditorAssetLibrary.set_metadata_tag(material, "TreeDrainOwner", OWNER)
        materials["TD_" + name] = material
    receipt = {"pid": expected_pid, "geometrySha256": manifest["geometrySha256"], "assets": [],
               "placedInStreet": False, "renderedAcceptance": "pending", "gameplayCollision": "not admitted"}
    for asset in manifest["assets"]:
        task = u.AssetImportTask()
        task.set_editor_properties({"filename": str(source / asset["lods"][0]["file"]),
                                   "destination_path": DESTINATION + "/Meshes", "destination_name": asset["id"],
                                   "automated": True, "replace_existing": False, "save": False,
                                   "factory": u.FbxFactory()})
        options = u.FbxImportUI()
        options.set_editor_properties({"automated_import_should_detect_type": False, "import_mesh": True,
            "import_as_skeletal": False, "import_materials": False, "import_textures": False,
            "import_animations": False, "mesh_type_to_import": u.FBXImportType.FBXIT_STATIC_MESH})
        options.static_mesh_import_data.set_editor_properties({"combine_meshes": True, "convert_scene": False,
            "convert_scene_unit": False, "force_front_x_axis": False, "import_uniform_scale": 1.0,
            "build_nanite": False, "auto_generate_collision": False, "remove_degenerates": False,
            "generate_lightmap_u_vs": False,
            "normal_import_method": u.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS})
        task.set_editor_property("options", options)
        tools.import_asset_tasks([task])
        package = DESTINATION + "/Meshes/" + asset["id"]
        if [p.split(".")[0] for p in task.imported_object_paths] != [package]:
            raise RuntimeError("Unexpected imported asset paths")
        mesh = u.load_asset(package)
        bounds = mesh.get_bounding_box()
        for index, axis in enumerate(("x", "y", "z")):
            for observed, expected in ((getattr(bounds.min, axis), asset["lods"][0]["boundsCm"][0][index]),
                                       (getattr(bounds.max, axis), asset["lods"][0]["boundsCm"][1][index])):
                if abs(observed - expected) > 0.15:
                    raise RuntimeError("Native axis/bounds mismatch " + package)
        for level, lod in enumerate(asset["lods"][1:], 1):
            if mesh_editor.import_lod(mesh, level, str(source / lod["file"])) != level:
                raise RuntimeError("Native LOD import failed")
        for index, slot in enumerate(mesh.static_materials):
            name = str(slot.material_slot_name)
            if name not in materials:
                raise RuntimeError("Unexpected material slot " + name)
            mesh.set_material(index, materials[name])
        counts = [mesh.get_num_triangles(i) for i in range(len(asset["lods"]))]
        if counts != [lod["triangles"] for lod in asset["lods"]]:
            raise RuntimeError("Native triangle loss " + package)
        if len(asset["lods"]) == 3:
            mesh_editor.set_lod_screen_sizes(mesh, [1, 0.25, 0.1])
        if asset["kind"] == "drain-or-sidewalk":
            body = mesh.get_editor_property("body_setup")
            if body is None:
                raise RuntimeError("Missing structural mesh body setup " + package)
            body.set_editor_property("collision_trace_flag", u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        u.EditorAssetLibrary.set_metadata_tag(mesh, "TreeDrainOwner", OWNER)
        u.EditorAssetLibrary.set_metadata_tag(mesh, "TreeDrainSourceSHA256", asset["lods"][0]["sha256"])
        u.EditorAssetLibrary.set_metadata_tag(mesh, "TreeDrainPlacement", "Not placed; support cutout and collision review required")
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError("Native mesh save failed")
        receipt["assets"].append({"package": package, "triangles": counts, "kind": asset["kind"]})
    for material in materials.values():
        if not u.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError("Owned material save failed")
    path = source / "native-import-receipt.json"
    if path.exists():
        raise RuntimeError("Native import receipt already exists")
    with path.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(receipt, indent=2) + "\n")
    return receipt
