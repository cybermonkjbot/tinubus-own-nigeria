"""Source-defined low-rise Arcade Crescent exteriors using the accepted kit."""
import importlib.util
from pathlib import Path

ROOT = Path('C:/dev/cribtest-game')
SPEC = importlib.util.spec_from_file_location('tc_arcade50_fabric',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fabric_pass49.py')
FABRIC = importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
FABRIC.FOCUS = ('arcade-crescent-1','arcade-crescent-7','arcade-crescent-9')
FABRIC.NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Arcade50'
FABRIC.PREPARED = FABRIC.OUT/'town-centre-street-pass-50-preflight.json'
FABRIC.RECEIPT = FABRIC.OUT/'town-centre-street-pass-50-parcel-frontage.json'
FABRIC.READBACK = FABRIC.OUT/'town-centre-street-pass-50-saved-readback.json'
FABRIC.PREFIX = 'TC_Arcade50_'
FABRIC.TAG = 'Baddie.TownCentreArcade50'
FABRIC.SOURCE_DIRECTORY = 'arcade50-import-source'
prepare = FABRIC.prepare
execute = FABRIC.execute
inspect_saved = FABRIC.inspect_saved
