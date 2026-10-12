"""Matched boundary views via the existing authenticated capture helper."""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_boundary51_capture',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/capture_town_centre_views.py')
CAPTURE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(CAPTURE)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--expected-pid',type=int,required=True)
    parser.add_argument('--phase',choices=('before','after'),required=True)
    args=parser.parse_args()
    data=json.loads((OUT/'town-centre-street-pass-51-preflight.json').read_text())
    CAPTURE.EXTRA_CAMERAS.update({c['id']:c for c in data['previewPoses']})
    CAPTURE.RECEIPT=OUT/('town-centre-street-pass-51-preflight.json' if args.phase=='before' else 'town-centre-street-pass-51-saved-readback.json')
    CAPTURE.OUT_DIR=OUT/('previews/town-centre-compounds51-'+args.phase)
    for c in data['previewPoses']:
        print(json.dumps(CAPTURE.capture_view(args.expected_pid,c['id'],c['id'])),flush=True)


if __name__=='__main__':main()
