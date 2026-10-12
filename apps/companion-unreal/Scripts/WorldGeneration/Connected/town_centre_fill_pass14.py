"""Augment Town Centre fill13 with facade detail and backing slabs.

This keeps the saved fill13 shell massing, then adds the omitted source-region
detail pieces and bounded ground support needed to reduce blank-wall and blue
void reads in the current market and civic proof cameras.
"""

import gc
import hashlib
import importlib.util
import json
import os
from datetime import datetime, timezone
from pathlib import Path

import unreal


ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT_NAME = "town-centre-street-pass-15-detail-and-backing-cleaned.json"
OWNER = "cribtest11-world-generation-20261010/town-centre-street-pass-15-detail-and-backing-cleaned"
LABEL_PREFIX = "TC_Fill14_"
CLEAR_PREFIXES = [LABEL_PREFIX]
DETAIL_TOKENS = (
    "ACServiceAssembly",
    "Gutter3900",
    "PlasterRepair",
    "RainwaterDownpipe",
    "Shutter2800",
)

MODULE_PATH = ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fill_pass11.py"
SPEC = importlib.util.spec_from_file_location("town_centre_fill_pass13_helper", MODULE_PATH)
HELPER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HELPER)
BASE = HELPER.BASE

DETAIL_PLACEMENTS = HELPER.PLACEMENTS

BACKING_SLABS = [
    {
        "id": "market_backing_a",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-7000.0, -28150.0, -5.0],
        "yaw": 143.21278381347656,
        "scale": [20.0, 22.0, 1.0],
        "sourceIds": ["market-road-7", "market-road-8"],
    },
    {
        "id": "market_backing_b",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-6300.0, -27200.0, -5.0],
        "yaw": 143.21278381347656,
        "scale": [20.0, 22.0, 1.0],
        "sourceIds": ["market-road-8", "market-road-9"],
    },
    {
        "id": "market_backing_c",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-5600.0, -26250.0, -5.0],
        "yaw": 143.21278381347656,
        "scale": [18.0, 20.0, 1.0],
        "sourceIds": ["market-road-9", "market-road-10"],
    },
    {
        "id": "civic_backing_a",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-25200.0, -500.0, -5.0],
        "yaw": -177.03988647460938,
        "scale": [16.0, 24.0, 1.0],
        "sourceIds": ["civic-street-2", "civic-street-4"],
    },
    {
        "id": "civic_backing_b",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-25120.0, -2350.0, -5.0],
        "yaw": -177.03988647460938,
        "scale": [16.0, 24.0, 1.0],
        "sourceIds": ["civic-street-4", "civic-street-5"],
    },
    {
        "id": "civic_backing_c",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-24960.0, -5750.0, -5.0],
        "yaw": -177.03988647460938,
        "scale": [18.0, 28.0, 1.0],
        "sourceIds": ["civic-street-2", "civic-street-5"],
    },
    {
        "id": "civic_backing_d",
        "asset": "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000",
        "locationCm": [-24880.0, -7400.0, -5.0],
        "yaw": -177.03988647460938,
        "scale": [14.0, 18.0, 1.0],
        "sourceIds": ["civic-street-4", "civic-street-6"],
    },
]

PROP_CLONES = [
    {
        "id": "civic_kiosk_near_a",
        "sourceLabel": "TC_Static_Kiosk_CivicStreet_02",
        "locationCm": [-29250.0, -3450.0, 0.0],
        "yaw": 8.0,
        "sourceIds": ["civic-street-2", "civic-street-4"],
    },
    {
        "id": "civic_kiosk_near_b",
        "sourceLabel": "TC_Static_Kiosk_CivicStreet_02",
        "locationCm": [-27950.0, -5150.0, 0.0],
        "yaw": 18.0,
        "sourceIds": ["civic-street-4", "civic-street-5"],
    },
]

REGION_DETAIL_CACHE = {}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


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


def detail_snapshots(region_id):
    if region_id in REGION_DETAIL_CACHE:
        return REGION_DETAIL_CACHE[region_id]
    spec = HELPER.SOURCE_REGIONS[region_id]
    bounds = spec["bounds"]
    rows = []
    for actor in BASE.actors().get_all_level_actors():
        label = actor.get_actor_label()
        if not label.startswith("TC_") or label.startswith("TC_Fill"):
            continue
        if not any(token in label for token in DETAIL_TOKENS):
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
        rows.append(HELPER.snapshot_actor(label))
    assert rows, region_id
    REGION_DETAIL_CACHE[region_id] = sorted(
        rows,
        key=lambda item: (
            item["locationCm"][0],
            item["locationCm"][1],
            item["locationCm"][2],
            item["label"],
        ),
    )
    return REGION_DETAIL_CACHE[region_id]


def clone_detail_region(source_region, target_id, target_anchor_cm, target_anchor_yaw):
    snapshots = detail_snapshots(source_region)
    anchor_rows = HELPER.snapshot_region(source_region)
    anchor_snapshot = next(
        item for item in anchor_rows if item["label"] == HELPER.SOURCE_REGIONS[source_region]["anchorLabel"]
    )
    yaw_delta = target_anchor_yaw - anchor_snapshot["rotation"]["yaw"]
    created = []
    for snapshot in snapshots:
        dx = snapshot["locationCm"][0] - anchor_snapshot["locationCm"][0]
        dy = snapshot["locationCm"][1] - anchor_snapshot["locationCm"][1]
        dz = snapshot["locationCm"][2] - anchor_snapshot["locationCm"][2]
        rx, ry = HELPER.rotate_xy(dx, dy, yaw_delta)
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
                "kind": "detail-region",
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


def clone_prop(source_label, target_id, location_cm, yaw):
    snapshot = HELPER.snapshot_actor(source_label)
    label = f"{LABEL_PREFIX}{target_id}_{source_label}"
    actor = HELPER.spawn_snapshot(snapshot, label, location_cm, yaw)
    return {
        "label": label,
        "actor": actor.get_path_name(),
        "kind": "prop-clone",
        "sourceLabel": source_label,
        "mesh": snapshot["mesh"],
        "locationCm": location_cm,
        "yaw": yaw,
    }


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
        "mode": "detail-and-backing-augmentation",
        "mapPackage": BASE.TARGET_MAP,
        "detailLevelPackage": BASE.DETAIL_MAP,
        "files": files,
        "districtId": "town-centre",
        "districtBounds": selection["bounds"],
        "districtParcelCount": selection["parcelCount"],
        "districtSelectionPath": str(BASE.DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(BASE.DISTRICT_SELECTION),
        "detailPlacements": DETAIL_PLACEMENTS,
        "backingSlabs": BACKING_SLABS,
        "propClones": PROP_CLONES,
        "createdActorCount": len(created),
        "createdActors": created,
        "previewCameras": [
            "market-road-frontage-a",
            "market-road-frontage-b",
            "civic-street-approach",
            "civic-terrace-open-a",
            "civic-terrace-open-b",
        ],
        "visualIntent": "Reduce blank-shell reads by adding omitted facade detail pieces from accepted source rows and by laying bounded concrete backing under the worst visible voids.",
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
    assert not BASE.dirty_names(), "Expected clean packages before detail/backing pass"
    BASE.make_level_current(BASE.DETAIL_MAP)
    removed = clear_prior_fills()
    created = []
    for placement in DETAIL_PLACEMENTS:
        created.extend(
            clone_detail_region(
                placement["group"],
                placement["id"],
                placement["anchorLocationCm"],
                placement["anchorYaw"],
            )
        )
    for slab in BACKING_SLABS:
        label = f"{LABEL_PREFIX}{slab['id']}"
        actor = spawn_mesh_actor(slab["asset"], label, slab["locationCm"], slab["yaw"], slab["scale"])
        created.append(
            {
                "label": label,
                "actor": actor.get_path_name(),
                "kind": "backing-slab",
                "mesh": slab["asset"],
                "locationCm": slab["locationCm"],
                "yaw": slab["yaw"],
                "scale": slab["scale"],
                "sourceIds": slab["sourceIds"],
            }
        )
    for prop in PROP_CLONES:
        created.append(clone_prop(prop["sourceLabel"], prop["id"], prop["locationCm"], prop["yaw"]))
    receipt_path = save_receipt(created)
    print(
        json.dumps(
            {
                "mapPackage": BASE.TARGET_MAP,
                "detailLevelPackage": BASE.DETAIL_MAP,
                "removedPriorFill14Actors": removed,
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
