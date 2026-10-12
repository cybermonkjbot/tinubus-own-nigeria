"""Parcel-fitted Arcade Crescent boundaries preserving recessed portal clearances."""
import importlib.util
import json
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_arcade51_compounds',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_compounds_pass46.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
KIT.FOCUS=('arcade-crescent-7','arcade-crescent-9')
KIT.NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/ArcadeCompounds51'
KIT.TARGET=KIT.OUT/'town-centre-street-pass-51-compounds.json'
KIT.SHELL_RECEIPT=KIT.OUT/'town-centre-street-pass-50-parcel-frontage.json'
KIT.ENTRANCE_RECEIPT=None
KIT.PREFIX='TC_ArcadeCompound51_'
KIT.TAG='Baddie.TownCentreArcadeCompounds51'
KIT.SOURCE_DIRECTORY='arcade-compounds51-import-source'
KIT.READBACK=KIT.OUT/'town-centre-street-pass-51-saved-readback.json'
KIT.GATE_WIDTHS={'arcade-crescent-9':720}
KIT.FRONT_CLEARANCE=80
PREPARED=KIT.OUT/'town-centre-street-pass-51-preflight.json'


def prepare(pid):
    KIT.BASE.ensure_identity(pid)
    assert KIT.BASE.active_world_package()==KIT.BASE.TARGET_MAP and not KIT.BASE.dirty_names()
    assert not PREPARED.exists() and not KIT.TARGET.exists()
    data={'mapPackage':KIT.BASE.TARGET_MAP,'detailLevelPackage':KIT.BASE.DETAIL_MAP,
          'plans':KIT.plans(),'previewPoses':KIT.cameras(),'source':KIT.BASE.district_selection()['source']}
    with PREPARED.open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')
    print(json.dumps({'preflight':str(PREPARED),'gateWidths':KIT.GATE_WIDTHS}))


def execute(pid):
    assert json.loads(PREPARED.read_text())['plans']==KIT.plans()
    KIT.execute(pid)


def inspect_saved(pid):
    KIT.inspect_saved(pid)


def inspect_mesh_contract(pid):
    KIT.BASE.ensure_identity(pid)
    assert not KIT.BASE.dirty_names()
    data=json.loads(KIT.TARGET.read_text())
    probes=0
    for row in data['createdActors']:
        asset=row['asset']
        assert KIT.BASE.sha(ROOT/asset['sourceObj'])==asset['sourceObjSha256']
        mesh=KIT.BASE.assets().load_asset(row['mesh'])
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==KIT.unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        desc=mesh.get_static_mesh_description(0)
        for i in range(desc.get_vertex_instance_count()):
            vi=KIT.unreal.VertexInstanceID(i)
            if not desc.is_vertex_instance_valid(vi):continue
            p=desc.get_vertex_position(desc.get_vertex_instance_vertex(vi));uv=desc.get_vertex_instance_uv(vi,0)
            assert abs(uv.x-(p.x+p.z)/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
            probes+=1
    result={'pid':pid,'mapPackage':KIT.BASE.TARGET_MAP,'verifiedAssetCount':len(data['createdActors']),
            'uvProbes':probes,'sourceHashesMatched':True,'staticTriangleCollisionMatched':True,
            'gateClearWidthsCm':{p['parcelId']:p['gateClearWidthCm'] for p in data['plans']},
            'runtimeTraversal':'not-inspected'}
    with (KIT.OUT/'town-centre-street-pass-51-mesh-readback.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
