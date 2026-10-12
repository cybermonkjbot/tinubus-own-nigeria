"""Replace owned camera-aligned filler with entrance-aligned parcel frontage.

Exact original labels and transforms must match before any removal.
"""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-23-parcel-frontage.json"
SPEC = importlib.util.spec_from_file_location("tc_parcel_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
FOCUS = {"market-road-5", "market-road-6", "market-road-7", "market-road-8",
         "market-road-9", "civic-street-2", "civic-street-4", "civic-street-5", "civic-street-6"}


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    parcels, roads = BASE.canonical()
    data, lookup = BASE.load_frontage_data(), BASE.v3_modules()
    original_rows, _ = BASE.frontage_rows(parcels, roads, data, lookup)
    by_label = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    removals, mismatches = [], []
    for index, row in enumerate(original_rows):
        if row["sourceId"].split(":", 1)[0] not in FOCUS:
            continue
        label = "TC_" + row["module"] + "_" + str(index)
        actor = by_label.get(label)
        if actor is None:
            mismatches.append(label)
            continue
        location = actor.get_actor_location()
        expected = row["transform"]["locationCm"]
        if max(abs(v - e) for v, e in zip((location.x, location.y, location.z), expected)) > 1:
            mismatches.append(label)
            continue
        removals.append({"label": label, "sourceId": row["sourceId"], "locationCm": expected})
    assert not mismatches, {"originalIdentityMismatches": mismatches[:20]}
    clones = [label for label in by_label if label.startswith("TC_Fill16_")]
    assert clones and removals
    new_rows, packs = [], []
    for parcel_id in sorted(FOCUS):
        parcel = parcels[parcel_id]
        entrance = BASE.ue_xy_from_source(parcel["entrance"][0], parcel["entrance"][2])
        centre = BASE.ue_xy_from_source(parcel["x"], parcel["z"])
        inward = BASE.normalize2(BASE.sub2(centre, entrance))
        yaw = math.degrees(math.atan2(inward[1], inward[0]))
        count = max(1, int(parcel["width"] // 11.7))
        assert count * 11.7 <= parcel["width"]
        storeys = 2 + BASE.stable_hash_int(parcel_id) % 2
        pack_origins = []
        for repeat in range(count):
            lateral = count * 1170 / 2 - repeat * 1170
            origin = BASE.transform_local([*entrance, 0], yaw, [0, lateral, 0])
            start = len(new_rows)
            BASE.place_frontage_pack(new_rows, data["placements"], parcel_id + f":r{repeat}", origin,
                                     yaw, storeys, {BASE.stable_hash_int(parcel_id + str(repeat)) % 3})
            for index in range(start, len(new_rows)):
                new_rows[index] = BASE.upgrade_frontage_row(new_rows[index], parcel_id, lookup)
            pack_origins.append(origin)
        packs.append({"parcelId": parcel_id, "entranceCm": list(entrance), "parcelCentreCm": list(centre),
                      "inward": list(inward), "yaw": yaw, "repeatCount": count, "storeys": storeys,
                      "frontageWidthCm": count * 1170, "parcelWidthCm": parcel["width"] * 100,
                      "packOriginsCm": pack_origins, "roadId": BASE.parcel_road(parcel, roads)["id"]})
    # Resolve every asset before changing the level.
    for row in new_rows:
        assert BASE.assets().load_asset(row["mesh"])
        for path in row["materials"]:
            assert BASE.assets().load_asset(path)
    BASE.make_level_current(BASE.DETAIL_MAP)
    for label in clones + [r["label"] for r in removals]:
        assert BASE.actors().destroy_actor(by_label[label])
    by_label.clear()
    created = []
    for index, row in enumerate(new_rows):
        t = row["transform"]
        actor = BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*t["locationCm"]), BASE.rotator(t["rotation"]))
        actor.set_actor_label(f"TC_Parcel23_{row['module']}_{index}")
        actor.tags = ["Baddie.TownCentreParcel23", "Baddie.Source:" + row["sourceId"], "Baddie.NoRuntimeAuthority"]
        actor.set_actor_scale3d(unreal.Vector(*t["scale"]))
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(BASE.assets().load_asset(row["mesh"]))
        for slot, path in enumerate(row["materials"]):
            component.set_material(slot, BASE.assets().load_asset(path))
        created.append({"label": actor.get_actor_label(), **row})
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail, actor, component
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(),
               "pid": expected_pid, "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP,
               "detailLevelPackage": BASE.DETAIL_MAP, "source": BASE.district_selection()["source"],
               "frontageSourceReceipt": str(BASE.FRONTAGE_RECEIPT.relative_to(ROOT)),
               "frontageSourceSha256": BASE.sha(BASE.FRONTAGE_RECEIPT),
               "removedOriginalActors": removals, "removedCameraAlignedClones": clones,
               "packs": packs, "createdActors": created,
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established",
               "limitations": ["Parcel-depth buildings and upper floor massing remain incomplete",
                                "Rendered continuity and frontage material appearance require review"]}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "parcels": len(packs), "removed": len(removals) + len(clones), "created": len(created)}))
