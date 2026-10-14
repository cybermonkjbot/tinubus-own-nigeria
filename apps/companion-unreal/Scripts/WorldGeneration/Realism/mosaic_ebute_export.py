"""Create an immutable Ebute Metta authoring packet; no Unreal imports or editor writes."""
import argparse
import hashlib
import json
from pathlib import Path

from WorldGeneration.Realism import mosaic_ebute_layout, mosaic_native


ROOT = Path(__file__).resolve().parents[5]
CONFIG_PATH = Path(__file__).with_name("mosaic-ebute-metta-v1.json")
OUTPUT_ROOT = ROOT / "output/lagos-mosaic-authoring"


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
    events = [{"id": "ebute:optional-vestibule-privacy-closure",
               "kind": "invitation-only-space-closes",
               "publicCirculationBlocked": False,
               "alternate": "street-facing collection point or continue on the public route",
               "clearingCondition": "Owner chooses to restore invitation access; otherwise remains optional",
               "interactionMayBeDeclined": True, "runtimeImplemented": False}]
    return {"schemaVersion": 1, "documentKind": "offline-neighborhood-authoring-packet",
            "runtimeRegistered": False, "engineMapSaved": False,
            "configSha256": mosaic_native.sha(CONFIG_PATH),
            "layoutSha256": mosaic_native.sha(mosaic_ebute_layout.__file__),
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
            "events": events,
            "audioIntent": {"playable": False,
                            "layers": ["street conversation", "small workroom tools", "food preparation"],
                            "recordingsPresent": False, "rightsCleared": False,
                            "householdAndWorkshopOcclusionAuthored": False},
            "assetChecklist": {
                "reusedMeshes": sorted({p["module"] for p in layout["placements"]}),
                "newMeshAssetsCreated": False,
                "inheritedYabaNamedMaterialSetNeedsLocalReview": True,
                "purposeSpecificFitoutsPending": ["cushion and furniture repair workroom", "clothing alterations room",
                                                  "food/provisions counter", "household doors and interiors"],
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
            "runtimeRegistered": False, "engineMapSaved": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    print(json.dumps(export()))
