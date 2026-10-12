"""Add parcel-bound courtyard returns from accepted frontage modules.

No mesh deformation: all attachment transforms retain source scale and UVs.
"""
import importlib.util
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-25-courtyard-returns.json"
SPEC = importlib.util.spec_from_file_location("tc_courtyard_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    assert not any(a.get_actor_label().startswith("TC_Courtyard25_") for a in BASE.actors().get_all_level_actors())
    previous = json.loads((OUT / "town-centre-street-pass-23-parcel-frontage.json").read_text())
    data, lookup = BASE.load_frontage_data(), BASE.v3_modules()
    parcels, _ = BASE.canonical()
    rows, blocks = [], []
    for pack in previous["packs"]:
        pid = pack["parcelId"]
        yaw = pack["yaw"]
        entrance = [*pack["entranceCm"], 0]
        width = pack["frontageWidthCm"]
        depth = 1170.0
        assert depth + 310 <= parcels[pid]["depth"] * 100
        returns = []
        for repeat in range(pack["repeatCount"]):
            returns.append(("rear" + str(repeat), [depth, -width / 2 + repeat * 1170, 0], 180))
        # Leave one side open as a yard/service passage on selected parcels.
        returns.append(("left", [depth, width / 2, 0], -90))
        if BASE.stable_hash_int(pid) % 3:
            returns.append(("right", [0, -width / 2, 0], 90))
        for wing, local, delta_yaw in returns:
            origin = BASE.transform_local(entrance, yaw, local)
            wing_rows = []
            BASE.place_frontage_pack(wing_rows, data["placements"], pid + ":" + wing,
                                     origin, yaw + delta_yaw, pack["storeys"], {0, 1, 2})
            for row in wing_rows:
                # Returns are residential/service exteriors, not additional shops.
                if row["module"] in {"Shutter2800", "OpenShopInterior", "PlasterRepair"}:
                    continue
                if row["module"] == "ShopBay":
                    row["module"] = "ResidentialBay"
                row = BASE.upgrade_frontage_row(row, pid + wing, lookup)
                row["parcelId"] = pid
                row["wing"] = wing
                rows.append(row)
        blocks.append({"parcelId": pid, "entranceCm": entrance, "yaw": yaw,
                       "widthCm": width, "returnDepthCm": depth, "wings": [r[0] for r in returns],
                       "storeys": pack["storeys"], "sourceParcelDepthCm": parcels[pid]["depth"] * 100})
    groups = defaultdict(list)
    for row in rows:
        assert BASE.assets().load_asset(row["mesh"])
        for path in row["materials"]:
            assert BASE.assets().load_asset(path)
        groups[(row["mesh"], tuple(row["materials"]))].append(row)
    # Existing native material processing produces only needed ISM derivatives.
    material_map, material_files = BASE.batching.batch_materials(rows)
    BASE.make_level_current(BASE.DETAIL_MAP)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    batches = []
    for index, ((mesh_path, materials), group) in enumerate(sorted(groups.items())):
        actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
        actor.set_actor_label(f"TC_Courtyard25_{index}")
        actor.tags = ["Baddie.TownCentreCourtyard25", "Baddie.NoRuntimeAuthority"]
        handles = subsystem.k2_gather_subobject_data_for_instance(actor)
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=handles[0], new_class=unreal.InstancedStaticMeshComponent,
            blueprint_context=None, conform_transform_to_parent=False))
        assert not str(reason), str(reason)
        component = library.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
        component.set_static_mesh(BASE.assets().load_asset(mesh_path))
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name("BlockAll")
        component.set_cull_distances(0, 0)
        for slot, path in enumerate(materials):
            component.set_material(slot, BASE.assets().load_asset(material_map[path]))
        for row in group:
            t = row["transform"]
            instance = component.add_instance(unreal.Transform(location=unreal.Vector(*t["locationCm"]),
                rotation=BASE.rotator(t["rotation"]), scale=unreal.Vector(*t["scale"])), world_space=True)
            row["actorLabel"] = actor.get_actor_label()
            row["instanceIndex"] = instance
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
               "source": BASE.district_selection()["source"], "blocks": blocks, "createdInstances": rows,
               "batches": batches, "materialMap": material_map,
               "files": {**material_files, **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established",
               "limitations": ["Courtyard exterior treatment does not complete canonical upper floors or private interiors",
                                "Corner joins and window alignment require rendered review"]}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "blocks": len(blocks), "instances": len(rows), "batches": len(batches)}))
