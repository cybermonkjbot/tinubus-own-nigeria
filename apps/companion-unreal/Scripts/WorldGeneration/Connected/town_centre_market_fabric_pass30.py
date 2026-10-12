"""Expand parcel-bound ordinary Market Road exterior fabric, native ISM only."""
import importlib.util
import json
import math
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-30-market-adjoining-fabric.json"
SPEC = importlib.util.spec_from_file_location("tc_market_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
FOCUS = {"market-road-4", "market-road-10", "market-road-11", "market-road-12"}


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    assert not any(a.get_actor_label().startswith("TC_Market30_") for a in BASE.actors().get_all_level_actors())
    parcels, roads = BASE.canonical()
    data, lookup = BASE.load_frontage_data(), BASE.v3_modules()
    original, _ = BASE.frontage_rows(parcels, roads, data, lookup)
    actors = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    removed = []
    for index, row in enumerate(original):
        if row["sourceId"].split(":", 1)[0] not in FOCUS:
            continue
        label = "TC_" + row["module"] + "_" + str(index)
        assert label in actors, label
        position = actors[label].get_actor_location()
        assert max(abs(v - e) for v, e in zip((position.x, position.y, position.z), row["transform"]["locationCm"])) < 1
        removed.append({"label": label, "sourceId": row["sourceId"], "transform": row["transform"]})
    rows, blocks = [], []
    for pid in sorted(FOCUS):
        parcel = parcels[pid]
        entrance = [*BASE.ue_xy_from_source(parcel["entrance"][0], parcel["entrance"][2]), 0]
        centre = BASE.ue_xy_from_source(parcel["x"], parcel["z"])
        direction = BASE.normalize2(BASE.sub2(centre, entrance[:2]))
        yaw = math.degrees(math.atan2(direction[1], direction[0]))
        repeats = max(1, int(parcel["width"] // 11.7))
        width = repeats * 1170
        storeys = 2 if parcel["floors"] <= 2 else 3
        wings = [("front" + str(i), [0, width / 2 - i * 1170, 0], 0, False) for i in range(repeats)]
        wings += [("rear" + str(i), [1170, -width / 2 + i * 1170, 0], 180, True) for i in range(repeats)]
        wings += [("return", [1170, width / 2, 0], -90, True)]
        assert 1480 <= parcel["depth"] * 100
        for wing, local, rotation, service in wings:
            origin = BASE.transform_local(entrance, yaw, local)
            wing_rows = []
            BASE.place_frontage_pack(wing_rows, data["placements"], pid + ":" + wing, origin, yaw + rotation,
                                     storeys, {0, 1, 2} if service else {BASE.stable_hash_int(pid) % 3})
            for row in wing_rows:
                if row["module"] in {"RainwaterDownpipe", "PlasterRepair"}:
                    continue
                if service and row["module"] in {"OpenShopInterior", "Shutter2800"}:
                    continue
                if service and row["module"] == "ShopBay":
                    row["module"] = "ResidentialBay"
                row = BASE.upgrade_frontage_row(row, pid + wing, lookup)
                row["parcelId"], row["wing"] = pid, wing
                rows.append(row)
                if service and row["module"].startswith("Window1300") and abs(row["transform"]["locationCm"][2] - 402.5) < 1:
                    lower = json.loads(json.dumps(row))
                    lower["sourceId"] += ":ground-opening"
                    lower["transform"]["locationCm"][2] -= 300
                    rows.append(lower)
        blocks.append({"parcelId": pid, "roadId": BASE.parcel_road(parcel, roads)["id"], "entranceCm": entrance,
                       "yaw": yaw, "widthCm": width, "returnDepthCm": 1170,
                       "sourceFloors": parcel["floors"], "moduleStoreys": storeys,
                       "floorParity": "partial; merged accepted modules are not exact canonical floor shells",
                       "wings": [w[0] for w in wings], "servicePassage": "one side left open"})
    groups = defaultdict(list)
    for row in rows:
        assert BASE.assets().load_asset(row["mesh"])
        for material in row["materials"]:
            assert BASE.assets().load_asset(material)
        groups[(row["mesh"], tuple(row["materials"]))].append(row)
    material_map, material_files = BASE.batching.batch_materials(rows)
    BASE.make_level_current(BASE.DETAIL_MAP)
    for row in removed:
        assert BASE.actors().destroy_actor(actors[row["label"]])
    actors.clear()
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    batches = []
    for index, ((mesh_path, materials), group) in enumerate(sorted(groups.items())):
        actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
        actor.set_actor_label(f"TC_Market30_{index}")
        actor.tags = ["Baddie.TownCentreMarket30", "Baddie.NoRuntimeAuthority"]
        handles = subsystem.k2_gather_subobject_data_for_instance(actor)
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=handles[0], new_class=unreal.InstancedStaticMeshComponent,
            blueprint_context=None, conform_transform_to_parent=False))
        assert not str(reason), str(reason)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
        component.set_static_mesh(BASE.assets().load_asset(mesh_path))
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name("BlockAll")
        component.set_cull_distances(0, 0)
        for slot, material in enumerate(materials):
            component.set_material(slot, BASE.assets().load_asset(material_map[material]))
        for row in group:
            t = row["transform"]
            row["actorLabel"] = actor.get_actor_label()
            row["instanceIndex"] = component.add_instance(unreal.Transform(location=unreal.Vector(*t["locationCm"]),
                rotation=BASE.rotator(t["rotation"]), scale=unreal.Vector(*t["scale"])), world_space=True)
        assert component.get_instance_count() == len(group)
        batches.append({"actorLabel": actor.get_actor_label(), "mesh": mesh_path,
                        "materials": [material_map[p] for p in materials], "instances": len(group)})
    del actor, component
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(), "pid": expected_pid,
               "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP, "detailLevelPackage": BASE.DETAIL_MAP,
               "source": BASE.district_selection()["source"], "blocks": blocks, "removedOriginalActors": removed,
               "createdInstances": rows, "batches": batches,
               "files": {**material_files, **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established", "visualAcceptance": "pending actual source-road and fixed previews",
               "missingDependencies": ["Exact canonical floor-count shells for taller apartments and single-storey shops",
                                       "Correct return downpipes and distinct approved building finishes"]}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "parcels": len(blocks), "instances": len(rows), "batches": len(batches)}))
