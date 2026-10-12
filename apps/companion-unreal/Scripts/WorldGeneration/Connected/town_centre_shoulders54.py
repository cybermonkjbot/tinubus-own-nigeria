"""Refine the five Arcade Crescent apartment shoulders without changing sources."""
import importlib.util
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_shoulders54_base',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_shoulders_pass48.py')
REFINE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(REFINE)
REFINE.PRIOR.TARGET=REFINE.OUT/'town-centre-street-pass-53-connections.json'
REFINE.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Shoulders54'
REFINE.TARGET=REFINE.OUT/'town-centre-street-pass-54-refined-shoulders.json'
REFINE.READBACK=REFINE.OUT/'town-centre-street-pass-54-saved-readback.json'
REFINE.PREFIX='TC_Shoulder54_'
REFINE.TAG='Baddie.TownCentreShoulders54'
REFINE.OLD_TAG='Baddie.TownCentreConnections53'
REFINE.SOURCE_DIRECTORY='shoulders54-import-source'
execute=REFINE.execute
inspect_saved=REFINE.inspect_saved
