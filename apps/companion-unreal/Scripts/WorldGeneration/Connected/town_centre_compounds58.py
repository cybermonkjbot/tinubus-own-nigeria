"""Measured Old Town Lane boundaries preserving canonical entrance paths."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_compounds58_base',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_compounds_pass46.py')
COMPOUND=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(COMPOUND)
COMPOUND.FOCUS=('old-town-lane-3','old-town-lane-4','old-town-lane-7')
COMPOUND.SHELL_RECEIPT=COMPOUND.OUT/'town-centre-street-pass-57-parcel-frontage.json'
COMPOUND.ENTRANCE_RECEIPT=None
COMPOUND.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds58'
COMPOUND.TARGET=COMPOUND.OUT/'town-centre-street-pass-58-compounds.json'
COMPOUND.READBACK=COMPOUND.OUT/'town-centre-street-pass-58-saved-readback.json'
COMPOUND.PREFIX='TC_Compound58_'
COMPOUND.TAG='Baddie.TownCentreCompounds58'
COMPOUND.SOURCE_DIRECTORY='compounds58-import-source'
COMPOUND.GATE_WIDTHS={'old-town-lane-3':500,'old-town-lane-4':280,'old-town-lane-7':280}
COMPOUND.GATE_MODE='sliding'


def prepare(pid):
    COMPOUND.BASE.ensure_identity(pid)
    assert COMPOUND.BASE.active_world_package()==COMPOUND.BASE.TARGET_MAP and not COMPOUND.BASE.dirty_names()
    data={'mapPackage':COMPOUND.BASE.TARGET_MAP,'detailLevelPackage':COMPOUND.BASE.DETAIL_MAP,
          'plans':COMPOUND.plans(),'previewPoses':COMPOUND.cameras(),
          'deferredParcels':{'old-town-lane-6':'Shell/front clearance below40cm; needs a different boundary layout',
                            'old-town-lane-10':'Shell/front clearance below40cm; needs a different boundary layout'}}
    with (COMPOUND.OUT/'town-centre-street-pass-58-preflight.json').open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')
    print('Three source-fitted Old Town Lane compound plans prepared')


execute=COMPOUND.execute
inspect_saved=COMPOUND.inspect_saved


def inspect_mesh_contract(pid):
    COMPOUND.BASE.ensure_identity(pid)
    assert not COMPOUND.BASE.dirty_names()
    data=json.loads(COMPOUND.TARGET.read_text())
    probes=0
    for row in data['createdActors']:
        asset=row['asset']
        assert COMPOUND.BASE.sha(ROOT/asset['sourceObj'])==asset['sourceObjSha256']
        mesh=COMPOUND.BASE.assets().load_asset(row['mesh'])
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==COMPOUND.unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        desc=mesh.get_static_mesh_description(0)
        for i in range(desc.get_vertex_instance_count()):
            vi=COMPOUND.unreal.VertexInstanceID(i)
            if not desc.is_vertex_instance_valid(vi):continue
            p=desc.get_vertex_position(desc.get_vertex_instance_vertex(vi));uv=desc.get_vertex_instance_uv(vi,0)
            assert abs(uv.x-(p.x+p.z)/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
            probes+=1
    result={'pid':pid,'verifiedAssetCount':len(data['createdActors']),'uvProbes':probes,
            'sourceHashesMatched':True,'staticTriangleCollisionMatched':True,'runtimeTraversal':'not-inspected'}
    with (COMPOUND.OUT/'town-centre-street-pass-58-mesh-readback.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
