"""Source-centred ordinary Old Town Lane apartment exteriors."""
import importlib.util
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_oldtown57_fabric',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_fabric_pass49.py')
FABRIC=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
FABRIC.FOCUS=('old-town-lane-3','old-town-lane-4','old-town-lane-6','old-town-lane-7','old-town-lane-10')
FABRIC.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/OldTown57'
FABRIC.PREPARED=FABRIC.OUT/'town-centre-street-pass-57-preflight.json'
FABRIC.RECEIPT=FABRIC.OUT/'town-centre-street-pass-57-parcel-frontage.json'
FABRIC.READBACK=FABRIC.OUT/'town-centre-street-pass-57-saved-readback.json'
FABRIC.PREFIX='TC_OldTown57_'
FABRIC.TAG='Baddie.TownCentreOldTown57'
FABRIC.SOURCE_DIRECTORY='oldtown57-import-source'
FABRIC.SEALED_FRAME=True
prepare=FABRIC.prepare
execute=FABRIC.execute
inspect_saved=FABRIC.inspect_saved
