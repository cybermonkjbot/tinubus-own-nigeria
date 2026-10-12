"""Reconcile source pedestrian route kinds with native surface overlays."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-29-route-classification.json"
SPEC = importlib.util.spec_from_file_location("tc_path_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    prior = json.loads((OUT / "town-centre-street-pass-22-canonical-ground.json").read_text())
    _, roads = BASE.canonical()
    by_label = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    concrete_path = "/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v2/ISM_v1/Materials/M_Concrete"
    concrete = BASE.assets().load_asset(concrete_path)
    assert concrete
    decisions = []
    for row in prior["createdActors"]:
        if row["role"] != "carriageway":
            continue
        road = roads[row["sourceIds"][0]]
        if road["kind"] not in {"path", "sand-path", "boardwalk", "retained"}:
            continue
        actor = by_label[row["label"]]
        assert actor.get_outer().get_path_name().split(".")[0] == BASE.DETAIL_MAP
        location = actor.get_actor_location()
        assert max(abs(a - b) for a, b in zip((location.x, location.y, location.z), row["locationCm"])) < 0.1
        decisions.append({"label": row["label"], "sourceIds": row["sourceIds"], "kind": road["kind"],
                          "sourceWidthMetres": road["width"],
                          "action": "concrete-pedestrian-surface" if road["kind"] == "path" else "remove-invalid-asphalt-overlay",
                          "supportRetained": row["label"].removesuffix("_asphalt") + "_support"})
    assert decisions
    BASE.make_level_current(BASE.DETAIL_MAP)
    for decision in decisions:
        actor = by_label[decision["label"]]
        if decision["action"] == "concrete-pedestrian-surface":
            actor.get_component_by_class(unreal.StaticMeshComponent).set_material(0, concrete)
        else:
            assert BASE.actors().destroy_actor(actor)
    by_label.clear()
    del actor
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(), "pid": expected_pid,
               "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP, "detailLevelPackage": BASE.DETAIL_MAP,
               "source": BASE.district_selection()["source"], "decisions": decisions,
               "pedestrianMaterial": concrete_path,
               "classification": [{"roadId": rid, "kind": roads[rid]["kind"], "widthMetres": roads[rid]["width"],
                                   "state": "partial-native", "finish": "missing-approved-finish" if roads[rid]["kind"] in {"sand-path", "boardwalk"} else "native-surface-present"}
                                  for rid in prior["roadIds"]],
               "missingDependencies": ["Approved native boardwalk decking and edge modules at source route transforms",
                                       "Licensed native-ready sand path surface and source-defined beach terrain"],
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established",
               "limitations": ["Concrete path finish is partial; junctions, drainage and terrain not accepted",
                                "Bounds-clipped route coverage is not a complete district or full route"]}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "decisions": len(decisions)}))
