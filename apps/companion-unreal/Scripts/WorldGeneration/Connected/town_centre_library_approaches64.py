"""Fit canonical Library Lane approaches using its saved roadside ledger."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_library64_base',SCRIPTS/'town_centre_approaches_pass47.py')
APPROACH=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(APPROACH)
SPEC=importlib.util.spec_from_file_location('tc_library64_refinement',SCRIPTS/'town_centre_shoulders_pass48.py')
REFINE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(REFINE)
APPROACH.FOCUS=('library-lane-9',)
APPROACH.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/LibraryApproaches64'
APPROACH.TARGET=APPROACH.OUT/'town-centre-street-pass-64-entrance-approaches.json'
APPROACH.READBACK=APPROACH.OUT/'town-centre-street-pass-64-saved-readback.json'
APPROACH.RETAINED_RECEIPT=APPROACH.OUT/'town-centre-street-pass-63-parcel-frontage.json'
APPROACH.ROADSIDE_RECEIPT=APPROACH.OUT/'town-centre-street-pass-35-roadside-corridors.json'
APPROACH.PREFIX='TC_LibraryApproach64_'
APPROACH.TAG='Baddie.TownCentreLibraryApproaches64'
APPROACH.SOURCE_DIRECTORY='library-approaches64-import-source'
APPROACH.SHOULDER_GEOMETRY=REFINE.geometry
PREPARED=APPROACH.OUT/'town-centre-street-pass-64-preflight.json'


def prepare(pid):
    APPROACH.BASE.ensure_identity(pid)
    assert APPROACH.BASE.active_world_package()==APPROACH.BASE.TARGET_MAP and not APPROACH.BASE.dirty_names()
    data={'mapPackage':APPROACH.BASE.TARGET_MAP,'detailLevelPackage':APPROACH.BASE.DETAIL_MAP,
          'roadsideReceipt':APPROACH.ROADSIDE_RECEIPT.relative_to(ROOT).as_posix(),
          'roadsideReceiptSha256':APPROACH.BASE.sha(APPROACH.ROADSIDE_RECEIPT),
          'plans':APPROACH.plans(),'previewPoses':APPROACH.cameras(),
          'deferredParcels':{'library-lane-10':'Measured road-edge-to-source length310.607380cm; requires separately fitted short transition'}}
    with PREPARED.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print(json.dumps({'plans':[{'parcelId':p['parcelId'],'roadId':p['roadId'],'lengthCm':p['lengthCm']} for p in data['plans']]}))


def execute(pid):
    data=json.loads(PREPARED.read_text())
    assert data['plans']==APPROACH.plans()
    assert data['roadsideReceiptSha256']==APPROACH.BASE.sha(APPROACH.ROADSIDE_RECEIPT)
    APPROACH.execute(pid)


inspect_saved=APPROACH.inspect_saved
