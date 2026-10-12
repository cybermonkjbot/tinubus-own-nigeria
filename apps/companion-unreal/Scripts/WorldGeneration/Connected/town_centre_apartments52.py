"""Five canonical Arcade Crescent apartment exteriors, without runtime access."""
import importlib.util
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_apartments52_fabric',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fabric_pass49.py')
FABRIC=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
FABRIC.FOCUS=('arcade-crescent-2','arcade-crescent-3','arcade-crescent-4','arcade-crescent-6','arcade-crescent-8')
FABRIC.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Apartments52'
FABRIC.PREPARED=FABRIC.OUT/'town-centre-street-pass-52-preflight.json'
FABRIC.RECEIPT=FABRIC.OUT/'town-centre-street-pass-52-parcel-frontage.json'
FABRIC.READBACK=FABRIC.OUT/'town-centre-street-pass-52-saved-readback.json'
FABRIC.PREFIX='TC_Apartments52_'
FABRIC.TAG='Baddie.TownCentreApartments52'
FABRIC.SOURCE_DIRECTORY='apartments52-import-source'
prepare=FABRIC.prepare
execute=FABRIC.execute
inspect_saved=FABRIC.inspect_saved
