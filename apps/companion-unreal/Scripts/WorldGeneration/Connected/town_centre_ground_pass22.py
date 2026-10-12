"""Ground canonical Town Centre corridors and parcels with pivot-correct meshes.

This is additive native exterior support, not a coastline or gameplay claim.
"""
import gc
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-22-canonical-ground.json"
SPEC = importlib.util.spec_from_file_location(
    "tc_ground_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py"
)
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
MESH = "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v3/Meshes/SM_Pavement1000"
PREFIX = "TC_Ground22_"


def clip_segment(a, b, bounds):
    low, high = 0.0, 1.0
    delta = (b[0] - a[0], b[1] - a[1])
    for axis, minimum, maximum in (
        (0, bounds["minX"], bounds["maxX"]),
        (1, bounds["minZ"], bounds["maxZ"]),
    ):
        if abs(delta[axis]) < 1e-8:
            if not minimum <= a[axis] <= maximum:
                return None
            continue
        t0, t1 = sorted(((minimum - a[axis]) / delta[axis], (maximum - a[axis]) / delta[axis]))
        low, high = max(low, t0), min(high, t1)
        if low >= high:
            return None
    return tuple((a[0] + t * delta[0], a[1] + t * delta[1]) for t in (low, high))


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP
    assert not BASE.dirty_names()
    assert not RECEIPT.exists(), "Preserve immutable pass receipt"
    assert not any(a.get_actor_label().startswith(PREFIX) for a in BASE.actors().get_all_level_actors())
    BASE.make_level_current(BASE.DETAIL_MAP)
    selection = BASE.district_selection()
    parcels, roads = BASE.canonical()
    mesh = BASE.assets().load_asset(MESH)
    asphalt = BASE.assets().load_asset(BASE.ROAD_MATERIAL)
    assert mesh and asphalt
    bounds = mesh.get_bounds()
    origin = (bounds.origin.x, bounds.origin.y, bounds.origin.z)
    dimensions = (bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2)
    assert all(value > 0 for value in dimensions)
    rows = []

    def slab(label, centre, length, width, yaw, top, source_ids, role, material=None):
        scale = (length / dimensions[0], width / dimensions[1], 1.0)
        angle = math.radians(yaw)
        ox, oy = origin[0] * scale[0], origin[1] * scale[1]
        # Position the mesh's geometric centre, not its off-centre import pivot.
        location = (
            centre[0] - ox * math.cos(angle) + oy * math.sin(angle),
            centre[1] - ox * math.sin(angle) - oy * math.cos(angle),
            top - (origin[2] + dimensions[2] / 2),
        )
        actor = BASE.actors().spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(pitch=0, yaw=yaw, roll=0)
        )
        actor.set_actor_label(PREFIX + label)
        actor.tags = ["Baddie.TownCentreGround22"] + ["Baddie.Source:" + sid for sid in source_ids]
        actor.set_actor_scale3d(unreal.Vector(*scale))
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(mesh)
        if material:
            component.set_material(0, material)
        component.set_collision_profile_name("BlockAll")
        rows.append({"label": actor.get_actor_label(), "sourceIds": source_ids, "role": role,
                     "centreCm": list(centre), "lengthCm": length, "widthCm": width,
                     "yaw": yaw, "topCm": top, "locationCm": list(location), "scale": list(scale),
                     "mesh": MESH, "material": material.get_path_name() if material else None})

    road_ids = []
    for road in roads.values():
        authored = False
        for index, (a, b) in enumerate(zip(road["points"], road["points"][1:])):
            clipped = clip_segment(a, b, selection["bounds"])
            if not clipped:
                continue
            ua, ub = [BASE.ue_xy_from_source(*point) for point in clipped]
            dx, dy = ub[0] - ua[0], ub[1] - ua[1]
            length = math.hypot(dx, dy)
            if length < 1:
                continue
            centre = ((ua[0] + ub[0]) / 2, (ua[1] + ub[1]) / 2)
            yaw = math.degrees(math.atan2(dy, dx))
            source_ids = [road["id"], f"{road['id']}:segment:{index}"]
            slab(f"{road['id']}_{index}_support", centre, length + 30,
                 (road["width"] + 2 * road.get("pavement", 3)) * 100, yaw, -2, source_ids, "road-corridor-support")
            slab(f"{road['id']}_{index}_asphalt", centre, length + 20,
                 road["width"] * 100, yaw, 0, source_ids, "carriageway", asphalt)
            authored = True
        if authored:
            road_ids.append(road["id"])

    for parcel in parcels.values():
        centre = BASE.ue_xy_from_source(parcel["x"], parcel["z"])
        yaw = 90 - math.degrees(parcel["yaw"])
        slab(parcel["id"] + "_footprint", centre, parcel["width"] * 100,
             parcel["depth"] * 100, yaw, -3, [parcel["id"]], "canonical-parcel-support")
        entrance = parcel.get("entrance")
        if not entrance:
            continue
        ue_entrance = BASE.ue_xy_from_source(entrance[0], entrance[2])
        road = BASE.parcel_road(parcel, roads)
        pose = BASE.nearest_road_pose({"x": entrance[0], "z": entrance[2]}, road)
        dx, dy = ue_entrance[0] - pose["point"][0], ue_entrance[1] - pose["point"][1]
        length = math.hypot(dx, dy)
        # Only the saved entrance-to-road relationship establishes this threshold.
        if length > 1:
            slab(parcel["id"] + "_entrance", ((ue_entrance[0] + pose["point"][0]) / 2,
                 (ue_entrance[1] + pose["point"][1]) / 2), length, 180,
                 math.degrees(math.atan2(dy, dx)), -1, [parcel["id"], road["id"]], "entrance-support")

    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(),
               "pid": expected_pid, "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP,
               "detailLevelPackage": BASE.DETAIL_MAP, "districtBounds": selection["bounds"],
               "source": selection["source"], "roadIds": road_ids, "parcelIds": sorted(parcels),
               "meshBounds": {"origin": origin, "dimensions": dimensions}, "createdActors": rows,
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "limitations": ["Support only; no navigation build or runtime traversal inspection",
                                "Road joins, threshold intersections and facade placements require rendered review",
                                "No continuous terrain or approved coastline inferred from district bounds"],
               "playableAcceptance": "not-established"}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "roads": len(road_ids), "parcels": len(parcels), "actors": len(rows)}))
    gc.collect()
