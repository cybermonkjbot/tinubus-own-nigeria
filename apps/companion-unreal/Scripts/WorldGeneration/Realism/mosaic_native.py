"""Exclusive additive mosaic authoring, reusing immutable imported native assets.

Call through the existing PID-bound bridge only after the other editor writer
has relinquished its lease. Importing this module does not mutate the editor.
"""
import gc
import hashlib
import json
import os
from pathlib import Path
from datetime import datetime, timezone

from WorldGeneration.Realism import mosaic_layout


ROOT = Path("C:/dev/cribtest-game")
CONFIG_PATH = Path(__file__).with_name("mosaic-yaba-v1.json")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11/realism"
RECEIPT = OUT / "mosaic-yaba-native-v1.json"
LEASE = OUT / "mosaic-native-writer.lock"
OTHER_LEASE = ROOT / "output/cribtest11-world-generation/execution/worker.lock"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source_inputs():
    config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    if config["schemaVersion"] != 1 or not 2 <= config["sideRoadSections"] <= 10:
        raise ValueError("Unsupported mosaic layout contract")
    if not config["namespace"].startswith("/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Mosaic_"):
        raise ValueError("Mosaic namespace escaped owned additive content")
    if len({item["id"] for item in config["buildings"]}) != len(config["buildings"]):
        raise ValueError("Duplicate building identity")
    for path, expected in ((OUT / config["baseReceipt"], config["baseReceiptSha256"]),
                           (OUT / config["assetsReceipt"], config["assetsReceiptSha256"]),
                           (ROOT / config["researchDocument"], config["researchDocumentSha256"])):
        if sha(path) != expected:
            raise ValueError("Pinned source changed: " + str(path))
    return config, json.loads((OUT / config["baseReceipt"]).read_text(encoding="utf-8"))


def guard(expected_pid, expected_world):
    import unreal
    from WorldGeneration.Realism import safe_level_change as safe
    if type(expected_pid) is not int or os.getpid() != expected_pid:
        raise RuntimeError("Editor identity changed")
    if Path(unreal.Paths.project_dir()).resolve() != (ROOT / "apps/companion-unreal").resolve():
        raise RuntimeError("Wrong project")
    if OTHER_LEASE.exists():
        raise RuntimeError("Existing world-generation lease is present; do not steal it")
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError("PIE is active")
    if safe.dirty_names():
        raise RuntimeError("Unreviewed dirty packages; save through their owner")
    actual = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split(".")[0]
    if actual != expected_world:
        raise RuntimeError("Editor world changed: " + actual)


def environment(config):
    import unreal
    from WorldGeneration.Realism import author_slice as common
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for spec in config["cameras"]:
        actor = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*(v * 100 for v in spec["locationMetres"])), common.rotation(spec["rotation"]))
        actor.set_actor_label("MosaicCamera_" + spec["id"])
        camera = actor.get_component_by_class(unreal.CameraComponent)
        camera.set_field_of_view(spec["fov"])
        camera.set_editor_property("constrain_aspect_ratio", False)
    actors.select_nothing()


def claim_world():
    import unreal
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assets.set_metadata_tag(world, "Baddie.ArtSlice.Owner", "lagos-mosaic-v1")
    assets.set_metadata_tag(world, "Baddie.ArtSlice.Role", "EditorOnly_LocalArt_NotProductionDistrict")


def author(expected_pid, expected_world):
    import unreal
    from WorldGeneration.Realism import author_slice as common
    from WorldGeneration.Realism import batch_frontage as batching
    from WorldGeneration.Realism import safe_level_change as safe
    config, base = source_inputs()
    package = config["namespace"] + "/Maps/" + config["mapName"]
    if RECEIPT.exists():
        raise RuntimeError("Existing immutable receipt; use readback, not replay")
    guard(expected_pid, expected_world)
    common.guard(expected_pid)
    common.verify_files(base["files"])
    common.fresh(package, True)
    rows, buildings = mosaic_layout.plan(base, config)
    from WorldGeneration.Realism import mosaic_activity, mosaic_audio, mosaic_navigation, mosaic_routines
    navigation = mosaic_navigation.build_navigation(config)
    routines = mosaic_routines.build_routines(config, navigation["destinations"])
    emitters = mosaic_audio.build_emitters(config, navigation["destinations"])
    materials = dict(base["materialUsageDerivatives"])
    missing = {path for row in rows for path in row["materials"] if path not in materials}
    if missing:
        raise ValueError("Missing inherited material contract: " + str(sorted(missing)))
    # A failed operation leaves its lease for explicit inspection and recovery.
    # Removing it in a finally block would hide partial native output.
    with LEASE.open("x", encoding="utf-8") as stream:
        json.dump({"pid": expected_pid, "package": package, "owner": "lagos-mosaic-v1"}, stream)
    guard(expected_pid, expected_world)
    safe.prepare_change(expected_pid)
    gc.collect()
    common.new_scene(package, "daylight", sha(__file__))
    claim_world()
    batches, journal = batching.place(rows, materials)
    environment(config)
    if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():
        raise RuntimeError("Mosaic map save failed")
    if safe.dirty_names():
        raise RuntimeError("Saved map still has dirty packages")
    common.verify_files(base["files"])
    files = dict(base["files"])
    files.update(common.package_files(package, True))
    record = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(),
              "pid": expected_pid, "mapPackage": package, "files": files,
              "scriptSha256": sha(__file__), "layoutSha256": sha(mosaic_layout.__file__),
              "configSha256": sha(CONFIG_PATH), "baseReceiptSha256": config["baseReceiptSha256"],
              "researchDocumentSha256": config["researchDocumentSha256"],
              "meshes": base["meshes"], "placements": journal, "batches": batches,
              "navigation": navigation, "routines": routines, "audioEmitters": emitters,
              "activityPolicy": mosaic_activity.build_policy(navigation["destinations"]),
              "activityInitialState": mosaic_activity.initial_state(navigation, routines),
              "events": mosaic_navigation.build_events(navigation["destinations"]),
              "authoringSystemHashes": {"navigation": sha(mosaic_navigation.__file__),
                                       "routines": sha(mosaic_routines.__file__), "audio": sha(mosaic_audio.__file__),
                                       "activity": sha(mosaic_activity.__file__)},
              "materialUsageDerivatives": materials, "buildingRuns": buildings,
              "cameras": config["cameras"], "mainStreetLengthMetres": 120,
              "sideStreetSurfaceLengthMetres": config["sideRoadSections"] * 10,
              "yard": config["yard"], "sourceGeometryChanged": False,
              "canonicalCoordinatesChanged": False, "geography": config["geography"],
              "gates": config["gates"], "pendingAssets": config["pendingAssets"],
              "partitioned": False, "runtimeRegistered": False,
              "visualAcceptance": "pending native captures", "playableAcceptance": False}
    with RECEIPT.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(record, stream, indent=2, allow_nan=False)
        stream.write("\n")
    LEASE.unlink()
    print(json.dumps({"mapPackage": package, "placements": len(journal), "receipt": str(RECEIPT)}))


def prepare_camera(expected_pid, camera_id):
    import unreal
    from WorldGeneration.Realism import author_slice as common
    config, _ = source_inputs()
    if LEASE.exists():
        raise RuntimeError("Partial mosaic operation needs owner inspection first")
    package = config["namespace"] + "/Maps/" + config["mapName"]
    guard(expected_pid, package)
    spec = next(camera for camera in config["cameras"] if camera["id"] == camera_id)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v * 100 for v in spec["locationMetres"])), common.rotation(spec["rotation"]), key)
    levels.set_level_viewport_fov(spec["fov"], key)
    levels.editor_set_game_view(True, key)
    levels.editor_invalidate_viewports()


def readback(expected_pid):
    import unreal
    from WorldGeneration.Realism import author_slice as common
    from WorldGeneration.Realism import batch_frontage as batching
    record = json.loads(RECEIPT.read_text(encoding="utf-8"))
    if LEASE.exists():
        raise RuntimeError("Partial mosaic operation needs owner inspection first")
    if record["configSha256"] != sha(CONFIG_PATH) or record["layoutSha256"] != sha(mosaic_layout.__file__):
        raise ValueError("Saved mosaic and current layout differ")
    guard(expected_pid, record["mapPackage"])
    common.verify_files(record["files"])
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components = {component.get_path_name(): component for actor in actors.get_all_level_actors()
                  for component in actor.get_components_by_class(unreal.StaticMeshComponent)}
    for row in record["placements"]:
        component = components[row["component"]]
        transform = component.get_instance_transform(row["instanceIndex"], world_space=True) if row["batched"] else component.get_owner().get_actor_transform()
        actual = batching.transform_data(transform)
        for key in ("locationCm", "scale"):
            if any(abs(a - b) > .05 for a, b in zip(actual[key], row["transform"][key])):
                raise ValueError("Placement mismatch: " + row["sourceId"])
        for axis in ("pitch", "yaw", "roll"):
            delta = (actual["rotation"][axis] - row["transform"]["rotation"][axis] + 180) % 360 - 180
            if abs(delta) > .001:
                raise ValueError("Rotation mismatch: " + row["sourceId"])
        if component.get_editor_property("static_mesh").get_path_name().split(".")[0] != row["mesh"]:
            raise ValueError("Mesh mismatch: " + row["sourceId"])
        expected_materials = [record["materialUsageDerivatives"][p] for p in row["materials"]] if row["batched"] else row["materials"]
        actual_materials = [component.get_material(i).get_path_name().split(".")[0] for i in range(component.get_num_materials())]
        if actual_materials != expected_materials:
            raise ValueError("Material mismatch: " + row["sourceId"])
    print(json.dumps({"mapPackage": record["mapPackage"], "sourceMappingsInspected": len(record["placements"]),
                      "mismatches": 0, "visualAcceptance": "not established by component inspection"}))
