"""Reuse asset-only import for waterfront accessories and lighting fixtures."""
import runpy
from pathlib import Path

ROOT=Path(__file__).resolve().parent
_shared=runpy.run_path(str(ROOT.parent/'secondary-detailing-v1/import_unreal.py'))
_shared['import_batch'].__globals__['ROOT']=ROOT


def import_batch(batch,destination='/Game/LagosArt/SecondaryDetailingV4'):
    return _shared['import_batch'](batch,destination=destination)
