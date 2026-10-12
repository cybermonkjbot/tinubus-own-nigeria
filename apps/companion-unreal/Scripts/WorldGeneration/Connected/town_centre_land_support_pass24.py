"""Fill verified inland Town Centre bounds with instanced native ground.

This does not author a new coast: the source bank must lie beyond the bounds.
"""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-24-inland-support.json"
SPEC = importlib.util.spec_from_file_location("tc_land_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    selection = BASE.district_selection()
    bounds = selection["bounds"]
    plan = BASE.city_plan(BASE.References())
    bank = plan["lagoon"]["southBank"]
    relevant = []
    for a, b in zip(bank, bank[1:]):
        if b[0] < bounds["minX"] or a[0] > bounds["maxX"]:
            continue
        assert b[0] > a[0]
        for x in (max(a[0], bounds["minX"]), min(b[0], bounds["maxX"])):
            t = (x - a[0]) / (b[0] - a[0])
            relevant.append(a[1] + t * (b[1] - a[1]))
    assert relevant and min(relevant) > bounds["maxZ"], "District ground cannot cross source lagoon bank"
    BASE.make_level_current(BASE.DETAIL_MAP)
    assert not any(a.get_actor_label() == "TC_Inland24_Ground" for a in BASE.actors().get_all_level_actors())
    mesh_path = "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000"
    mesh = BASE.assets().load_asset(mesh_path)
    assert mesh
    mesh_bounds = mesh.get_bounds()
    dims = (2 * mesh_bounds.box_extent.x, 2 * mesh_bounds.box_extent.y)
    actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    actor.set_actor_label("TC_Inland24_Ground")
    actor.tags = ["Baddie.TownCentreInland24", "Baddie.Source:town-centre", "Baddie.NoRuntimeAuthority"]
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_instance(actor)
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=handles[0], new_class=unreal.InstancedStaticMeshComponent,
        blueprint_context=None, conform_transform_to_parent=False))
    assert not str(reason), str(reason)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(
        subsystem.k2_find_subobject_data_from_handle(handle))
    assert isinstance(component, unreal.InstancedStaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    material = BASE.assets().load_asset("/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v2/ISM_v1/Materials/M_Concrete")
    assert material and material.get_editor_property("used_with_instanced_static_meshes")
    component.set_material(0, material)
    component.set_collision_profile_name("BlockAll")
    count_x = math.ceil((bounds["maxZ"] - bounds["minZ"]) / 20)
    count_y = math.ceil((bounds["maxX"] - bounds["minX"]) / 20)
    tile_x = (bounds["maxZ"] - bounds["minZ"]) * 100 / count_x
    tile_y = (bounds["maxX"] - bounds["minX"]) * 100 / count_y
    scale = (tile_x / dims[0], tile_y / dims[1], 1.0)
    top = -24.0
    transforms = []
    for x in range(count_x):
        for y in range(count_y):
            cx = bounds["minZ"] * 100 + (x + 0.5) * tile_x
            cy = bounds["minX"] * 100 + (y + 0.5) * tile_y
            location = unreal.Vector(cx - mesh_bounds.origin.x * scale[0],
                                     cy - mesh_bounds.origin.y * scale[1],
                                     top - mesh_bounds.origin.z - mesh_bounds.box_extent.z)
            transforms.append(unreal.Transform(location=location, rotation=unreal.Rotator(0, 0, 0), scale=unreal.Vector(*scale)))
    component.add_instances(transforms, False, True)
    assert component.get_instance_count() == len(transforms)
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail, component, actor
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(),
               "pid": expected_pid, "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP,
               "detailLevelPackage": BASE.DETAIL_MAP, "source": selection["source"],
               "bounds": bounds, "minimumSourceSouthBankZ": min(relevant), "groundTopCm": top,
               "actorLabel": "TC_Inland24_Ground", "mesh": mesh_path, "tileDimensionsCm": [tile_x, tile_y],
               "instanceCount": len(transforms), "gridCounts": [count_x, count_y],
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established",
               "limitations": ["Flat inland support; terrain finish and district-edge continuation remain incomplete",
                                "No coastline change or distant substitute district", "Navigation and runtime collision unverified"]}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "instances": len(transforms)}))
