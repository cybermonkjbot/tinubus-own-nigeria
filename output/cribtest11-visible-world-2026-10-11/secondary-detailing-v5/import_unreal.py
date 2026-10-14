"""Asset-only wrapper; preserve the native owner's shared importer edits."""
import json
import runpy
from pathlib import Path

import unreal

ROOT=Path(__file__).resolve().parent
SHARED_IMPORTER=ROOT.parent/'secondary-detailing-v1/import_unreal.py'


def import_batch(batch,destination='/Game/LagosArt/SecondaryDetailingV5',
                 shared_atlas_destination=None,asset_ids=None):
    namespace=runpy.run_path(str(SHARED_IMPORTER))
    globals_=namespace['import_batch'].__globals__
    globals_['ROOT']=ROOT
    if shared_atlas_destination is not None:
        # Explicit opt-in to already-imported, matching textures. Never update
        # another owner's asset or rely on a name without provenance.
        source_records=json.loads((ROOT/'manifest.json').read_text())['textures']
        shared={}
        for record in source_records:
            name=Path(record['file']).stem
            asset=unreal.load_asset(shared_atlas_destination+'/Textures/'+name)
            if not isinstance(asset,unreal.Texture2D):
                raise RuntimeError('Shared atlas Texture2D missing: '+name)
            if unreal.EditorAssetLibrary.get_metadata_tag(asset,'LagosKitSourceSHA256')!=record['sha256']:
                raise RuntimeError('Shared atlas source hash mismatch: '+name)
            if asset.get_editor_property('srgb')!=record['srgb']:
                raise RuntimeError('Shared atlas color-space mismatch: '+name)
            shared[name]=asset

        def reuse_texture(record,destination,receipt):
            asset=shared[Path(record['file']).stem]
            receipt.setdefault('reused_textures',[]).append(asset.get_path_name())
            return asset

        globals_['texture']=reuse_texture
    return namespace['import_batch'](batch,destination=destination,asset_ids=asset_ids)
