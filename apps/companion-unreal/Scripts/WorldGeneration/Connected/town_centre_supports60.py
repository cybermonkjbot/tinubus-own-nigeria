"""Replace oversized retained entrance support with carriageway-edge slabs."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_supports60_kit',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_approaches59.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
BASE=KIT.APPROACH.BASE
IMPORT=KIT.APPROACH.IMPORT
OUT=KIT.APPROACH.OUT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Supports60'
RECEIPT=OUT/'town-centre-street-pass-60-support-repair.json'
READBACK=OUT/'town-centre-street-pass-60-saved-readback.json'
PREPARED=OUT/'town-centre-street-pass-60-preflight.json'


def prepare(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    prior=json.loads(KIT.APPROACH.TARGET.read_text())
    data={'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
          'plans':prior['plans'],'previewPoses':json.loads(KIT.PREPARED.read_text())['previewPoses']}
    with PREPARED.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print('Four retained support replacements prepared')


def execute(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names() and not RECEIPT.exists()
    data=json.loads(PREPARED.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for p in data['plans']:
        r=p['retainedSupport'];a=actors[r['label']];c=a.get_component_by_class(KIT.APPROACH.unreal.StaticMeshComponent)
        assert 'Baddie.TownCentreGround22' in [str(t) for t in a.tags]
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==r['mesh']
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()),r['locationCm']))<.05
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_scale3d()),r['scale']))<.001
    del a,c
    rows,files=[],{}
    for p in data['plans']:
        package=NS+'/Meshes/SM_'+p['parcelId'].replace('-','_')+'_edge_support'
        mesh,asset=IMPORT.import_section(package,p['lengthCm'],180,-1,-1,source_directory='supports60-import-source')
        files.update(BASE.package_files(package))
        r=p['retainedSupport'];a=actors[r['label']];c=a.get_component_by_class(KIT.APPROACH.unreal.StaticMeshComponent)
        yaw=math.degrees(math.atan2(p['direction'][1],p['direction'][0]))
        t={'locationCm':[*p['startCm'],0],'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}
        c.set_static_mesh(mesh);c.set_material(0,BASE.assets().load_asset(IMPORT.MATERIAL))
        a.set_actor_location(KIT.APPROACH.unreal.Vector(*t['locationCm']),False,False)
        a.set_actor_rotation(BASE.rotator(t['rotation']),False)
        a.set_actor_scale3d(KIT.APPROACH.unreal.Vector(1,1,1))
        a.tags=list(a.tags)+['Baddie.TownCentreSupports60','Baddie.NoRuntimeAuthority']
        c.set_collision_profile_name('BlockAll');c.set_mobility(KIT.APPROACH.unreal.ComponentMobility.STATIC)
        rows.append({'label':r['label'],'parcelId':p['parcelId'],'sourceId':p['parcelId']+':entrance-support',
                     'mesh':package,'materials':[IMPORT.MATERIAL],'transform':t,'asset':asset,'supersededSupport':r})
    del a,c,mesh;actors.clear()
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert KIT.APPROACH.unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert KIT.APPROACH.unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':BASE.district_selection()['source'],
            'plans':data['plans'],'createdActors':rows,'files':files,
            'limitations':['Four existing actors updated; prior Ground22 transform records superseded only for these labels',
                           'Static support only; road surface retained, no gameplay or drainage claim']}
    with RECEIPT.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print('Four edge-fitted support actors saved')


def inspect_saved(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    data=json.loads(RECEIPT.read_text());prior=json.loads(KIT.APPROACH.TARGET.read_text())
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()};probes=0
    for r in data['createdActors']+prior['createdActors']:
        a=actors[r['label']];c=a.get_component_by_class(KIT.APPROACH.unreal.StaticMeshComponent);t=r['transform']
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()),t['locationCm']))<.05
        assert abs((a.get_actor_rotation().yaw-t['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(a.get_actor_scale3d()))<.001
        mesh=c.get_editor_property('static_mesh');assert mesh.get_path_name().split('.')[0]==r['mesh']
        assert str(c.get_collision_profile_name())=='BlockAll'
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(r['materials']))]==r['materials']
        if r in data['createdActors']:
            assert BASE.sha(ROOT/r['asset']['sourceObj'])==r['asset']['sourceObjSha256']
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==KIT.APPROACH.unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            d=mesh.get_static_mesh_description(0)
            for i in range(d.get_vertex_instance_count()):
                vi=KIT.APPROACH.unreal.VertexInstanceID(i)
                if not d.is_vertex_instance_valid(vi):continue
                p=d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
                assert abs(uv.x-p.x/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
                probes+=1
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    result={'pid':pid,'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
            'safeReloadCompleted':True,'updatedSupportActors':4,'retainedApproachActors':16,'uvProbes':probes,
            'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'updatedSupports':4,'retainedApproaches':16,'uvProbes':probes}))
