"""Replace shell-dominant filler with corner-facing frontage fabric.

This pass rebuilds the failed market and civic proof-camera edges using the
accepted Town Centre frontage regions, but in a different composition:
front-facing pass-by rows, short return blocks, and visible ground closure.
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
RECEIPT_NAME = "town-centre-street-pass-21-civic-face-b-trimmed.json"
OWNER = "cribtest11-world-generation-20261010/town-centre-street-pass-21-civic-face-b-trimmed"
LABEL_PREFIX = "TC_Fill16_"
CLEAR_PREFIXES = ["TC_Fill13_", "TC_Fill14_", "TC_Fill16_"]

MODULE_PATH = ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fill_pass11.py"
SPEC = importlib.util.spec_from_file_location("town_centre_fill_pass13_helper", MODULE_PATH)
HELPER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HELPER)
BASE = HELPER.BASE


REGION_PLACEMENTS = [
    {
        "id": "market_face_a",
        "group": "market",
        "anchorLocationCm": [-7680.0, -23680.0, 0.0],
        "anchorYaw": 56.0,
        "sourceIds": ["market-road-7", "market-road-8"],
    },
    {
        "id": "market_face_b",
        "group": "market",
        "anchorLocationCm": [-6240.0, -22980.0, 0.0],
        "anchorYaw": 56.0,
        "sourceIds": ["market-road-8", "market-road-9"],
    },
    {
        "id": "market_return_a",
        "group": "market",
        "anchorLocationCm": [-5280.0, -24380.0, 0.0],
        "anchorYaw": 56.0,
        "sourceIds": ["market-road-9", "market-road-10"],
    },
    {
        "id": "civic_face_a",
        "group": "civic",
        "anchorLocationCm": [-25220.0, -1800.0, 0.0],
        "anchorYaw": 350.0,
        "sourceTrimCm": {"minY": -450.0, "maxY": 450.0},
        "sourceIds": ["civic-street-2", "civic-street-4"],
    },
    {
        "id": "civic_face_b",
        "group": "civic",
        "anchorLocationCm": [-24980.0, -4350.0, 0.0],
        "anchorYaw": 18.0,
        "sourceTrimCm": {"minY": -450.0, "maxY": 450.0},
        "sourceIds": ["civic-street-4", "civic-street-5"],
    },
    {
        "id": "civic_face_c",
        "group": "civic",
        "anchorLocationCm": [-29720.0, 1200.0, 0.0],
        "anchorYaw": 350.0,
        "sourceIds": ["civic-street-2", "civic-street-5"],
    },
]


GROUND_SLABS = [
    {
        "id": "market_ground_a",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-6900.0, -23580.0, -5.0],
        "yaw": 56.0,
        "scale": [28.0, 28.0, 1.0],
        "sourceIds": ["market-road-7", "market-road-9"],
    },
    {
        "id": "market_ground_b",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-5360.0, -24160.0, -5.0],
        "yaw": 56.0,
        "scale": [18.0, 20.0, 1.0],
        "sourceIds": ["market-road-9", "market-road-10"],
    },
    {
        "id": "market_ground_c",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-10400.0, -24880.0, -5.0],
        "yaw": 34.0,
        "scale": [20.0, 18.0, 1.0],
        "sourceIds": ["market-road-7", "market-road-8"],
    },
    {
        "id": "civic_ground_b",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-25040.0, -4300.0, -5.0],
        "yaw": 18.0,
        "scale": [18.0, 22.0, 1.0],
        "sourceIds": ["civic-street-4", "civic-street-5"],
    },
    {
        "id": "civic_ground_c",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-29740.0, 1200.0, -5.0],
        "yaw": 350.0,
        "scale": [18.0, 22.0, 1.0],
        "sourceIds": ["civic-street-2", "civic-street-5"],
    },
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def actor_by_label_or_none(label):
    for actor in BASE.actors().get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def rotate_xy(dx, dy, yaw_delta):
    radians = math.radians(yaw_delta)
    cos_yaw = math.cos(radians)
    sin_yaw = math.sin(radians)
    return (
        dx * cos_yaw - dy * sin_yaw,
        dx * sin_yaw + dy * cos_yaw,
    )


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
    snapshots = HELPER.snapshot_region(source_region)
    anchor_snapshot = next(
        item for item in snapshots if item["label"] == HELPER.SOURCE_REGIONS[source_region]["anchorLabel"]
    )
    placement = next(item for item in REGION_PLACEMENTS if item["id"] == target_id)
    trim = placement.get("sourceTrimCm")
    yaw_delta = target_anchor_yaw - anchor_snapshot["rotation"]["yaw"]
    created = []
    for snapshot in snapshots:
        dx = snapshot["locationCm"][0] - anchor_snapshot["locationCm"][0]
        dy = snapshot["locationCm"][1] - anchor_snapshot["locationCm"][1]
        dz = snapshot["locationCm"][2] - anchor_snapshot["locationCm"][2]
        if trim:
            if dx < trim.get("minX", float("-inf")) or dx > trim.get("maxX", float("inf")):
                continue
            if dy < trim.get("minY", float("-inf")) or dy > trim.get("maxY", float("inf")):
                continue
        rx, ry = rotate_xy(dx, dy, yaw_delta)
        location_cm = [
            target_anchor_cm[0] + rx,
            target_anchor_cm[1] + ry,
            target_anchor_cm[2] + dz,
        ]
        label = f"{LABEL_PREFIX}{target_id}_{snapshot['label']}"
        assert actor_by_label_or_none(label) is None, label
        actor = HELPER.spawn_snapshot(snapshot, label, location_cm, snapshot["rotation"]["yaw"] + yaw_delta)
        created.append(
            {
                "label": label,
                "actor": actor.get_path_name(),
                "kind": "frontage-region",
                "sourceLabel": snapshot["label"],
                "sourceRegion": source_region,
                "mesh": snapshot["mesh"],
                "locationCm": location_cm,
                "yaw": snapshot["rotation"]["yaw"] + yaw_delta,
            }
        )
    return created


def spawn_mesh_actor(asset_path, label, location_cm, yaw, scale):
    assert actor_by_label_or_none(label) is None, label
    actor = BASE.actors().spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location_cm),
        unreal.Rotator(0.0, yaw, 0.0),
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(BASE.assets().load_asset(asset_path))
    return actor


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
        "mode": "corner-facing-filler-fabric",
        "mapPackage": BASE.TARGET_MAP,
        "detailLevelPackage": BASE.DETAIL_MAP,
        "files": files,
        "districtId": "town-centre",
        "districtBounds": selection["bounds"],
        "districtParcelCount": selection["parcelCount"],
        "districtSelectionPath": str(BASE.DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(BASE.DISTRICT_SELECTION),
        "regionPlacements": REGION_PLACEMENTS,
        "groundSlabs": GROUND_SLABS,
        "createdActorCount": len(created),
        "createdActors": created,
        "previewCameras": [
            "market-road-frontage-a",
            "market-road-frontage-b",
            "civic-street-approach",
            "civic-terrace-open-a",
            "civic-terrace-open-b",
        ],
        "visualIntent": "Replace shell-side proof views with frontage-facing pass-by buildings, short return blocks, and explicit ground closure using accepted Town Centre source regions.",
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
    assert not BASE.dirty_names(), "Expected clean packages before corner-fabric pass"
    BASE.make_level_current(BASE.DETAIL_MAP)
    removed = clear_prior_fills()
    created = []
    for placement in REGION_PLACEMENTS:
        created.extend(
            clone_region(
                placement["group"],
                placement["id"],
                placement["anchorLocationCm"],
                placement["anchorYaw"],
            )
        )
    for slab in GROUND_SLABS:
        label = f"{LABEL_PREFIX}{slab['id']}"
        actor = spawn_mesh_actor(slab["asset"], label, slab["locationCm"], slab["yaw"], slab["scale"])
        created.append(
            {
                "label": label,
                "actor": actor.get_path_name(),
                "kind": "ground-slab",
                "mesh": slab["asset"],
                "locationCm": slab["locationCm"],
                "yaw": slab["yaw"],
                "scale": slab["scale"],
                "sourceIds": slab["sourceIds"],
            }
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
