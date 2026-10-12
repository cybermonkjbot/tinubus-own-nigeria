"""Fit Old Town Lane3's measured short approach without relocating its entrance."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_short61_base',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_approaches59.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
APPROACH=KIT.APPROACH
APPROACH.FOCUS=('old-town-lane-3',)
APPROACH.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/ShortApproach61'
APPROACH.TARGET=APPROACH.OUT/'town-centre-street-pass-61-entrance-approaches.json'
APPROACH.READBACK=APPROACH.OUT/'town-centre-street-pass-61-saved-readback.json'
APPROACH.RETAINED_RECEIPT=APPROACH.OUT/'town-centre-street-pass-60-support-repair.json'
APPROACH.PREFIX='TC_ShortApproach61_'
APPROACH.TAG='Baddie.TownCentreShortApproach61'
APPROACH.SOURCE_DIRECTORY='short-approach61-import-source'
# The measured451.219cm route retains180cm rise and a90cm raised deck.
APPROACH.MIN_APPROACH_LENGTH=450
APPROACH.PROFILE_BREAKS=(180,270)
KIT.PREPARED=APPROACH.OUT/'town-centre-street-pass-61-preflight.json'


def prepare(pid):
    APPROACH.BASE.ensure_identity(pid)
    assert APPROACH.BASE.active_world_package()==APPROACH.BASE.TARGET_MAP and not APPROACH.BASE.dirty_names()
    plans=APPROACH.plans()
    assert abs(plans[0]['lengthCm']-451.21956934903665)<.01
    data={'mapPackage':APPROACH.BASE.TARGET_MAP,'detailLevelPackage':APPROACH.BASE.DETAIL_MAP,
          'plans':plans,'previewPoses':APPROACH.cameras(),'profileScope':'Measured parcel3 only; no accessibility or runtime claim'}
    with KIT.PREPARED.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print('Measured451.219cm short approach prepared')


execute=KIT.execute
inspect_saved=APPROACH.inspect_saved


def inspect_mesh_contract(pid):
    KIT.inspect_mesh_contract(pid,APPROACH.OUT/'town-centre-street-pass-61-mesh-readback.json')
