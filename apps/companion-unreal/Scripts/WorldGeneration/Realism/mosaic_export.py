"""Export a content-addressed authoring packet without importing Unreal."""
import argparse
import hashlib
import json
from pathlib import Path

from WorldGeneration.Realism import mosaic_audio, mosaic_layout, mosaic_navigation, mosaic_native, mosaic_routines


ROOT = Path(__file__).resolve().parents[5]
OUTPUT_ROOT = ROOT / "output/lagos-mosaic-authoring"


def build_packet():
    config, base = mosaic_native.source_inputs()
    placements, buildings = mosaic_layout.plan(base, config)
    navigation = mosaic_navigation.build_navigation(config)
    routines = mosaic_routines.build_routines(config, navigation["destinations"])
    emitters = mosaic_audio.build_emitters(config, navigation["destinations"])
    return {"schemaVersion": 1, "documentKind": "offline-mosaic-authoring-packet",
            "runtimeRegistered": False, "engineMapSaved": False,
            "configSha256": mosaic_native.sha(mosaic_native.CONFIG_PATH),
            "layoutSha256": mosaic_native.sha(mosaic_layout.__file__),
            "navigationSha256": mosaic_native.sha(mosaic_navigation.__file__),
            "routinesSha256": mosaic_native.sha(mosaic_routines.__file__),
            "audioSha256": mosaic_native.sha(mosaic_audio.__file__),
            "sourceReceiptSha256": config["baseReceiptSha256"],
            "geography": config["geography"], "placements": placements, "buildings": buildings,
            "materialUsageDerivatives": base["materialUsageDerivatives"],
            "navigation": navigation, "events": mosaic_navigation.build_events(navigation["destinations"]),
            "routines": routines, "audioEmitters": emitters,
            "pendingAssets": config["pendingAssets"], "gates": config["gates"]}


def export():
    packet = build_packet()
    data = (json.dumps(packet, sort_keys=True, indent=2, allow_nan=False) + "\n").encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT_ROOT / ("mosaic-yaba-" + digest + ".json")
    if path.exists():
        if path.read_bytes() != data:
            raise ValueError("Content-addressed packet differs from its name")
    else:
        with path.open("xb") as stream:
            stream.write(data)
    return {"path": str(path), "sha256": digest, "placementCount": len(packet["placements"]),
            "destinationCount": len(packet["navigation"]["destinations"]), "runtimeRegistered": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    print(json.dumps(export()))
