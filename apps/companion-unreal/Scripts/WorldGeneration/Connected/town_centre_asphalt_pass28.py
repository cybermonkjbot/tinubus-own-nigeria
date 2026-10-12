"""Correct physical asphalt sampling on scaled canonical carriageways."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-28-asphalt-metres.json"
SPEC = importlib.util.spec_from_file_location("tc_asphalt_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
PACKAGE = "/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Materials/M_Asphalt_WorldMetres28"


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP
    assert set(BASE.dirty_names()).issubset({PACKAGE})
    assert not RECEIPT.exists()
    assert not (ROOT / "apps/companion-unreal/Content" / (PACKAGE.removeprefix("/Game/") + ".uasset")).exists()
    material = (BASE.assets().load_asset(PACKAGE) if BASE.assets().does_asset_exist(PACKAGE)
                else BASE.assets().duplicate_asset(BASE.ROAD_MATERIAL, PACKAGE))
    assert material
    library = unreal.MaterialEditingLibrary
    world = library.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1500, 0)
    divide = library.create_material_expression(material, unreal.MaterialExpressionDivide, -1100, 0)
    divide.set_editor_property("const_b", 200.0)
    assert library.connect_material_expressions(world, "XY", divide, "A")
    texture_records = []
    for prop in (unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialProperty.MP_NORMAL):
        node = library.get_material_property_input_node(material, prop)
        assert isinstance(node, unreal.MaterialExpressionTextureSampleParameter2D)
        assert library.connect_material_expressions(divide, "", node, "UVs")
        texture_records.append({"property": str(prop), "texture": node.get_editor_property("texture").get_path_name(),
                                "samplerType": str(node.get_editor_property("sampler_type"))})
    roughness = library.get_material_property_input_node(material, unreal.MaterialProperty.MP_ROUGHNESS)
    maximum = library.create_material_expression(material, unreal.MaterialExpressionMax, -150, 300)
    maximum.set_editor_property("const_b", 0.88)
    assert library.connect_material_expressions(roughness, "R", maximum, "A")
    assert library.connect_material_property(maximum, "", unreal.MaterialProperty.MP_ROUGHNESS)
    specular = BASE.common.parameter(material, unreal.MaterialExpressionScalarParameter, "DryRoadSpecular", 0.18, 1400)
    assert library.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    BASE.assets().set_metadata_tag(material, "Baddie.TownCentre.SurfaceUV", "WorldXY_cm/200; source 2m asphalt period; no mesh or texture-byte changes")
    library.recompile_material(material)
    assert BASE.assets().save_loaded_asset(material, only_if_is_dirty=False)
    rows = []
    for actor in BASE.actors().get_all_level_actors():
        if not (actor.get_actor_label().startswith("TC_Ground22_") and actor.get_actor_label().endswith("_asphalt")):
            continue
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        assert component.get_material(0).get_path_name().split(".")[0] == BASE.ROAD_MATERIAL
        component.set_material(0, material)
        rows.append({"label": actor.get_actor_label(), "previousMaterial": BASE.ROAD_MATERIAL, "newMaterial": PACKAGE})
    assert rows
    del actor, component, world
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(), "pid": expected_pid,
               "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP, "detailLevelPackage": BASE.DETAIL_MAP,
               "materialPackage": PACKAGE, "sourceMaterial": BASE.ROAD_MATERIAL, "textureInputs": texture_records,
               "physicalPeriodMetres": [2, 2], "coordinateBasis": "WorldXY centimetres divided by 200",
               "changedActors": rows, "files": {**BASE.package_files(PACKAGE), **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established", "visualAcceptance": "pending matched and source-road captures"}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "carriageways": len(rows), "material": PACKAGE}))
