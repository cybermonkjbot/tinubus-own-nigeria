"""Create an immutable Ebute Metta authoring packet; no Unreal imports or editor writes."""
import argparse
import hashlib
import json
from pathlib import Path

from WorldGeneration.Realism import mosaic_ebute_detail, mosaic_ebute_layout, mosaic_native


ROOT = Path(__file__).resolve().parents[5]
CONFIG_PATH = Path(__file__).with_name("mosaic-ebute-metta-v1.json")
OUTPUT_ROOT = ROOT / "output/lagos-mosaic-authoring"
DETAIL_DOCUMENT_PATH = ROOT / "docs/world-building/lagos-mosaic-2026-10-11/ebute-metta-detail-v1.md"


def build_packet():
    config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    base_config, base = mosaic_native.source_inputs()
    receipt = ROOT / "output/cribtest11-visible-world-2026-10-11/realism"
    for relative, expected in ((config["baseReceipt"], config["baseReceiptSha256"]),
                               (config["assetsReceipt"], config["assetsReceiptSha256"])):
        if mosaic_native.sha(receipt / relative) != expected:
            raise ValueError("Pinned Ebute source asset changed: " + relative)
    if mosaic_native.sha(ROOT / config["researchDocument"]) != config["researchDocumentSha256"]:
        raise ValueError("Pinned Ebute research document changed")
    layout = mosaic_ebute_layout.plan(base, config)
    detail = mosaic_ebute_detail.plan(layout)
    return {"schemaVersion": 2, "documentKind": "offline-neighborhood-authoring-packet",
            "runtimeRegistered": False, "engineMapSaved": False,
            "configSha256": mosaic_native.sha(CONFIG_PATH),
            "layoutSha256": mosaic_native.sha(mosaic_ebute_layout.__file__),
            "detailPlannerSha256": mosaic_native.sha(mosaic_ebute_detail.__file__),
            "detailDocumentPath": str(DETAIL_DOCUMENT_PATH.relative_to(ROOT)),
            "detailDocumentSha256": mosaic_native.sha(DETAIL_DOCUMENT_PATH),
            "sourceAccessDate": config["sourceAccessDate"],
            "sourceReceiptSha256": config["baseReceiptSha256"],
            "sourceAssetsReceiptSha256": config["assetsReceiptSha256"],
            "researchDocumentSha256": config["researchDocumentSha256"],
            "baseYabaConfigSha256": mosaic_native.sha(mosaic_native.CONFIG_PATH),
            "sourceAssetFiles": base["files"],
            "inheritedYabaSourceWasRescaled": False,
            "geography": config["coordinateContract"],
            "osmAttribution": config["sourceAttribution"],
            "osmLicense": config["sourceLicense"],
            "layout": layout,
            "streetLevelDetail": detail,
            "events": detail["events"],
            "audioIntent": {"playable": False,
                            "layers": ["conversation", "small workroom tools", "stock and fabric handling", "domestic room tone"],
                            "recordingsPresent": False, "rightsCleared": False,
                            "householdAndWorkshopOcclusionAuthored": True,
                            "occlusionGeometryCreated": False,
                            "emitters": detail["audioEmitters"]},
            "assetChecklist": {
                "reusedMeshes": sorted({p["module"] for p in layout["placements"]}),
                "newMeshAssetsCreated": False,
                "inheritedYabaNamedMaterialSetNeedsLocalReview": True,
                "meshRecipes": detail["meshRecipes"],
                "materialRecipes": detail["materialRecipes"],
                "finishRecipes": detail["finishRecipes"],
                "fitoutBlueprintsAuthored": True,
                "fitoutGeometryPending": True,
                "surfaceAndWeatherMaterialsPending": True,
                "lightingAndSeasonalSimulationPending": True,
                "npcTrafficAudioAndUtilitySystemsPending": True},
            "unresolved": ["surveyed width, frontage and current building condition",
                           "public access and continuous accessible footways",
                           "crossings, local grades, drain outlets and vertical datum",
                           "fresh dated street photography and material survey",
                           "current occupancy, local dialogue and consented sound"],
            "baseConfigSchemaVersion": base_config["schemaVersion"]}


def export():
    packet = build_packet()
    data = (json.dumps(packet, sort_keys=True, indent=2, allow_nan=False) + "\n").encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT_ROOT / ("mosaic-ebute-metta-" + digest + ".json")
    if path.exists():
        if path.read_bytes() != data:
            raise ValueError("Content-addressed packet differs from its name")
    else:
        with path.open("xb") as stream:
            stream.write(data)
    return {"path": str(path), "sha256": digest,
            "streetCount": len(packet["layout"]["streets"]),
            "edgeCount": len(packet["layout"]["edges"]),
            "buildingCount": len(packet["layout"]["buildings"]),
            "placementCount": len(packet["layout"]["placements"]),
            "roomVolumeCount": sum(len(p["rooms"]) for p in packet["streetLevelDetail"]["premises"]),
            "fitoutPropCount": len(packet["streetLevelDetail"]["propPlacements"]),
            "meshRecipeCount": len(packet["streetLevelDetail"]["meshRecipes"]),
            "audioEmitterCount": len(packet["streetLevelDetail"]["audioEmitters"]),
            "runtimeRegistered": False, "engineMapSaved": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    print(json.dumps(export()))
