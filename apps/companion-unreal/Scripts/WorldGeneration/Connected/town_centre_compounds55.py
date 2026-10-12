"""Source-fitted boundaries for two dependency-ready Arcade apartments."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_compounds55_base',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_compounds_pass46.py')
COMPOUND=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(COMPOUND)
COMPOUND.FOCUS=('arcade-crescent-2','arcade-crescent-3')
COMPOUND.SHELL_RECEIPT=COMPOUND.OUT/'town-centre-street-pass-52-parcel-frontage.json'
COMPOUND.ENTRANCE_RECEIPT=None
COMPOUND.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds55'
COMPOUND.TARGET=COMPOUND.OUT/'town-centre-street-pass-55-compounds.json'
COMPOUND.READBACK=COMPOUND.OUT/'town-centre-street-pass-55-saved-readback.json'
COMPOUND.PREFIX='TC_Compound55_'
COMPOUND.TAG='Baddie.TownCentreCompounds55'
COMPOUND.SOURCE_DIRECTORY='compounds55-import-source'
COMPOUND.FRONT_CLEARANCE=130


def prepare(pid):
    COMPOUND.BASE.ensure_identity(pid)
    assert COMPOUND.BASE.active_world_package()==COMPOUND.BASE.TARGET_MAP and not COMPOUND.BASE.dirty_names()
    data={'mapPackage':COMPOUND.BASE.TARGET_MAP,'detailLevelPackage':COMPOUND.BASE.DETAIL_MAP,
          'plans':COMPOUND.plans(),'previewPoses':COMPOUND.cameras()}
    with (COMPOUND.OUT/'town-centre-street-pass-55-preflight.json').open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')
    print('Two compound plans prepared')


execute=COMPOUND.execute
inspect_saved=COMPOUND.inspect_saved
