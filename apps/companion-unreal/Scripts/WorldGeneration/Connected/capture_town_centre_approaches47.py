"""Serial matched road approach captures, using the existing authenticated editor."""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_approach_capture',SCRIPTS/'capture_town_centre_views.py')
CAPTURE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(CAPTURE)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--expected-pid',type=int,required=True)
    parser.add_argument('--phase',choices=('before','after'),required=True);args=parser.parse_args()
    code=f"""
import importlib.util,json
s=importlib.util.spec_from_file_location('tc_approach_cameras',{str(SCRIPTS/'town_centre_approaches_pass47.py')!r})
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
m.BASE.ensure_identity({args.expected_pid})
assert m.BASE.active_world_package()==m.BASE.TARGET_MAP and not m.BASE.dirty_names()
print(json.dumps({{'cameras':m.cameras()}}))
del m,s
"""
    cameras=CAPTURE.bridge(args.expected_pid,code)[-1]['cameras']
    CAPTURE.EXTRA_CAMERAS.update({c['id']:c for c in cameras})
    out=ROOT/'output/cribtest11-visible-world-2026-10-11'
    number=46 if args.phase=='before' else 47
    CAPTURE.RECEIPT=out/f'town-centre-street-pass-{number}-saved-readback.json'
    CAPTURE.OUT_DIR=out/f'previews/town-centre-approaches47-{args.phase}'
    for c in cameras:print(json.dumps(CAPTURE.capture_view(args.expected_pid,c['id'],c['id'])),flush=True)


if __name__=='__main__':main()
