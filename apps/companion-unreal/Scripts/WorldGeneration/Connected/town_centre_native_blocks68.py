"""Extend canonical pass-by blocks with the accepted native exterior grammar."""
import importlib.util
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_blocks68_native',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_native_grammar65.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
KIT.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/NativeBlocks68'
KIT.FABRIC.FOCUS=('library-lane-2','library-lane-4','garden-avenue-5','garden-avenue-7','garden-avenue-14')
KIT.FABRIC.NS=KIT.NS
KIT.FABRIC.PREPARED=KIT.OUT/'town-centre-street-pass-68-preflight.json'
KIT.FABRIC.RECEIPT=KIT.OUT/'town-centre-street-pass-68-parcel-frontage.json'
KIT.FABRIC.PREFIX='TC_NativeBlocks68_'
KIT.FABRIC.TAG='Baddie.NativeBlocks68'
KIT.FABRIC.SOURCE_DIRECTORY='native-blocks68-import-source'
KIT.REQUEST=KIT.OUT/'town-centre-street-pass-68-pcg-request.json'
KIT.READBACK=KIT.OUT/'town-centre-street-pass-68-saved-readback.json'
KIT.GRAPH=KIT.NS+'/PCG/PCG_CanonicalExteriorGrammar68'
KIT.SPLINE_TAG='Baddie.NativeBlocks68.Facade'
KIT.GENERATION_TAG='Baddie.NativeBlocks68'
KIT.ANCHOR_SUFFIX='_Anchors68'
KIT.ASSET_READBACK=KIT.OUT/'town-centre-street-pass-68-mesh-spline-readback.json'

prepare=KIT.prepare
execute=KIT.execute
finish=KIT.finish
inspect_saved=KIT.inspect_saved
inspect_asset_contract=KIT.inspect_asset_contract
status=KIT.status
