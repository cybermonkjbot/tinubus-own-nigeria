"""Canonical shop and apartment exteriors adjoining Old Town Lane."""
import importlib.util
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_library63_fabric',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fabric_pass49.py')
FABRIC=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
FABRIC.FOCUS=('library-lane-9','library-lane-10')
FABRIC.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Library63'
FABRIC.PREPARED=FABRIC.OUT/'town-centre-street-pass-63-preflight.json'
FABRIC.RECEIPT=FABRIC.OUT/'town-centre-street-pass-63-parcel-frontage.json'
FABRIC.READBACK=FABRIC.OUT/'town-centre-street-pass-63-saved-readback.json'
FABRIC.PREFIX='TC_Library63_'
FABRIC.TAG='Baddie.TownCentreLibrary63'
FABRIC.SOURCE_DIRECTORY='library63-import-source'
FABRIC.SEALED_FRAME=True
prepare=FABRIC.prepare
execute=FABRIC.execute
inspect_saved=FABRIC.inspect_saved
