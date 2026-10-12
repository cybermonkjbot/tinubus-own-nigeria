"""Source-preserving Old Town Lane approach surfaces and refined shoulders."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_approaches59_base',SCRIPTS/'town_centre_approaches_pass47.py')
APPROACH=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(APPROACH)
SPEC=importlib.util.spec_from_file_location('tc_approaches59_refinement',SCRIPTS/'town_centre_shoulders_pass48.py')
REFINE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(REFINE)
APPROACH.FOCUS=('old-town-lane-4','old-town-lane-6','old-town-lane-7','old-town-lane-10')
APPROACH.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Approaches59'
APPROACH.TARGET=APPROACH.OUT/'town-centre-street-pass-59-entrance-approaches.json'
APPROACH.READBACK=APPROACH.OUT/'town-centre-street-pass-59-saved-readback.json'
APPROACH.RETAINED_RECEIPT=APPROACH.OUT/'town-centre-street-pass-58-compounds.json'
APPROACH.PREFIX='TC_Approach59_'
APPROACH.TAG='Baddie.TownCentreApproaches59'
APPROACH.SOURCE_DIRECTORY='approaches59-import-source'
APPROACH.SHOULDER_GEOMETRY=REFINE.geometry
PREPARED=APPROACH.OUT/'town-centre-street-pass-59-preflight.json'


def prepare(pid):
    APPROACH.BASE.ensure_identity(pid)
    assert APPROACH.BASE.active_world_package()==APPROACH.BASE.TARGET_MAP and not APPROACH.BASE.dirty_names()
    data={'mapPackage':APPROACH.BASE.TARGET_MAP,'detailLevelPackage':APPROACH.BASE.DETAIL_MAP,
          'plans':APPROACH.plans(),'previewPoses':APPROACH.cameras(),
          'deferredParcels':{'old-town-lane-3':'Source entrance too close for existing raised-pavement profile; needs short transition'}}
    with PREPARED.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print('Four source-preserving approach plans prepared')


def execute(pid):
    assert json.loads(PREPARED.read_text())['plans']==APPROACH.plans()
    APPROACH.execute(pid)


inspect_saved=APPROACH.inspect_saved


def inspect_mesh_contract(pid,target=None):
    APPROACH.BASE.ensure_identity(pid)
    assert not APPROACH.BASE.dirty_names()
    data=json.loads(APPROACH.TARGET.read_text());probes=0
    for row in data['createdActors']:
        assert APPROACH.BASE.sha(ROOT/row['asset']['sourceObj'])==row['asset']['sourceObjSha256']
        mesh=APPROACH.BASE.assets().load_asset(row['mesh'])
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==APPROACH.unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        desc=mesh.get_static_mesh_description(0)
        for i in range(desc.get_vertex_instance_count()):
            vi=APPROACH.unreal.VertexInstanceID(i)
            if not desc.is_vertex_instance_valid(vi):continue
            p=desc.get_vertex_position(desc.get_vertex_instance_vertex(vi));uv=desc.get_vertex_instance_uv(vi,0)
            assert abs(uv.x-p.x/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
            probes+=1
    result={'pid':pid,'verifiedAssetCount':len(data['createdActors']),'uvProbes':probes,
            'sourceHashesMatched':True,'staticTriangleCollisionMatched':True,'shoulderGridStepCm':15,'runtimeTraversal':'not-inspected'}
    with (target or APPROACH.OUT/'town-centre-street-pass-59-mesh-readback.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
