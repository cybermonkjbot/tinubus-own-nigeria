"""Additive Lagos street pass for the connected town-centre neighbourhood.

This script snapshots the existing connected town-centre parent into plain
data, creates a fresh persistent parent map revision, reattaches the saved
streamed child levels, then adds detailed street-facing frontage packs, kerbs,
pavements, drains, asphalt overlays, and a small amount of static Lagos
roadside dressing around the canonical destination corridors.

It preserves the earlier connected parent map and never claims gameplay or
runtime readiness.
"""
import gc
import hashlib
import json
import math
import os
import copy
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import sys

import unreal

sys.dont_write_bytecode = True
ROOT = Path("C:/dev/cribtest-game")
SCRIPTS = ROOT / "apps/companion-unreal/Scripts"
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
REALISM_OUT = OUT / "realism"
CONNECTED_RECEIPT = OUT / "town-centre-connected-neighbourhood-v1.json"
DISTRICT_SELECTION = OUT / "town-centre-district-selection.json"
V3_ASSET_RECEIPT = REALISM_OUT / "frontage-v3-native-assets.json"
sys.path.insert(0, str(SCRIPTS))
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import batch_frontage as batching
from WorldGeneration.canonical import References, city_plan

SOURCE_MAP = "/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011"
TARGET_MAP = "/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01"
DETAIL_MAP = "/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01"
RECEIPT_NAME = "town-centre-street-pass-01.json"
OWNER = "cribtest11-world-generation-20261010/town-centre-street-pass-01"
TAG = "Baddie.TownCentreStreetPass01"
ROAD_MESH = "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/DenseStreet_v1/Meshes/SM_RoadStraight10m"
ROAD_MATERIAL = "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/DenseStreet_v1/Materials/M_Asphalt_ISM"
DANFO = "/Game/BaddieGeneratedWorld/Presentation/StreetProps_v1/SM_Danfo"
KIOSK = "/Game/BaddieGeneratedWorld/Presentation/StreetProps_v1/SM_Kiosk"
CONNECTED_DISTRICT_LEVEL = "/Game/BaddieGeneratedWorld/R_35d9213d603f458bdc1355d2/Maps/L_city_districts_town_centre_c904bd18ec"
REFINE_RECEIPT_NAME = "town-centre-street-pass-05-refine04.json"
PLACEHOLDER_CLEANUP_RECEIPT_NAME = "town-centre-street-pass-06-placeholder-cleanup.json"
ALL_CLOSED_BOX_CLEANUP_RECEIPT_NAME = "town-centre-street-pass-07-all-closed-box-cleanup.json"
ADJOINING_MESH_CLEANUP_RECEIPT_NAME = "town-centre-street-pass-08-adjoining-meshcell-cleanup.json"

FRONTAGE_RECEIPT = REALISM_OUT / "frontage-v2-ism-v1.json"
FOCUS_RADIUS_PAD_METRES = 55.0
PRIORITY_PARCELS = {
    "market-road-2", "market-road-4", "market-road-5", "market-road-6", "market-road-7",
    "cross-town-road-17", "cross-town-road-18", "cross-town-road-19",
    "garden-avenue-11", "garden-avenue-13", "garden-avenue-14",
    "civic-street-2", "civic-street-4", "civic-street-5",
    "library-lane-2", "library-lane-3", "library-lane-4",
}
PASS_BY_CONTEXT_PARCELS = {
    "market-road-5", "market-road-6", "market-road-7", "market-road-8", "market-road-9",
    "market-road-10", "market-road-12",
    "civic-street-2", "civic-street-4", "civic-street-5", "civic-street-6",
    "unity-avenue-3", "unity-avenue-9",
}
ADJOINING_CLOSED_MESH_LEVELS = {
    "/Game/BaddieGeneratedWorld/R_8a7214b59aa169bc7ccb240a/Maps/L_city_dance_again_exterior_b95522161c": {
        "mesh-cell:-1:-1:3651a566d87b",
        "mesh-cell:-1:-1:6ba0c451f54f",
        "mesh-cell:-1:-1:6cffe552c1fc",
        "mesh-cell:-1:-1:72f4c98c41bd",
        "mesh-cell:-1:-1:f4b6d8f83942",
    },
    "/Game/BaddieGeneratedWorld/R_658b0b3d7c41d58ae50d820f/Maps/L_city_civic_terrace_exterior_6fdc438d5f": {
        "mesh-cell:0:-2:0453e67b17d8",
        "mesh-cell:0:-2:2540caa2a358",
        "mesh-cell:0:-2:2bd0a2983324",
        "mesh-cell:0:-2:35c994cb4ce1",
        "mesh-cell:0:-2:4b5b48b9a8cd",
        "mesh-cell:0:-2:57733a202e67",
        "mesh-cell:0:-2:7947b7840c55",
        "mesh-cell:0:-2:81c723f8a09e",
        "mesh-cell:0:-2:a87e83741d44",
        "mesh-cell:0:-2:ac2500549624",
        "mesh-cell:0:-2:c28d82f66e94",
        "mesh-cell:0:-2:c7162a236889",
        "mesh-cell:0:-2:cad68309650d",
        "mesh-cell:0:-2:d3625fbf6a93",
        "mesh-cell:0:-2:faca9b073f55",
    },
}
PACK_SPACING_CM = 1180.0
PLACEHOLDER_BLOCKER_CANONICAL_IDS = {
    "instances:box:-3:-2:bd1d46717507",
    "instances:box:-3:-2:f796b7cc9f18",
    "instances:box:-3:-1:1022331955f0",
    "instances:box:-3:-1:45686af65818",
    "instances:box:-3:-1:46db624a079d",
    "instances:box:-3:-1:a51bb30745ae",
    "instances:box:-2:-2:15512fdcdfbc",
    "instances:box:-2:-2:706abeee8c6a",
    "instances:box:-2:-2:9c522f4c1089",
    "instances:box:-2:-2:b7fc771cdb76",
    "instances:box:-2:-1:552b74f677eb",
    "instances:box:-2:-1:a1cd54801af9",
    "instances:box:-2:-1:bfceae7164ed",
    "instances:box:-2:-1:c081424d2567",
    "instances:box:-1:-4:2665a9e1551b",
    "instances:box:-1:-3:24cb395fc375",
    "instances:box:-1:-3:31abc32c7c94",
    "instances:box:-1:-3:4c65f67c3dff",
    "instances:box:-1:-3:671b352b3ae3",
    "instances:box:-1:-3:d9a0a3ff2b85",
    "instances:box:-1:-2:22683553cf6a",
    "instances:box:-1:-2:6ce6a2c7a896",
    "instances:box:-1:-2:7e8a0a2eeb97",
    "instances:box:-1:-2:95cfcb3335f7",
    "instances:box:0:-4:99e2680f4e57",
    "instances:box:0:-4:aa4de039b2ee",
    "instances:box:0:-3:38fd06816917",
    "instances:box:0:-3:93f4f6dcf842",
    "instances:box:0:-3:ff8ffaf68ea6",
}

ROAD_WINDOWS = [
    {"id": "market-core", "roads": {"market-road", "unity-avenue", "garden-avenue"}, "center": (-236.0, -57.0), "radius": 150.0},
    {"id": "library-cross-town", "roads": {"library-lane", "cross-town-road", "civic-street"}, "center": (-278.0, -771.0), "radius": 145.0},
    {"id": "civic-node", "roads": {"civic-street", "unity-avenue", "station-road"}, "center": (34.5, -271.0), "radius": 140.0},
    {"id": "garden-node", "roads": {"garden-avenue", "station-road"}, "center": (500.0, -145.0), "radius": 135.0},
]

CAMERAS = [
    {
        "id": "market-road-junction",
        "locationCm": [-18200.0, -23600.0, 165.0],
        "rotation": {"pitch": 2.5, "yaw": 34.0, "roll": 0.0},
        "fov": 68.0,
    },
    {
        "id": "library-lane-approach",
        "locationCm": [-76000.0, -25800.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 55.0, "roll": 0.0},
        "fov": 70.0,
    },
    {
        "id": "civic-street-approach",
        "locationCm": [-28400.0, -2600.0, 165.0],
        "rotation": {"pitch": 2.0, "yaw": 8.0, "roll": 0.0},
        "fov": 68.0,
    },
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def stable_hash_int(value):
    return int(hashlib.sha256(value.encode("utf-8")).hexdigest()[:8], 16)


def vector(value):
    return [value.x, value.y, value.z]


def rotator(spec):
    return unreal.Rotator(pitch=spec["pitch"], yaw=spec["yaw"], roll=spec["roll"])


def load_frontage_data():
    data = json.loads(FRONTAGE_RECEIPT.read_text(encoding="utf-8"))
    common.verify_files(data["files"])
    return data


def load_v3_assets():
    data = json.loads(V3_ASSET_RECEIPT.read_text(encoding="utf-8"))
    common.verify_files(data["files"])
    return data


def v3_modules():
    imports = load_v3_assets()
    return {
        row["module"]: {
            "mesh": row["package"],
            "materials": row["materialPackages"],
            "sourceFBXSha256": row["source"]["sha256"],
        }
        for row in imports["meshes"]
    }


def clear_python_refs():
    for name in ("last_type", "last_value", "last_traceback", "last_exc"):
        if hasattr(sys, name):
            setattr(sys, name, None)
    gc.collect()


def ensure_identity(expected_pid):
    assert os.getpid() == expected_pid, "Editor PID changed"
    assert Path(unreal.Paths.project_dir()).resolve() == (ROOT / "apps/companion-unreal").resolve()
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is None


def assets():
    return unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)


def levels():
    return unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def active_world_package():
    current_world = world()
    return current_world.get_path_name().split(".")[0] if current_world else None


def dirty_names():
    return [p.get_name() for p in list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
            + list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())]


def package_files(package, is_map=False):
    primary = ROOT / "apps/companion-unreal/Content" / (package.removeprefix("/Game/") + (".umap" if is_map else ".uasset"))
    assert primary.is_file(), package
    extra = [primary.with_suffix(s) for s in (".uexp", ".ubulk", ".uptnl") if primary.with_suffix(s).is_file()]
    return {str(p.relative_to(ROOT)).replace("\\", "/"): sha(p) for p in [primary] + extra}


def connected_receipt():
    return json.loads(CONNECTED_RECEIPT.read_text(encoding="utf-8"))


def district_selection():
    return json.loads(DISTRICT_SELECTION.read_text(encoding="utf-8"))


def snapshot_environment():
    labels = set(connected_receipt()["environmentActors"])
    result = []
    for actor in actors().get_all_level_actors():
        label = actor.get_actor_label()
        if label not in labels:
            continue
        record = {
            "label": label,
            "class": actor.get_class().get_name(),
            "locationCm": vector(actor.get_actor_location()),
            "rotation": {"pitch": actor.get_actor_rotation().pitch, "yaw": actor.get_actor_rotation().yaw, "roll": actor.get_actor_rotation().roll},
        }
        if record["class"] == "DirectionalLight":
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            colour = component.get_light_color()
            record["component"] = {
                "mobility": str(component.get_editor_property("mobility")),
                "intensity": component.get_editor_property("intensity"),
                "lightColour": [colour.r, colour.g, colour.b],
                "atmosphereSunLight": component.get_editor_property("atmosphere_sun_light"),
            }
        elif record["class"] == "SkyLight":
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            record["component"] = {
                "mobility": str(component.get_editor_property("mobility")),
                "intensity": component.get_editor_property("intensity"),
            }
        elif record["class"] == "ExponentialHeightFog":
            component = actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
            record["component"] = {
                "fogDensity": component.get_editor_property("fog_density"),
                "fogHeightFalloff": component.get_editor_property("fog_height_falloff"),
            }
        elif record["class"] == "PostProcessVolume":
            settings = actor.get_editor_property("settings")
            record["unbound"] = actor.get_editor_property("unbound")
            record["component"] = {
                "overrideAutoExposureMethod": settings.override_auto_exposure_method,
                "autoExposureMethod": str(settings.auto_exposure_method),
                "overrideAutoExposureApplyPhysicalCameraExposure": settings.override_auto_exposure_apply_physical_camera_exposure,
                "autoExposureApplyPhysicalCameraExposure": settings.auto_exposure_apply_physical_camera_exposure,
                "overrideAutoExposureBias": settings.override_auto_exposure_bias,
                "autoExposureBias": settings.auto_exposure_bias,
            }
        result.append(record)
    assert labels == {item["label"] for item in result}, "Missing connected environment actors"
    return sorted(result, key=lambda item: item["label"])


def spawn_environment(records):
    actors_subsystem = actors()
    for record in records:
        location = unreal.Vector(*record["locationCm"])
        rotation = rotator(record["rotation"])
        if record["class"] == "DirectionalLight":
            actor = actors_subsystem.spawn_actor_from_class(unreal.DirectionalLight, location, rotation)
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
            component.set_intensity(record["component"]["intensity"])
            component.set_light_color(unreal.LinearColor(*record["component"]["lightColour"], 1.0))
            component.set_atmosphere_sun_light(record["component"]["atmosphereSunLight"])
        elif record["class"] == "SkyAtmosphere":
            actor = actors_subsystem.spawn_actor_from_class(unreal.SkyAtmosphere, location, rotation)
        elif record["class"] == "SkyLight":
            actor = actors_subsystem.spawn_actor_from_class(unreal.SkyLight, location, rotation)
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
            component.set_intensity(record["component"]["intensity"])
            component.recapture_sky()
        elif record["class"] == "ExponentialHeightFog":
            actor = actors_subsystem.spawn_actor_from_class(unreal.ExponentialHeightFog, location, rotation)
            component = actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
            component.set_editor_property("fog_density", record["component"]["fogDensity"])
            component.set_editor_property("fog_height_falloff", record["component"]["fogHeightFalloff"])
        elif record["class"] == "PostProcessVolume":
            actor = actors_subsystem.spawn_actor_from_class(unreal.PostProcessVolume, location, rotation)
            actor.set_editor_property("unbound", record.get("unbound", True))
            settings = actor.get_editor_property("settings")
            component = record["component"]
            settings.override_auto_exposure_method = component["overrideAutoExposureMethod"]
            settings.auto_exposure_method = unreal.AutoExposureMethod.AEM_MANUAL
            settings.override_auto_exposure_apply_physical_camera_exposure = component["overrideAutoExposureApplyPhysicalCameraExposure"]
            settings.auto_exposure_apply_physical_camera_exposure = component["autoExposureApplyPhysicalCameraExposure"]
            settings.override_auto_exposure_bias = component["overrideAutoExposureBias"]
            settings.auto_exposure_bias = component["autoExposureBias"]
            actor.set_editor_property("settings", settings)
        else:
            raise AssertionError("Unsupported environment actor class: " + record["class"])
        actor.set_actor_label(record["label"])


def attach_child_levels():
    receipt = connected_receipt()
    current_world = world()
    attached = []
    for child in receipt["childLevels"]:
        streaming = unreal.EditorLevelUtils.add_level_to_world(current_world, child["package"], unreal.LevelStreamingDynamic)
        assert streaming is not None, child["package"]
        streaming.set_editor_property("disable_distance_streaming", True)
        attached.append({
            "job": child["job"],
            "identity": child["identity"],
            "package": child["package"],
            "role": child["role"],
            "streamingObject": streaming.get_path_name(),
            "loadedLevel": streaming.get_loaded_level().get_path_name() if streaming.get_loaded_level() else None,
            "disableDistanceStreaming": True,
        })
    return attached


def create_connected_parent_from_receipt(expected_pid):
    ensure_identity(expected_pid)
    assert world().get_path_name().split(".")[0] == SOURCE_MAP, "Expected the existing connected town-centre parent to be loaded"
    assert not dirty_names(), "Do not duplicate or switch while packages are dirty"
    assert not assets().does_asset_exist(TARGET_MAP), "Target map already exists: " + TARGET_MAP
    environment = copy.deepcopy(snapshot_environment())
    clear_python_refs()
    assert levels().new_level(TARGET_MAP, is_partitioned_world=False), "Failed to create new connected parent"
    spawn_environment(environment)
    return attach_child_levels()


def prepare_new_parent(expected_pid):
    child_levels = create_connected_parent_from_receipt(expected_pid)
    assert world().get_path_name().split(".")[0] == TARGET_MAP
    assert not assets().does_asset_exist(DETAIL_MAP), "Detail map already exists: " + DETAIL_MAP
    detail_streaming = unreal.EditorLevelUtils.create_new_streaming_level(unreal.LevelStreamingDynamic, DETAIL_MAP, False)
    assert detail_streaming is not None, "Failed to create additive street detail level"
    detail_streaming.set_editor_property("disable_distance_streaming", True)
    child_levels.append({
        "job": "city/districts/town-centre/streetpass01-detail01",
        "identity": "editor-authored-streetpass01-detail01",
        "package": DETAIL_MAP,
        "role": "street-detail-additive",
        "streamingObject": detail_streaming.get_path_name(),
        "loadedLevel": detail_streaming.get_loaded_level().get_path_name() if detail_streaming.get_loaded_level() else None,
        "disableDistanceStreaming": True,
    })
    return child_levels, detail_streaming


def set_manual_exposure():
    post = next(a for a in actors().get_all_level_actors() if a.get_class().get_name() == "PostProcessVolume")
    settings = post.get_editor_property("settings")
    settings.override_auto_exposure_method = True
    settings.auto_exposure_method = unreal.AutoExposureMethod.AEM_MANUAL
    settings.override_auto_exposure_apply_physical_camera_exposure = True
    settings.auto_exposure_apply_physical_camera_exposure = False
    settings.override_auto_exposure_bias = True
    # Keep the streets readable in the software-rendered daylight shell while
    # avoiding the full connected-parent whiteout from the earlier capture pass.
    settings.auto_exposure_bias = 0.0
    post.set_editor_property("settings", settings)
    sky = next(a for a in actors().get_all_level_actors() if a.get_actor_label() == "TownCentreConnected_SkyLight")
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_intensity(1.0)
    sky_component.recapture_sky()


def canonical():
    refs = References()
    plan = city_plan(refs)
    parcels = {p["id"]: p for p in plan["parcels"] if p["district"] == "town-centre"}
    roads = {r["id"]: r for r in plan["roads"]}
    return parcels, roads


def parcel_road(parcel, roads):
    road_id = parcel.get("streetId")
    if road_id:
        return roads[road_id]
    road_name = parcel.get("street")
    assert road_name, f"Parcel has no road binding: {parcel['id']}"
    for road in roads.values():
        if road["name"] == road_name:
            return road
    raise KeyError(f"Unknown road for parcel {parcel['id']}: {road_name}")


def source_to_ue(x_metres, z_metres, y_metres=0.0):
    return unreal.Vector(z_metres * 100.0, x_metres * 100.0, y_metres * 100.0)


def ue_xy_from_source(x_metres, z_metres):
    return (z_metres * 100.0, x_metres * 100.0)


def road_direction_yaw(dir_xy):
    angle = math.degrees(math.atan2(dir_xy[1], dir_xy[0]))
    return angle + 90.0


def normalize2(vec):
    length = math.hypot(vec[0], vec[1])
    assert length > 0.001
    return (vec[0] / length, vec[1] / length)


def add2(a, b):
    return (a[0] + b[0], a[1] + b[1])


def sub2(a, b):
    return (a[0] - b[0], a[1] - b[1])


def mul2(v, s):
    return (v[0] * s, v[1] * s)


def dist2(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def nearest_point_on_segment(p, a, b):
    ab = sub2(b, a)
    ab_len_sq = ab[0] * ab[0] + ab[1] * ab[1]
    if ab_len_sq <= 0.001:
        return a, 0.0
    t = max(0.0, min(1.0, ((p[0] - a[0]) * ab[0] + (p[1] - a[1]) * ab[1]) / ab_len_sq))
    return add2(a, mul2(ab, t)), t


def nearest_road_pose(parcel, road):
    p = ue_xy_from_source(parcel["x"], parcel["z"])
    best = None
    road_points = [ue_xy_from_source(pt[0], pt[1]) for pt in road["points"]]
    for index in range(len(road_points) - 1):
        a, b = road_points[index], road_points[index + 1]
        point, t = nearest_point_on_segment(p, a, b)
        distance = dist2(p, point)
        if best is None or distance < best["distance"]:
            direction = normalize2(sub2(b, a))
            right = (-direction[1], direction[0])
            best = {"point": point, "direction": direction, "right": right, "distance": distance, "segmentIndex": index, "t": t}
    assert best is not None
    return best


def selected_parcel_ids(parcels, roads):
    selected = set(PRIORITY_PARCELS)
    for parcel_id, parcel in parcels.items():
        if parcel["kind"] not in {"shop", "apartment", "townhouse"}:
            continue
        road = parcel_road(parcel, roads)
        for window in ROAD_WINDOWS:
            if road["id"] not in window["roads"]:
                continue
            if dist2((parcel["x"], parcel["z"]), tuple(window["center"])) <= window["radius"] + FOCUS_RADIUS_PAD_METRES:
                selected.add(parcel_id)
                break
    return sorted(parcel_id for parcel_id in selected if parcel_id in parcels)


def frontage_templates(frontage_data):
    templates = {}
    for row in frontage_data["placements"]:
        templates.setdefault(row["module"], row)
    return templates


def clone_row(template, source_id, location_cm, yaw, module=None):
    item = json.loads(json.dumps(template))
    item["sourceId"] = source_id
    item["sourceIdBasis"] = "town-centre additive street pass; source-derived from accepted Yaba frontage receipt"
    item["module"] = module or item["module"]
    item["transform"] = {
        "locationCm": [float(location_cm[0]), float(location_cm[1]), float(location_cm[2])],
        "rotation": {"pitch": 0.0, "yaw": float(yaw), "roll": 0.0},
        "scale": [1.0, 1.0, 1.0],
    }
    return item


def transform_local(origin_cm, yaw_deg, local_cm):
    yaw = math.radians(yaw_deg)
    x = origin_cm[0] + local_cm[0] * math.cos(yaw) - local_cm[1] * math.sin(yaw)
    y = origin_cm[1] + local_cm[0] * math.sin(yaw) + local_cm[1] * math.cos(yaw)
    z = origin_cm[2] + local_cm[2]
    return [x, y, z]


def apply_v3_variant(row, variants, parcel_id, seed_suffix, v3_lookup):
    choice = variants[stable_hash_int(parcel_id + ":" + seed_suffix) % len(variants)]
    module = v3_lookup.get(choice)
    if not module:
        return row
    row["module"] = choice
    row["mesh"] = module["mesh"]
    row["materials"] = module["materials"]
    row["sourceFBXSha256"] = module["sourceFBXSha256"]
    return row


def upgrade_frontage_row(row, parcel_id, v3_lookup):
    module_name = row["module"]
    if module_name in {"ShopBay", "ResidentialBay"}:
        variants = [module_name, module_name + "_V3Left", module_name + "_V3Right"]
        return apply_v3_variant(row, variants, parcel_id, row["sourceId"], v3_lookup)
    if module_name == "Window1300":
        return apply_v3_variant(row, ["Window1300", "Window1300_V3Curtain", "Window1300_V3Louvres"], parcel_id, row["sourceId"], v3_lookup)
    if module_name == "ACServiceAssembly":
        return apply_v3_variant(row, ["ACServiceAssembly", "ACServiceAssembly_V3Compact", "ACServiceAssembly_V3Cover"], parcel_id, row["sourceId"], v3_lookup)
    module = v3_lookup.get(module_name)
    if module:
        row["mesh"] = module["mesh"]
        row["materials"] = module["materials"]
        row["sourceFBXSha256"] = module["sourceFBXSha256"]
    return row


def v3_stock_rows(parcel_id, bay, origin_cm, yaw_deg, v3_lookup):
    stock_specs = [
        ("V3_StockBottle", [126.0, -390.0 * bay - 110.0, 108.0], 0.0),
        ("V3_StockJar", [124.0, -390.0 * bay - 180.0, 112.0], 6.0),
        ("V3_StockTin", [124.0, -390.0 * bay - 250.0, 112.0], -8.0),
        ("V3_PaperPackage", [60.0, -390.0 * bay - 96.0, 3.0], 14.0),
        ("V3_TimberCrate", [18.0, -390.0 * bay - 232.0, 0.0], 90.0),
        ("V3_GrainSack", [28.0, -390.0 * bay - 302.0, 0.0], -30.0),
    ]
    rows = []
    for module_name, local, yaw_offset in stock_specs:
        module = v3_lookup.get(module_name)
        if not module:
            continue
        row = {
            "sourceId": f"{parcel_id}:bay:{bay}:{module_name}",
            "sourceIdBasis": "town-centre additive street pass; source-derived from accepted Yaba frontage receipt",
            "module": module_name,
            "transform": {
                "locationCm": transform_local(origin_cm, yaw_deg, local),
                "rotation": {"pitch": 0.0, "yaw": float(yaw_deg + yaw_offset), "roll": 0.0},
                "scale": [1.0, 1.0, 1.0],
            },
            "mesh": module["mesh"],
            "materials": module["materials"],
            "sourceFBXSha256": module["sourceFBXSha256"],
        }
        rows.append(row)
    return rows


def place_frontage_pack(rows, base_rows, parcel_id, origin_cm, yaw_deg, storeys, closed_bays):
    skip = {"Pavement1000", "Drain1000", "DrainCover1000", "Kerb1000", "RoadJunction", "JunctionInlet"}
    for base in base_rows:
        module = base["module"]
        if module in skip or module in {"OpenShopInterior", "Shutter2800"}:
            continue
        rows.append(clone_row(
            base,
            f"{parcel_id}:{module}:{len(rows)}",
            transform_local(origin_cm, yaw_deg, base["transform"]["locationCm"]),
            yaw_deg,
        ))
    for bay in range(3):
        module = "Shutter2800" if bay in closed_bays else "OpenShopInterior"
        local = [0.0, -390.0 * bay - (55.0 if module == "Shutter2800" else 195.0), 0.0]
        base = next(r for r in base_rows if r["module"] == module)
        rows.append(clone_row(
            base,
            f"{parcel_id}:bay:{bay}:{module}",
            transform_local(origin_cm, yaw_deg, local),
            yaw_deg,
        ))
    if storeys >= 3:
        for base in base_rows:
            module = base["module"]
            if module in skip or module in {"OpenShopInterior", "RainwaterDownpipe", "PlasterRepair", "Shutter2800"}:
                continue
            local = list(base["transform"]["locationCm"])
            local[2] += 644.5
            if module == "ShopBay":
                module = "ResidentialBay"
            rows.append(clone_row(
                next(r for r in base_rows if r["module"] == module),
                f"{parcel_id}:upper:{module}:{len(rows)}",
                transform_local(origin_cm, yaw_deg, local),
                yaw_deg,
                module=module,
            ))


def frontage_repeat_count(parcel_id, parcel):
    width_metres = parcel["width"]
    if width_metres >= 34.0:
        count = 3
    elif width_metres >= 18.0:
        count = 2
    else:
        count = 1
    if parcel_id in PASS_BY_CONTEXT_PARCELS:
        count += 1
    return min(count, 4)


def repeated_pack_origins(origin_xy, direction, repeat_count):
    center_index = (repeat_count - 1) * 0.5
    origins = []
    for index in range(repeat_count):
        offset_cm = (index - center_index) * PACK_SPACING_CM
        shifted_xy = add2(origin_xy, mul2(direction, offset_cm))
        origins.append([shifted_xy[0], shifted_xy[1], 0.0])
    return origins


def frontage_rows(parcels, roads, frontage_data, v3_lookup):
    base_rows = frontage_data["placements"]
    rows = []
    pack_records = []
    for parcel_id in selected_parcel_ids(parcels, roads):
        parcel = parcels[parcel_id]
        road = parcel_road(parcel, roads)
        pose = nearest_road_pose(parcel, road)
        parcel_xy = ue_xy_from_source(parcel["x"], parcel["z"])
        side_sign = 1.0 if ((parcel_xy[0] - pose["point"][0]) * pose["right"][0] + (parcel_xy[1] - pose["point"][1]) * pose["right"][1]) >= 0 else -1.0
        road_half = road["width"] * 50.0
        curb_offset = road_half + 339.0
        center_shift = 585.0
        direction = pose["direction"]
        right = pose["right"]
        origin_xy = add2(pose["point"], mul2(right, side_sign * curb_offset))
        origin_xy = add2(origin_xy, mul2(direction, -center_shift if side_sign > 0 else center_shift))
        yaw = road_direction_yaw(direction) + (0.0 if side_sign > 0 else 180.0)
        width_metres = parcel["width"]
        storeys = 3 if parcel["style"] in {"civic", "urban-apartment", "courtyard"} or width_metres >= 22.0 else 2
        repeat_count = frontage_repeat_count(parcel_id, parcel)
        pack_origins = repeated_pack_origins(origin_xy, direction, repeat_count)
        start_index = len(rows)
        for repeat_index, origin_cm in enumerate(pack_origins):
            closed_bays = {stable_hash_int(f"{parcel_id}:repeat:{repeat_index}") % 3}
            if parcel["kind"] == "shop" and parcel["style"] in {"corner-shop", "mixed-use"}:
                closed_bays = {stable_hash_int(f"{parcel_id}:repeat:{repeat_index}:shop") % 3}
            place_frontage_pack(rows, base_rows, f"{parcel_id}:r{repeat_index}", origin_cm, yaw, storeys, closed_bays)
        for index in range(start_index, len(rows)):
            source_basis = rows[index]["sourceId"].split(":r", 1)[0]
            rows[index] = upgrade_frontage_row(rows[index], source_basis, v3_lookup)
            if rows[index]["module"] == "OpenShopInterior":
                parts = rows[index]["sourceId"].split(":")
                bay = int(parts[3])
                repeat_token = parts[1]
                repeat_index = int(repeat_token.removeprefix("r"))
                rows.extend(v3_stock_rows(source_basis, bay + repeat_index * 3, pack_origins[repeat_index], yaw, v3_lookup))
        pack_records.append({
            "parcelId": parcel_id,
            "roadId": road["id"],
            "originCm": [origin_xy[0], origin_xy[1], 0.0],
            "packOriginsCm": pack_origins,
            "yaw": yaw,
            "storeys": storeys,
            "repeatCount": repeat_count,
        })
    return rows, pack_records


def road_rows(frontage_data, v3_lookup):
    templates = frontage_templates(frontage_data)
    rows = []
    for window in ROAD_WINDOWS:
        center_xy = ue_xy_from_source(window["center"][0], window["center"][1])
        radius_cm = window["radius"] * 100.0
        for road_id in sorted(window["roads"]):
            road = canonical_data[1][road_id]
            points = [ue_xy_from_source(pt[0], pt[1]) for pt in road["points"]]
            road_half = road["width"] * 50.0
            offsets = {
                "Kerb1000": road_half + 11.0,
                "Drain1000": road_half + 64.0,
                "DrainCover1000": road_half + 64.0,
                "Pavement1000": road_half + 296.0,
            }
            for seg_index in range(len(points) - 1):
                a, b = points[seg_index], points[seg_index + 1]
                seg = sub2(b, a)
                seg_length = math.hypot(seg[0], seg[1])
                if seg_length < 100.0:
                    continue
                direction = normalize2(seg)
                right = (-direction[1], direction[0])
                yaw = road_direction_yaw(direction)
                steps = int(seg_length // 100.0)
                road_steps = int(seg_length // 1000.0)
                for step in range(road_steps):
                    start = add2(a, mul2(direction, step * 1000.0))
                    mid = add2(start, mul2(direction, 500.0))
                    if dist2(mid, center_xy) > radius_cm:
                        continue
                    road_origin = add2(start, mul2(right, road_half + 339.0))
                    rows.append({
                        "kind": "road",
                        "mesh": ROAD_MESH,
                        "material": ROAD_MATERIAL,
                        "locationCm": [road_origin[0], road_origin[1], 0.0],
                        "rotation": {"pitch": 0.0, "yaw": yaw, "roll": 0.0},
                        "scale": [max(1.0, road["width"] / 5.0), 1.0, 1.0],
                        "label": f"TC_Road_{road_id}_{window['id']}_{seg_index}_{step}",
                    })
                for step in range(steps):
                    start = add2(a, mul2(direction, step * 100.0))
                    end = add2(a, mul2(direction, (step + 1) * 100.0))
                    mid = add2(start, mul2(direction, 50.0))
                    if dist2(mid, center_xy) > radius_cm:
                        continue
                    for module, offset in offsets.items():
                        if module == "DrainCover1000" and step % 4 != 0:
                            continue
                        row_a = clone_row(
                            templates[module],
                            f"{window['id']}:{road_id}:{seg_index}:{step}:{module}:A",
                            [start[0] + right[0] * offset, start[1] + right[1] * offset, -5.0 if module != 'Kerb1000' else -20.0],
                            yaw,
                        )
                        row_b = clone_row(
                            templates[module],
                            f"{window['id']}:{road_id}:{seg_index}:{step}:{module}:B",
                            [end[0] - right[0] * offset, end[1] - right[1] * offset, -5.0 if module != 'Kerb1000' else -20.0],
                            yaw + 180.0,
                        )
                        rows.append(upgrade_frontage_row(row_a, road_id, v3_lookup))
                        rows.append(upgrade_frontage_row(row_b, road_id, v3_lookup))
    return rows


def spawn_frontage_rows(rows, material_derivatives):
    created = []
    for row in rows:
        actor = actors().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*row["transform"]["locationCm"]), rotator(row["transform"]["rotation"]))
        actor.set_actor_label("TC_" + row["module"] + "_" + str(len(created)))
        actor.tags = [unreal.Name(TAG), unreal.Name("Baddie.EditorOnlyArt"), unreal.Name("Baddie.NoRuntimeAuthority")]
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(assets().load_asset(row["mesh"]))
        for slot, material_path in enumerate(row["materials"]):
            target = material_derivatives.get(material_path, material_path)
            component.set_material(slot, assets().load_asset(target))
        actor.set_actor_scale3d(unreal.Vector(*row["transform"]["scale"]))
        created.append({
            "label": actor.get_actor_label(),
            "module": row["module"],
            "sourceId": row["sourceId"],
            "actor": actor.get_path_name(),
            "mesh": row["mesh"],
            "materials": row["materials"],
            "locationCm": row["transform"]["locationCm"],
            "yaw": row["transform"]["rotation"]["yaw"],
        })
    return created


def spawn_road_rows(rows):
    created = []
    for row in rows:
        if "transform" in row:
            actor = actors().spawn_actor_from_class(
                unreal.StaticMeshActor,
                unreal.Vector(*row["transform"]["locationCm"]),
                rotator(row["transform"]["rotation"]),
            )
            label = "TC_" + row["module"] + "_" + str(len(created))
            actor.set_actor_label(label)
            actor.tags = [unreal.Name(TAG), unreal.Name("Baddie.EditorOnlyArt"), unreal.Name("Baddie.NoRuntimeAuthority")]
            component = actor.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(assets().load_asset(row["mesh"]))
            for slot, material_path in enumerate(row["materials"]):
                component.set_material(slot, assets().load_asset(material_path))
            actor.set_actor_scale3d(unreal.Vector(*row["transform"]["scale"]))
            created.append({
                "label": label,
                "actor": actor.get_path_name(),
                "mesh": row["mesh"],
                "materials": row["materials"],
                "locationCm": row["transform"]["locationCm"],
                "yaw": row["transform"]["rotation"]["yaw"],
                "scale": row["transform"]["scale"],
                "module": row["module"],
                "sourceId": row["sourceId"],
            })
            continue
        actor = actors().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*row["locationCm"]), rotator(row["rotation"]))
        actor.set_actor_label(row["label"])
        actor.tags = [unreal.Name(TAG), unreal.Name("Baddie.EditorOnlyArt"), unreal.Name("Baddie.NoRuntimeAuthority")]
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(assets().load_asset(row["mesh"]))
        component.set_material(0, assets().load_asset(row["material"]))
        actor.set_actor_scale3d(unreal.Vector(*row["scale"]))
        created.append({
            "label": row["label"],
            "actor": actor.get_path_name(),
            "mesh": row["mesh"],
            "material": row["material"],
            "locationCm": row["locationCm"],
            "yaw": row["rotation"]["yaw"],
            "scale": row["scale"],
        })
    return created


def spawn_static_dressing():
    dressing = [
        {"label": "TC_Static_Danfo_MarketRoad", "asset": DANFO, "locationCm": [-6800.0, -23100.0, 0.0], "yaw": 122.0},
        {"label": "TC_Static_Kiosk_MarketRoad", "asset": KIOSK, "locationCm": [-17600.0, -24250.0, 0.0], "yaw": 38.0},
        {"label": "TC_Static_Danfo_MarketRoad_Stop", "asset": DANFO, "locationCm": [-21400.0, -26500.0, 0.0], "yaw": 32.0},
        {"label": "TC_Static_Kiosk_CivicStreet", "asset": KIOSK, "locationCm": [-25900.0, 6200.0, 0.0], "yaw": 355.0},
        {"label": "TC_Static_Kiosk_CivicStreet_02", "asset": KIOSK, "locationCm": [-30400.0, -3200.0, 0.0], "yaw": 8.0},
        {"label": "TC_Static_Danfo_CivicStreet", "asset": DANFO, "locationCm": [-33000.0, -2200.0, 0.0], "yaw": 10.0},
        {"label": "TC_Static_Danfo_CrossTown", "asset": DANFO, "locationCm": [-81200.0, -34600.0, 0.0], "yaw": 77.0},
        {"label": "TC_Static_Kiosk_GardenAvenue", "asset": KIOSK, "locationCm": [-15100.0, 50300.0, 0.0], "yaw": 35.0},
    ]
    created = []
    for spec in dressing:
        actor = actors().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*spec["locationCm"]), unreal.Rotator(pitch=0.0, yaw=spec["yaw"], roll=0.0))
        actor.set_actor_label(spec["label"])
        actor.tags = [unreal.Name(TAG), unreal.Name("Baddie.EditorOnlyArt"), unreal.Name("Baddie.NoRuntimeAuthority")]
        actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(assets().load_asset(spec["asset"]))
        created.append(dict(spec, actor=actor.get_path_name()))
    return created


def loaded_level_by_package(package):
    current_world = world()
    for level in unreal.EditorLevelUtils.get_levels(current_world):
        if level.get_outer().get_path_name().split(".")[0] == package:
            return level
    raise AssertionError("Loaded level not found: " + package)


def streaming_level_by_package(package):
    streaming = unreal.GameplayStatics.get_streaming_level(world(), unreal.Name(package))
    assert streaming is not None, "Streaming level not found: " + package
    return streaming


def make_level_current(package):
    unreal.EditorLevelUtils.make_level_current(streaming_level_by_package(package))


def clear_detail_actors():
    prefix = DETAIL_MAP + "." + DETAIL_MAP.rsplit("/", 1)[-1] + ":PersistentLevel."
    doomed = []
    for actor in actors().get_all_level_actors():
        if prefix not in actor.get_path_name():
            continue
        if TAG not in {str(tag) for tag in actor.tags}:
            continue
        doomed.append(actor)
    for actor in doomed:
        assert actors().destroy_actor(actor), actor.get_actor_label()
    return len(doomed)


def placeholder_canonical_id(actor):
    for tag in actor.tags:
        text = str(tag)
        if text.startswith("Baddie.CanonicalId="):
            return text.split("=", 1)[1]
    return None


def remove_closed_placeholder_blockers():
    affected = []
    for actor in actors().get_all_level_actors():
        canonical_id = placeholder_canonical_id(actor)
        if canonical_id not in PLACEHOLDER_BLOCKER_CANONICAL_IDS:
            continue
        tags = {str(tag) for tag in actor.tags}
        if "Baddie.Readiness=Closed" not in tags:
            continue
        origin, extent = actor.get_actor_bounds(False)
        record = {
            "label": actor.get_actor_label(),
            "actor": actor.get_path_name(),
            "canonicalId": canonical_id,
            "levelPackage": actor.get_outer().get_path_name().split(".")[0],
            "origin": [origin.x, origin.y, origin.z],
            "extent": [extent.x, extent.y, extent.z],
        }
        assert actors().destroy_actor(actor), record["label"]
        affected.append(record)
    return sorted(affected, key=lambda row: row["canonicalId"])


def remove_all_closed_instance_boxes():
    affected = []
    for actor in actors().get_all_level_actors():
        if actor.get_outer().get_path_name().split(".")[0] != CONNECTED_DISTRICT_LEVEL:
            continue
        canonical_id = placeholder_canonical_id(actor)
        if not canonical_id or not canonical_id.startswith("instances:box:"):
            continue
        tags = {str(tag) for tag in actor.tags}
        if "Baddie.Readiness=Closed" not in tags:
            continue
        origin, extent = actor.get_actor_bounds(False)
        record = {
            "label": actor.get_actor_label(),
            "actor": actor.get_path_name(),
            "canonicalId": canonical_id,
            "levelPackage": actor.get_outer().get_path_name().split(".")[0],
            "origin": [origin.x, origin.y, origin.z],
            "extent": [extent.x, extent.y, extent.z],
        }
        assert actors().destroy_actor(actor), record["label"]
        affected.append(record)
    return sorted(affected, key=lambda row: row["canonicalId"])


def remove_closed_adjoining_mesh_cells():
    affected = []
    for actor in actors().get_all_level_actors():
        level_package = actor.get_outer().get_path_name().split(".")[0]
        allowed_ids = ADJOINING_CLOSED_MESH_LEVELS.get(level_package)
        if not allowed_ids:
            continue
        canonical_id = placeholder_canonical_id(actor)
        if canonical_id not in allowed_ids:
            continue
        if not canonical_id.startswith("mesh-cell:"):
            continue
        tags = {str(tag) for tag in actor.tags}
        if "Baddie.Readiness=Closed" not in tags:
            continue
        origin, extent = actor.get_actor_bounds(False)
        record = {
            "label": actor.get_actor_label(),
            "actor": actor.get_path_name(),
            "canonicalId": canonical_id,
            "levelPackage": level_package,
            "origin": [origin.x, origin.y, origin.z],
            "extent": [extent.x, extent.y, extent.z],
        }
        assert actors().destroy_actor(actor), record["label"]
        affected.append(record)
    return sorted(affected, key=lambda row: (row["levelPackage"], row["canonicalId"]))


def save_and_receipt(frontage_rows_created, road_rows_created, packs, dressing, child_levels, detail_streaming, removed_placeholders):
    detail_world = detail_streaming.get_loaded_level().get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail_world, DETAIL_MAP), "Failed to save additive street detail level"
    child_world = loaded_level_by_package(CONNECTED_DISTRICT_LEVEL).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(child_world, CONNECTED_DISTRICT_LEVEL), "Failed to save connected district child level"
    world_asset = world()
    assets().set_metadata_tag(world_asset, "Baddie.TownCentreStreetPass.Owner", OWNER)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world_asset, TARGET_MAP), "Failed to save duplicated town-centre pass"
    files = package_files(TARGET_MAP, True)
    files.update(package_files(DETAIL_MAP, True))
    files.update(package_files(CONNECTED_DISTRICT_LEVEL, True))
    OUT.mkdir(parents=True, exist_ok=True)
    selection = district_selection()
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "sourceMapPackage": SOURCE_MAP,
        "mapPackage": TARGET_MAP,
        "detailLevelPackage": DETAIL_MAP,
        "files": files,
        "childLevels": child_levels,
        "districtId": "town-centre",
        "districtSelectionPath": str(DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(DISTRICT_SELECTION),
        "districtBounds": selection["bounds"],
        "districtParcelCount": selection["parcelCount"],
        "districtParcelIds": selection["parcelIds"],
        "districtRoadIdsIntersectingBounds": selection["roadIdsIntersectingBounds"],
        "districtDestinations": selection["destinations"],
        "streetPassTag": TAG,
        "selectedRoadWindows": [
            {**window, "roads": sorted(window["roads"])}
            for window in ROAD_WINDOWS
        ],
        "frontageReceipt": str(FRONTAGE_RECEIPT.relative_to(ROOT)).replace("\\", "/"),
        "frontageReceiptSha256": sha(FRONTAGE_RECEIPT),
        "frontagePackCount": len(packs),
        "frontagePacks": packs,
        "frontageActorCount": len(frontage_rows_created),
        "frontageActors": frontage_rows_created[:80],
        "roadActorCount": len(road_rows_created),
        "roadActors": road_rows_created[:80],
        "staticDressing": dressing,
        "removedClosedPlaceholderCount": len(removed_placeholders),
        "removedClosedPlaceholders": removed_placeholders,
        "cameras": CAMERAS,
        "visualIntent": "Street-first Lagos detailing over the connected town-centre parent using accepted Yaba frontage, pavement, drain, and street-prop assets.",
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / RECEIPT_NAME
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def save_refine_receipt(frontage_rows_created, road_rows_created, packs, dressing, removed_actor_count, removed_placeholders):
    detail_world = loaded_level_by_package(DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail_world, DETAIL_MAP), "Failed to save refined detail level"
    child_world = loaded_level_by_package(CONNECTED_DISTRICT_LEVEL).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(child_world, CONNECTED_DISTRICT_LEVEL), "Failed to save refined connected district child level"
    world_asset = world()
    assets().set_metadata_tag(world_asset, "Baddie.TownCentreStreetPass.Owner", OWNER)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world_asset, TARGET_MAP), "Failed to save refined town-centre parent"
    files = package_files(TARGET_MAP, True)
    files.update(package_files(DETAIL_MAP, True))
    files.update(package_files(CONNECTED_DISTRICT_LEVEL, True))
    selection = district_selection()
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "mode": "refine-existing-loaded-streetpass01",
        "sourceMapPackage": SOURCE_MAP,
        "mapPackage": TARGET_MAP,
        "detailLevelPackage": DETAIL_MAP,
        "files": files,
        "districtId": "town-centre",
        "districtSelectionPath": str(DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(DISTRICT_SELECTION),
        "districtBounds": selection["bounds"],
        "districtParcelCount": selection["parcelCount"],
        "districtParcelIds": selection["parcelIds"],
        "districtRoadIdsIntersectingBounds": selection["roadIdsIntersectingBounds"],
        "districtDestinations": selection["destinations"],
        "streetPassTag": TAG,
        "selectedRoadWindows": [{**window, "roads": sorted(window["roads"])} for window in ROAD_WINDOWS],
        "frontageReceipt": str(FRONTAGE_RECEIPT.relative_to(ROOT)).replace("\\", "/"),
        "frontageReceiptSha256": sha(FRONTAGE_RECEIPT),
        "frontageV3AssetReceipt": str(V3_ASSET_RECEIPT.relative_to(ROOT)).replace("\\", "/"),
        "frontageV3AssetReceiptSha256": sha(V3_ASSET_RECEIPT),
        "replacedTaggedDetailActors": removed_actor_count,
        "frontagePackCount": len(packs),
        "frontagePacks": packs,
        "frontageActorCount": len(frontage_rows_created),
        "frontageActors": frontage_rows_created[:120],
        "roadActorCount": len(road_rows_created),
        "roadActors": road_rows_created[:120],
        "staticDressing": dressing,
        "removedClosedPlaceholderCount": len(removed_placeholders),
        "removedClosedPlaceholders": removed_placeholders,
        "cameras": CAMERAS,
        "visualIntent": "Refined town-centre corridor pass using wider canonical parcel coverage, accepted frontage-v3 variants, and removed closed placeholder shells in the validated Market and Civic corridors.",
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / REFINE_RECEIPT_NAME
    assert not receipt_path.exists(), receipt_path
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def save_placeholder_cleanup_receipt(removed_placeholders):
    child_world = loaded_level_by_package(CONNECTED_DISTRICT_LEVEL).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(child_world, CONNECTED_DISTRICT_LEVEL), "Failed to save cleaned connected district child level"
    files = package_files(CONNECTED_DISTRICT_LEVEL, True)
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "mode": "cleanup-closed-placeholder-blockers-only",
        "mapPackage": active_world_package(),
        "connectedDistrictLevelPackage": CONNECTED_DISTRICT_LEVEL,
        "files": files,
        "districtId": "town-centre",
        "districtSelectionPath": str(DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(DISTRICT_SELECTION),
        "removedClosedPlaceholderCount": len(removed_placeholders),
        "removedClosedPlaceholders": removed_placeholders,
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / PLACEHOLDER_CLEANUP_RECEIPT_NAME
    assert not receipt_path.exists(), receipt_path
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def save_all_closed_box_cleanup_receipt(removed_placeholders):
    child_world = loaded_level_by_package(CONNECTED_DISTRICT_LEVEL).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(child_world, CONNECTED_DISTRICT_LEVEL), "Failed to save cleaned connected district child level"
    files = package_files(CONNECTED_DISTRICT_LEVEL, True)
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "mode": "cleanup-all-closed-instance-boxes",
        "mapPackage": active_world_package(),
        "connectedDistrictLevelPackage": CONNECTED_DISTRICT_LEVEL,
        "files": files,
        "districtId": "town-centre",
        "districtSelectionPath": str(DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(DISTRICT_SELECTION),
        "removedClosedInstanceBoxCount": len(removed_placeholders),
        "removedClosedInstanceBoxes": removed_placeholders,
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / ALL_CLOSED_BOX_CLEANUP_RECEIPT_NAME
    assert not receipt_path.exists(), receipt_path
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def save_adjoining_mesh_cleanup_receipt(removed_mesh_cells):
    files = {}
    for package in sorted(ADJOINING_CLOSED_MESH_LEVELS):
        level_world = loaded_level_by_package(package).get_outer()
        assert unreal.EditorLoadingAndSavingUtils.save_map(level_world, package), "Failed to save cleaned adjoining level"
        files.update(package_files(package, True))
    receipt = {
        "schemaVersion": 1,
        "recordedAt": datetime.now(timezone.utc).isoformat(),
        "pid": os.getpid(),
        "owner": OWNER,
        "mode": "cleanup-closed-adjoining-mesh-cells",
        "mapPackage": active_world_package(),
        "adjoiningLevelPackages": sorted(ADJOINING_CLOSED_MESH_LEVELS),
        "files": files,
        "districtId": "town-centre",
        "districtSelectionPath": str(DISTRICT_SELECTION.relative_to(ROOT)).replace("\\", "/"),
        "districtSelectionSha256": sha(DISTRICT_SELECTION),
        "removedClosedMeshCellCount": len(removed_mesh_cells),
        "removedClosedMeshCells": removed_mesh_cells,
        "playableAcceptance": "not-established",
        "runtimeAdmission": False,
    }
    receipt_path = OUT / ADJOINING_MESH_CLEANUP_RECEIPT_NAME
    assert not receipt_path.exists(), receipt_path
    with receipt_path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    return receipt_path


def prepare_camera(camera_id):
    spec = next(c for c in CAMERAS if c["id"] == camera_id)
    key = levels().get_active_viewport_config_key()
    levels().set_level_viewport_camera_info(unreal.Vector(*spec["locationCm"]), rotator(spec["rotation"]), key)
    levels().set_level_viewport_fov(spec["fov"], key)
    levels().editor_set_game_view(True, key)
    levels().editor_set_viewport_realtime(True)
    levels().editor_invalidate_viewports()


def execute(expected_pid):
    ensure_identity(expected_pid)
    child_levels, detail_streaming = prepare_new_parent(expected_pid)
    set_manual_exposure()
    frontage_data = load_frontage_data()
    v3_lookup = v3_modules()
    material_derivatives = frontage_data["materialUsageDerivatives"]
    frontage_data["placements"] = [p for p in frontage_data["placements"] if p["module"] in {
        "FrontageStructure", "ShopBay", "ResidentialBay", "Window1300", "Shutter2800", "OpenShopInterior",
        "Balcony3900", "ACServiceAssembly", "RoofSheetPanel", "Gutter3900", "RainwaterDownpipe",
        "PlasterRepair", "Drain1000", "DrainCover1000", "Kerb1000", "Pavement1000"
    }]
    global canonical_data
    canonical_data = canonical()
    parcels, roads = canonical_data
    frontage_rows_data, packs = frontage_rows(parcels, roads, frontage_data, v3_lookup)
    road_rows_data = road_rows(frontage_data, v3_lookup)
    frontage_created = spawn_frontage_rows(frontage_rows_data, material_derivatives)
    roads_created = spawn_road_rows(road_rows_data)
    dressing = spawn_static_dressing()
    removed_placeholders = remove_closed_placeholder_blockers()
    receipt_path = save_and_receipt(frontage_created, roads_created, packs, dressing, child_levels, detail_streaming, removed_placeholders)
    prepare_camera("market-road-junction")
    print(json.dumps({
        "mapPackage": TARGET_MAP,
        "detailLevelPackage": DETAIL_MAP,
        "frontageActors": len(frontage_created),
        "roadActors": len(roads_created),
        "dressingActors": len(dressing),
        "removedClosedPlaceholders": len(removed_placeholders),
        "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
        "receiptSha256": sha(receipt_path),
    }))


def refine_existing(expected_pid):
    ensure_identity(expected_pid)
    assert world().get_path_name().split(".")[0] == TARGET_MAP, "Expected the existing streetpass01 map to be loaded"
    assert not dirty_names(), "Refine pass expects a clean loaded map"
    set_manual_exposure()
    frontage_data = load_frontage_data()
    v3_lookup = v3_modules()
    material_derivatives = frontage_data["materialUsageDerivatives"]
    frontage_data["placements"] = [p for p in frontage_data["placements"] if p["module"] in {
        "FrontageStructure", "ShopBay", "ResidentialBay", "Window1300", "Shutter2800", "OpenShopInterior",
        "Balcony3900", "ACServiceAssembly", "RoofSheetPanel", "Gutter3900", "RainwaterDownpipe",
        "PlasterRepair", "Drain1000", "DrainCover1000", "Kerb1000", "Pavement1000"
    }]
    global canonical_data
    canonical_data = canonical()
    parcels, roads = canonical_data
    make_level_current(DETAIL_MAP)
    removed_actor_count = clear_detail_actors()
    frontage_rows_data, packs = frontage_rows(parcels, roads, frontage_data, v3_lookup)
    road_rows_data = road_rows(frontage_data, v3_lookup)
    frontage_created = spawn_frontage_rows(frontage_rows_data, material_derivatives)
    roads_created = spawn_road_rows(road_rows_data)
    dressing = spawn_static_dressing()
    removed_placeholders = remove_closed_placeholder_blockers()
    receipt_path = save_refine_receipt(frontage_created, roads_created, packs, dressing, removed_actor_count, removed_placeholders)
    prepare_camera("market-road-junction")
    print(json.dumps({
        "mapPackage": TARGET_MAP,
        "detailLevelPackage": DETAIL_MAP,
        "removedTaggedDetailActors": removed_actor_count,
        "frontageActors": len(frontage_created),
        "roadActors": len(roads_created),
        "dressingActors": len(dressing),
        "removedClosedPlaceholders": len(removed_placeholders),
        "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
        "receiptSha256": sha(receipt_path),
    }))


def cleanup_closed_placeholders_only(expected_pid):
    ensure_identity(expected_pid)
    assert active_world_package() == TARGET_MAP, "Expected the existing streetpass01 map to be loaded"
    assert not dirty_names(), "Cleanup pass expects a clean loaded map"
    removed_placeholders = remove_closed_placeholder_blockers()
    receipt_path = save_placeholder_cleanup_receipt(removed_placeholders)
    prepare_camera("market-road-junction")
    print(json.dumps({
        "mapPackage": TARGET_MAP,
        "connectedDistrictLevelPackage": CONNECTED_DISTRICT_LEVEL,
        "removedClosedPlaceholders": len(removed_placeholders),
        "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
        "receiptSha256": sha(receipt_path),
    }))


def cleanup_all_closed_instance_boxes(expected_pid):
    ensure_identity(expected_pid)
    assert active_world_package() == TARGET_MAP, "Expected the existing streetpass01 map to be loaded"
    assert not dirty_names(), "Cleanup pass expects a clean loaded map"
    removed_placeholders = remove_all_closed_instance_boxes()
    receipt_path = save_all_closed_box_cleanup_receipt(removed_placeholders)
    prepare_camera("market-road-junction")
    print(json.dumps({
        "mapPackage": TARGET_MAP,
        "connectedDistrictLevelPackage": CONNECTED_DISTRICT_LEVEL,
        "removedClosedInstanceBoxes": len(removed_placeholders),
        "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
        "receiptSha256": sha(receipt_path),
    }))


def cleanup_closed_adjoining_mesh_cells(expected_pid):
    ensure_identity(expected_pid)
    assert active_world_package() == TARGET_MAP, "Expected the existing streetpass01 map to be loaded"
    assert not dirty_names(), "Cleanup pass expects a clean loaded map"
    removed_mesh_cells = remove_closed_adjoining_mesh_cells()
    receipt_path = save_adjoining_mesh_cleanup_receipt(removed_mesh_cells)
    prepare_camera("civic-street-approach")
    print(json.dumps({
        "mapPackage": TARGET_MAP,
        "adjoiningLevelPackages": sorted(ADJOINING_CLOSED_MESH_LEVELS),
        "removedClosedMeshCells": len(removed_mesh_cells),
        "receipt": str(receipt_path.relative_to(ROOT)).replace("\\", "/"),
        "receiptSha256": sha(receipt_path),
    }))


def dispatch(expected_pid):
    mode = os.environ.get("BADDIE_TOWN_CENTRE_PASS_MODE", "").strip().lower()
    if mode == "execute":
        execute(expected_pid)
        return
    if mode == "refine":
        refine_existing(expected_pid)
        return
    if mode == "cleanup":
        cleanup_closed_placeholders_only(expected_pid)
        return
    if mode == "cleanup-all":
        cleanup_all_closed_instance_boxes(expected_pid)
        return
    if mode == "cleanup-adjoining-mesh":
        cleanup_closed_adjoining_mesh_cells(expected_pid)
        return
    if active_world_package() == TARGET_MAP:
        refine_existing(expected_pid)
        return
    execute(expected_pid)


if __name__ == "__main__":
    dispatch(int(os.environ["BADDIE_EXPECTED_PID"]))
