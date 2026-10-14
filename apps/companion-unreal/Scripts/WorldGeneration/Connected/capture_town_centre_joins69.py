"""Matched view of the source-preserving Library Lane bend surface repair."""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_joins69_capture',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/capture_town_centre_views.py')
CAPTURE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(CAPTURE)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--expected-pid',type=int,required=True)
    parser.add_argument('--phase',choices=('before','after','junctions'),required=True);args=parser.parse_args()
    data=json.loads((OUT/'town-centre-street-pass-69-preflight.json').read_text())
    if args.phase=='junctions':
        import math
        roads=json.loads((ROOT/'.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-city-v3.json').read_text())['roads']
        road=next(r for r in roads if r['id']=='library-lane')
        cameras=[]
        for p in data['plans']:
            index=p['pointIndex'];a,b=road['points'][index-1:index+1]
            dx,dz=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dz)
            direction=[dz/length,dx/length];centre=p['centreCm']
            cameras.append({'id':'library-lane-join-'+str(index),'locationCm':[centre[i]-direction[i]*700 for i in (0,1)]+[165],
                'rotation':{'pitch':-5,'yaw':math.degrees(math.atan2(direction[1],direction[0])),'roll':0},'fov':78})
        data['previewPoses']=cameras
    CAPTURE.EXTRA_CAMERAS.update({c['id']:c for c in data['previewPoses']})
    CAPTURE.RECEIPT=OUT/('town-centre-street-pass-69-preflight.json' if args.phase=='before' else 'town-centre-street-pass-69-saved-readback.json')
    CAPTURE.OUT_DIR=OUT/('previews/town-centre-joins69-'+args.phase)
    for c in data['previewPoses']:print(json.dumps(CAPTURE.capture_view(args.expected_pid,c['id'],c['id'])),flush=True)


if __name__=='__main__':main()
