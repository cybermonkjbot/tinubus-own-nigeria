"""Serial matched native viewport evidence, never PIE or an engine launch."""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/unreal-adjoining-continuation01-20261014'
PATH = ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/capture_town_centre_views.py'
SPEC = importlib.util.spec_from_file_location('continuation_capture', PATH)
CAP = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAP)


def protected_capture_state():
    records = CAP.bridge(10404, """
import json, runpy
namespace = runpy.run_path('C:/dev/cribtest-game/apps/companion-unreal/Scripts/StreetFinish/continue_adjoining01.py')
namespace['guard']()
protection = namespace['check_protected'](readonly=True)
print(json.dumps({'protectedEvidence': protection}))
del namespace
""")
    matches = [r['protectedEvidence'] for r in records if 'protectedEvidence' in r]
    if len(matches) != 1:
        raise RuntimeError('Missing or ambiguous protected capture comparison')
    return matches[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--phase', required=True)
    parser.add_argument('--camera', action='append')
    args = parser.parse_args()
    cameras = json.loads((OUT / 'cameras.json').read_text())
    CAP.RECEIPT = OUT / 'survey.json'
    CAP.OUT_DIR = OUT / 'evidence' / args.phase
    CAP.load_receipt = lambda: {'mapPackage': '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane', 'cameras': []}
    CAP.camera_specs = lambda: {c['id']: c for c in cameras}
    for camera in cameras:
        if args.camera and camera['id'] not in args.camera:
            continue
        before = protected_capture_state()
        record = CAP.capture_view(10404, camera['id'], camera['id'])
        after = protected_capture_state()
        if before != after:
            raise RuntimeError('Protected capture coverage changed')
        print(json.dumps({'image': record['image'], 'camera': record['cameraConfig'],
                         'protectedEvidence': after}), flush=True)


if __name__ == '__main__':
    main()
