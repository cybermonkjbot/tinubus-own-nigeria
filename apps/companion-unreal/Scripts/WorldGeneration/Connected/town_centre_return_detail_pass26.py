"""Correct copied return downpipes and furnish ground-floor window openings."""
import importlib.util
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path("C:/dev/cribtest-game")
OUT = ROOT / "output/cribtest11-visible-world-2026-10-11"
RECEIPT = OUT / "town-centre-street-pass-26-return-details.json"
SPEC = importlib.util.spec_from_file_location("tc_return_base", ROOT / "apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    previous = json.loads((OUT / "town-centre-street-pass-25-courtyard-returns.json").read_text())
    by_label = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    removals = defaultdict(list)
    additions = defaultdict(list)
    for row in previous["createdInstances"]:
        if row["module"] == "RainwaterDownpipe":
            removals[row["actorLabel"]].append(row["instanceIndex"])
        if row["module"].startswith("Window1300") and abs(row["transform"]["locationCm"][2] - 402.5) < 1:
            addition = json.loads(json.dumps(row))
            addition["sourceId"] += ":ground-opening"
            addition["transform"]["locationCm"][2] -= 300
            additions[row["actorLabel"]].append(addition)
    assert removals and additions
    for label in set(removals) | set(additions):
        component = by_label[label].get_component_by_class(unreal.InstancedStaticMeshComponent)
        batch = next(b for b in previous["batches"] if b["actorLabel"] == label)
        assert component.get_instance_count() == batch["instances"], "Unexpected intervening instance edits"
    BASE.make_level_current(BASE.DETAIL_MAP)
    created, removed = [], []
    for label, indices in removals.items():
        component = by_label[label].get_component_by_class(unreal.InstancedStaticMeshComponent)
        for index in sorted(indices, reverse=True):
            assert component.remove_instance(index)
        removed.append({"actorLabel": label, "priorInstanceIndices": sorted(indices), "reason": "Rendered pipes project horizontally across ground at return corners"})
    for label, rows in additions.items():
        component = by_label[label].get_component_by_class(unreal.InstancedStaticMeshComponent)
        for row in rows:
            t = row["transform"]
            row["instanceIndex"] = component.add_instance(unreal.Transform(location=unreal.Vector(*t["locationCm"]),
                rotation=BASE.rotator(t["rotation"]), scale=unreal.Vector(*t["scale"])), world_space=True)
            created.append(row)
    counts = {label: by_label[label].get_component_by_class(unreal.InstancedStaticMeshComponent).get_instance_count()
              for label in set(removals) | set(additions)}
    by_label.clear()
    del component
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {"schemaVersion": 1, "recordedAt": datetime.now(timezone.utc).isoformat(), "pid": expected_pid,
               "districtId": "town-centre", "mapPackage": BASE.TARGET_MAP, "detailLevelPackage": BASE.DETAIL_MAP,
               "source": BASE.district_selection()["source"], "removedInstances": removed,
               "createdInstances": created, "finalComponentInstanceCounts": counts,
               "files": {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               "playableAcceptance": "not-established", "missingReplacement": "Correct corner downpipe route for courtyard wings; original copied return pipes removed only"}
    with RECEIPT.open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"receipt": str(RECEIPT), "removedPipes": sum(len(v) for v in removals.values()), "groundWindows": len(created)}))
