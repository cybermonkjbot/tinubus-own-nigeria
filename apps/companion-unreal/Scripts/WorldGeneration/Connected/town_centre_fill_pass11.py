"""Bounded filler-building pass for Town Centre streetpass01.

This pass stops cloning isolated shell stacks and instead reuses complete
street-facing frontage regions already accepted in the detail level, including
their pavement, kerb and drain support. The target is ordinary pass-by Lagos
street massing, not new hero destinations.
"""

import gc
import hashlib
import importlib.util
import json
import math
import os
from datetime import datetime, timezone
from pathlib import Path

import unreal


ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT_NAME = "town-centre-street-pass-13-filler-frontage-regions.json"
OWNER = "cribtest11-world-generation-20261010/town-centre-street-pass-13-filler-frontage-regions"
LABEL_PREFIX = "TC_Fill13_"
CLEAR_PREFIXES = ["TC_Fill11_", "TC_Fill12_", "TC_Fill13_"]

MODULE_PATH = ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py"
SPEC = importlib.util.spec_from_file_location("town_centre_street_pass", MODULE_PATH)
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)

REGION_LABEL_TOKENS = (
    "FrontageStructure",
    "ShopBay",
    "OpenShopInterior",
    "ResidentialBay",
    "Window",
    "Balcony",
    "RoofSheetPanel",
    "Drain",
    "DrainCover",
    "Kerb",
    "Pavement",
)

SOURCE_REGIONS = {
    "market": {
        "anchorLabel": "TC_FrontageStructure_4698",
        "bounds": {
            "minX": -8200.0,
            "maxX": -6400.0,
            "minY": -23750.0,
            "maxY": -21650.0,
        },
    },
    "civic": {
        "anchorLabel": "TC_FrontageStructure_224",
        "bounds": {
            "minX": -25150.0,
            "maxX": -24850.0,
            "minY": -4900.0,
            "maxY": -3150.0,
        },
    },
}
REGION_SNAPSHOT_CACHE = {}

PLACEMENTS = [
    {
        "id": "market_extension_a",
        "group": "market",
        "anchorLabel": "TC_FrontageStructure_4698",
        "anchorLocationCm": [-7369.1514, -27466.1615, 0.0],
        "anchorYaw": 143.2127799987793,
        "sourceIds": ["market-road-7", "market-road-8"],
    },
    {
        "id": "market_extension_b",
        "group": "market",
        "anchorLabel": "TC_FrontageStructure_4698",
        "anchorLocationCm": [-6662.5144, -26521.1408, 0.0],
        "anchorYaw": 143.2127799987793,
        "sourceIds": ["market-road-8", "market-road-9"],
    },
    {
        "id": "market_extension_c",
        "group": "market",
        "anchorLabel": "TC_FrontageStructure_4698",
        "anchorLocationCm": [-5955.8773, -25576.1202, 0.0],
        "anchorYaw": 143.2127799987793,
        "sourceIds": ["market-road-9", "market-road-10"],
    },
    {
        "id": "civic_opposite_extension_a",
        "group": "civic",
        "anchorLabel": "TC_FrontageStructure_224",
        "anchorLocationCm": [-25132.6056, -1268.7259, 0.0],
        "anchorYaw": -177.0398864746094,
        "sourceIds": ["civic-street-2", "civic-street-4"],
    },
    {
        "id": "civic_opposite_extension_b",
        "group": "civic",
        "anchorLabel": "TC_FrontageStructure_224",
        "anchorLocationCm": [-25193.5416, -90.3003, 0.0],
        "anchorYaw": -177.0398864746094,
        "sourceIds": ["civic-street-4", "civic-street-5"],
    },
    {
        "id": "civic_return_a",
        "group": "civic",
        "anchorLabel": "TC_FrontageStructure_224",
        "anchorLocationCm": [-24888.8617, -5982.4282, 0.0],
        "anchorYaw": -177.0398864746094,
        "sourceIds": ["civic-street-2", "civic-street-5"],
    },
    {
        "id": "civic_return_b",
        "group": "civic",
        "anchorLabel": "TC_FrontageStructure_224",
        "anchorLocationCm": [-24827.9257, -7160.8538, 0.0],
        "anchorYaw": -177.0398864746094,
        "sourceIds": ["civic-street-4", "civic-street-6"],
    },
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def actor_by_label(label):
    for actor in BASE.actors().get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise AssertionError(f"Actor not found: {label}")


def static_mesh_component(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    assert component is not None, actor.get_actor_label()
    return component


def snapshot_actor(label):
    actor = actor_by_label(label)
    component = static_mesh_component(actor)
    location = actor.get_actor_location()
    rotation = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    mesh = component.get_editor_property("static_mesh")
    assert mesh is not None, label
    materials = []
    for material in component.get_editor_property("override_materials"):
        materials.append(material.get_path_name() if material else None)
    return {
        "label": label,
        "mesh": mesh.get_path_name(),
        "materials": materials,
        "locationCm": [
            location.x,
            location.y,
            location.z,
        ],
        "rotation": {
            "pitch": rotation.pitch,
            "yaw": rotation.yaw,
            "roll": rotation.roll,
        },
        "scale": [
            scale.x,
            scale.y,
            scale.z,
        ],
        "tags": [str(tag) for tag in actor.tags],
    }


def snapshot_region(region_id):
    if region_id in REGION_SNAPSHOT_CACHE:
        return REGION_SNAPSHOT_CACHE[region_id]
    spec = SOURCE_REGIONS[region_id]
    bounds = spec["bounds"]
    snapshots = []
    for actor in BASE.actors().get_all_level_actors():
        label = actor.get_actor_label()
        if not label.startswith("TC_"):
            continue
        if not any(token in label for token in REGION_LABEL_TOKENS):
            continue
        location = actor.get_actor_location()
        if not (
            bounds["minX"] <= location.x <= bounds["maxX"]
            and bounds["minY"] <= location.y <= bounds["maxY"]
        ):
            continue
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        if component is None or component.get_editor_property("static_mesh") is None:
            continue
        snapshots.append(snapshot_actor(label))
    assert snapshots, region_id
    assert any(item["label"] == spec["anchorLabel"] for item in snapshots), region_id
    REGION_SNAPSHOT_CACHE[region_id] = sorted(
        snapshots,
        key=lambda item: (
            item["locationCm"][0],
            item["locationCm"][1],
            item["locationCm"][2],
            item["label"],
        ),
    )
    return REGION_SNAPSHOT_CACHE[region_id]


def rotate_xy(dx, dy, yaw_delta):
    radians = math.radians(yaw_delta)
    cos_yaw = math.cos(radians)
    sin_yaw = math.sin(radians)
    return (
        dx * cos_yaw - dy * sin_yaw,
        dx * sin_yaw + dy * cos_yaw,
    )


def spawn_snapshot(snapshot, target_label, location_cm, yaw):
    assert actor_by_label_or_none(target_label) is None, target_label
    actor = BASE.actors().spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location_cm),
        unreal.Rotator(
            pitch=snapshot["rotation"]["pitch"],
            yaw=yaw,
            roll=snapshot["rotation"]["roll"],
        ),
    )
    actor.set_actor_label(target_label)
    actor.set_actor_scale3d(unreal.Vector(*snapshot["scale"]))
    actor.tags = [unreal.Name(tag) for tag in snapshot["tags"]]
    component = static_mesh_component(actor)
    component.set_static_mesh(BASE.assets().load_asset(snapshot["mesh"]))
    component.set_editor_property(
        "override_materials",
        [BASE.assets().load_asset(path) if path else None for path in snapshot["materials"]],
    )
    return actor


def actor_by_label_or_none(label):
    for actor in BASE.actors().get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def clear_prior_fills():
    doomed = []
    for actor in BASE.actors().get_all_level_actors():
        label = actor.get_actor_label()
        if any(label.startswith(prefix) for prefix in CLEAR_PREFIXES):
            doomed.append(actor)
    for actor in doomed:
        assert BASE.actors().destroy_actor(actor), actor.get_actor_label()
    return len(doomed)


def clone_region(source_region, target_id, target_anchor_cm, target_anchor_yaw):
    snapshots = snapshot_region(source_region)
    anchor_snapshot = next(
        item for item in snapshots if item["label"] == SOURCE_REGIONS[source_region]["anchorLabel"]
    )
    created = []
    yaw_delta = target_anchor_yaw - anchor_snapshot["rotation"]["yaw"]
    for snapshot in snapshots:
        dx = snapshot["locationCm"][0] - anchor_snapshot["locationCm"][0]
        dy = snapshot["locationCm"][1] - anchor_snapshot["locationCm"][1]
        dz = snapshot["locationCm"][2] - anchor_snapshot["locationCm"][2]
        rx, ry = rotate_xy(dx, dy, yaw_delta)
        location_cm = [
            target_anchor_cm[0] + rx,
            target_anchor_cm[1] + ry,
            target_anchor_cm[2] + dz,
        ]
        label = f"{LABEL_PREFIX}{target_id}_{snapshot['label']}"
        actor = spawn_snapshot(snapshot, label, location_cm, snapshot["rotation"]["yaw"] + yaw_delta)
        created.append(
            {
                "label": label,
                "actor": actor.get_path_name(),
                "sourceLabel": snapshot["label"],
                "sourceRegion": source_region,
                "mesh": snapshot["mesh"],
                "locationCm": location_cm,
                "yaw": snapshot["rotation"]["yaw"] + yaw_delta,
            }
        )
    return created


def save_receipt(created):
    detail_world = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail_world, BASE.DETAIL_MAP), "Failed to save detail level"
    world_asset = BASE.world()
    BASE.assets().set_metadata_tag(world_asset, "Baddie.TownCentreStreetPass.Owner", OWNER)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world_asset, BASE.TARGET_MAP), "Failed to save parent map"
    files = {}
    files.update(BASE.package_files(BASE.TARGET_MAP, True))
    files.update(BASE.package_files(BASE.DETAIL_MAP, True))
    selection = BASE.district_selection()
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "mode": "bounded-filler-building-pass",
        "mapPackage": BASE.TARGET_MAP,
        "detailLevelPackage": BASE.DETAIL_MAP,
        "files": files,
        "districtId": "town-centre",
        "districtBounds": selection["bounds"],
        "districtParcelCount": selection["parcelCount"],
        "districtSelectionPath": str(BASE.DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(BASE.DISTRICT_SELECTION),
        "targetPlacements": PLACEMENTS,
        "sourceRegions": SOURCE_REGIONS,
        "createdActorCount": len(created),
        "createdActors": created,
        "previewCameras": [
            "market-road-frontage-a",
            "market-road-frontage-b",
            "civic-street-approach",
            "civic-terrace-open-a",
            "civic-terrace-open-b",
        ],
        "visualIntent": "Add ordinary pass-by buildings with grounded frontage support to the market and civic corridors so Town Centre reads as connected Lagos street fabric rather than isolated shell masses.",
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / RECEIPT_NAME
    assert not receipt_path.exists(), receipt_path
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP, "Expected Town Centre streetpass01 to be loaded"
    assert not BASE.dirty_names(), "Expected clean packages before filler pass"
    BASE.make_level_current(BASE.DETAIL_MAP)
    removed = clear_prior_fills()
    created = []
    for placement in PLACEMENTS:
        created.extend(
            clone_region(
                placement["group"],
                placement["id"],
                placement["anchorLocationCm"],
                placement["anchorYaw"],
            )
        )
    receipt_path = save_receipt(created)
    print(
        json.dumps(
            {
                "mapPackage": BASE.TARGET_MAP,
                "detailLevelPackage": BASE.DETAIL_MAP,
                "removedPriorFillActors": removed,
                "createdActorCount": len(created),
                "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
                "receiptSha256": sha(receipt_path),
            }
        )
    )


if __name__ == "__main__":
    try:
        execute(int(os.environ["BADDIE_EXPECTED_PID"]))
    finally:
        gc.collect()
