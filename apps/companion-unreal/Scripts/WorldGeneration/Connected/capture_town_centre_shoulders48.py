"""Capture receipt-defined poses without routing them to the legacy camera list."""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_shoulder_capture',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/capture_town_centre_views.py')
CAPTURE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(CAPTURE)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--expected-pid',type=int,required=True);args=parser.parse_args()
    data=json.loads((OUT/'town-centre-street-pass-48-refined-shoulders.json').read_text())
    cameras=data['cameras']
    CAPTURE.EXTRA_CAMERAS.update({c['id']:c for c in cameras})
    CAPTURE.RECEIPT=OUT/'town-centre-street-pass-48-saved-readback.json'
    CAPTURE.OUT_DIR=OUT/'previews/town-centre-shoulders48-after'
    for c in cameras:print(json.dumps(CAPTURE.capture_view(args.expected_pid,c['id'],c['id'])),flush=True)


if __name__=='__main__':main()
