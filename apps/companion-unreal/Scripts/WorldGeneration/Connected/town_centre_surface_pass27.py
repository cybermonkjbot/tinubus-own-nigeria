"""Remove verified legacy road overlays and reconcile inland support height."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-27-surface-continuity.json"
SPEC = importlib.util.spec_from_file_location("tc_surface_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    actors = BASE.actors().get_all_level_actors()
    legacy = []
    for actor in actors:
        if not actor.get_actor_label().startswith("TC_Road_"):
            continue
        assert actor.get_outer().get_path_name().split(".")[0] == BASE.DETAIL_MAP
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        assert component.get_editor_property("static_mesh").get_path_name().split(".")[0] == BASE.ROAD_MESH
        centre, extent = actor.get_actor_bounds(False)
        assert abs(centre.z + extent.z + 20) < 0.1
        legacy.append({"label": actor.get_actor_label(), "locationCm": BASE.vec(actor.get_actor_location()) if hasattr(BASE, "vec") else [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z],
                       "boundsCentreCm": [centre.x, centre.y, centre.z], "boundsExtentCm": [extent.x, extent.y, extent.z],
                       "mesh": BASE.ROAD_MESH})
    assert legacy
    ground = [actor for actor in actors if actor.get_actor_label() == "TC_Inland24_Ground"]
    assert len(ground) == 1
    component = ground[0].get_component_by_class(unreal.InstancedStaticMeshComponent)
    assert component.get_instance_count() == 2964
    mesh_bounds = component.get_editor_property("static_mesh").get_bounds()
    transforms = []
    for index in range(component.get_instance_count()):
        transform = component.get_instance_transform(index, world_space=True)
        assert abs(transform.translation.z + mesh_bounds.origin.z + mesh_bounds.box_extent.z + 24) < 0.1
        transforms.append(transform)
    BASE.make_level_current(BASE.DETAIL_MAP)
    for actor in actors:
        if actor.get_actor_label().startswith("TC_Road_"):
            assert BASE.actors().destroy_actor(actor)
    for index, transform in enumerate(transforms):
        location = transform.translation
        transform.translation = unreal.Vector(location.x, location.y, location.z + 19)
        assert component.update_instance_transform(index, transform, world_space=True,
                                                   mark_render_state_dirty=index == len(transforms) - 1, teleport=True)
    del actors, actor, ground, component, mesh_bounds
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(), "pid": expected_pid,
               "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP, "detailLevelPackage": BASE.DETAIL_MAP,
               "source": BASE.district_selection()["source"], "removedLegacyRoadOverlays": legacy,
               "retainedRoadAuthority": "TC_Ground22 source-centred carriageways and original canonical road-ribbon level",
               "groundActor": "TC_Inland24_Ground", "groundInstances": len(transforms),
               "previousGroundTopCm": -24, "newGroundTopCm": -5,
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "limitations": ["Road intersections and surface materials still require detailed review",
                                "Legacy pavement/drain module heights are not reconciled by this pass"],
               "playableAcceptance": "not-established"}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "removedRoadOverlays": len(legacy), "groundTopCm": -5}))
