"""Refine curved crossing shoulders without moving the saved edge/profile contract."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_shoulder_previous',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_approaches_pass47.py')
PRIOR=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(PRIOR)
BASE=PRIOR.BASE
IMPORT=PRIOR.IMPORT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Shoulders48'
TARGET=OUT/'town-centre-street-pass-48-refined-shoulders.json'
READBACK=OUT/'town-centre-street-pass-48-saved-readback.json'
PREFIX='TC_Shoulder48_'
TAG='Baddie.TownCentreShoulders48'
OLD_TAG='Baddie.TownCentreApproaches47'
SOURCE_DIRECTORY='shoulders48-import-source'


def geometry(plan):
    length=plan['lengthCm']
    def height(x,y):
        if x<180:middle=max(0,x*15/180)
        elif x<=286:middle=15
        else:middle=15-16*min(1,max(0,(x-286)/(length-286)))
        blend=min(1,max(0,(abs(y)-90)/70))
        return middle+(15-middle)*blend
    vertices,faces=[],[]
    for row in plan['shoulderPolygons']:
        poly=row['localPolygonCm']
        for x in range(math.floor(min(p[0] for p in poly)/15)*15,math.ceil(max(p[0] for p in poly)/15)*15,15):
            strip=PRIOR.APRON.clip(PRIOR.APRON.clip(poly,0,x,True),0,x+15,False)
            if len(strip)<3:continue
            for y in range(math.floor(min(p[1] for p in strip)/15)*15,math.ceil(max(p[1] for p in strip)/15)*15,15):
                cell=PRIOR.APRON.clip(PRIOR.APRON.clip(strip,1,y,True),1,y+15,False)
                if len(cell)<3:continue
                # Interior cells supply top triangles only, not buried internal walls.
                area=sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(cell,cell[1:]+cell[:1]))
                if abs(area)<.001:continue
                if area<0:cell.reverse()
                offset=len(vertices)
                vertices.extend([[a,b,height(a,b)] for a,b in cell])
                faces.extend([[offset,offset+i,offset+i+1] for i in range(1,len(cell)-1)])
        # Preserve exterior side support; do not average full-height walls into every cell.
        area=sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1]))
        boundary=poly if area>0 else list(reversed(poly))
        for a,b in zip(boundary,boundary[1:]+boundary[:1]):
            count=max(1,math.ceil(math.dist(a,b)/15))
            for i in range(count):
                p=[a[k]+(b[k]-a[k])*i/count for k in range(2)]
                q=[a[k]+(b[k]-a[k])*(i+1)/count for k in range(2)]
                offset=len(vertices)
                vertices.extend([[*p,-12],[*q,-12],[*p,height(*p)],[*q,height(*q)]])
                faces.extend([[offset,offset+1,offset+3],[offset,offset+3,offset+2]])
        offset=len(vertices)
        vertices.extend([[a,b,-12] for a,b in boundary])
        faces.extend([[offset,offset+i+1,offset+i] for i in range(1,len(boundary)-1)])
    assert vertices and faces
    return vertices,faces


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    assert not TARGET.exists() and not READBACK.exists()
    prior=json.loads(PRIOR.TARGET.read_text())
    old={r['parcelId']:r for r in prior['createdActors'] if r.get('role')=='shoulders' or r['sourceId'].endswith(':shoulders')}
    assert len(old)==len(prior['plans']) and old
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for r in old.values():
        a=actors[r['label']];c=a.get_component_by_class(unreal.StaticMeshComponent);t=a.get_actor_transform()
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert OLD_TAG in [str(tag) for tag in a.tags]
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==r['mesh']
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),r['transform']['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-r['transform']['rotation']['yaw']+180)%360-180)<.001
    del a,c,t
    BASE.make_level_current(BASE.DETAIL_MAP)
    rows,files,planned=[],{},[]
    for plan in prior['plans']:
        pid=plan['parcelId'];r=old[pid]
        vertices,faces=geometry(plan)
        package=NS+'/Meshes/SM_'+pid.replace('-','_')+'_shoulders'
        mesh,asset=IMPORT.import_section(package,0,0,0,0,supplied_geometry=(vertices,faces),source_directory=SOURCE_DIRECTORY)
        files.update(BASE.package_files(package))
        rows.append({**r,'label':PREFIX+pid,'mesh':package,'asset':asset})
        planned.append({'parcelId':pid,'roadId':plan['roadId'],'profile':plan['profile'],
                        'footprintSource':PRIOR.TARGET.name,
                        'polygonCount':len(plan['shoulderPolygons']),'gridStepCm':15,'vertices':len(vertices),'triangles':len(faces)})
    for r in old.values():assert BASE.actors().destroy_actor(actors[r['label']])
    actors.clear()
    for r in rows:
        t=r['transform']
        a=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        a.set_actor_label(r['label']);a.tags=[TAG,'Baddie.Source:'+r['sourceId'],'Baddie.NoRuntimeAuthority']
        c=a.get_component_by_class(unreal.StaticMeshComponent);c.set_static_mesh(BASE.assets().load_asset(r['mesh']))
        for i,path in enumerate(r['materials']):c.set_material(i,BASE.assets().load_asset(path))
        c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
    del a,c,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    receipt={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
             'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],'plans':planned,
             'createdActors':rows,'removedActors':list(old.values()),'files':files,'cameras':prior.get('cameras',prior.get('previewPoses',[])),
             'visualAcceptance':'pending','limitations':['Same bilinear profile/footprint with 15cm tessellation; edge normals and finish still require visual review',
                            'Static support only; no drainage excavation or runtime movement acceptance']}
    with TARGET.open('x',encoding='utf-8') as stream:json.dump(receipt,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(TARGET),'replacedShoulders':len(rows)}))


def inspect_saved(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    receipt=json.loads(TARGET.read_text());prior=json.loads(PRIOR.TARGET.read_text())
    removed={r['label'] for r in receipt['removedActors']}
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert not removed.intersection(actors)
    retained=[r for r in prior['createdActors'] if r['label'] not in removed]+prior.get('retainedApartmentActors',[])
    uv_probes=0
    for r in receipt['createdActors']+retained:
        a=actors[r['label']];t=a.get_actor_transform();c=a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),r['transform']['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-r['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d))<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==r['mesh']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(r['materials']))]==r['materials']
        assert str(c.get_collision_profile_name())=='BlockAll'
        if r in receipt['createdActors']:
            assert BASE.sha(ROOT/r['asset']['sourceObj'])==r['asset']['sourceObjSha256']
            mesh=c.get_editor_property('static_mesh')
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            d=mesh.get_static_mesh_description(0)
            for i in range(d.get_vertex_instance_count()):
                vi=unreal.VertexInstanceID(i)
                if not d.is_vertex_instance_valid(vi):continue
                p=d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
                assert abs(uv.x-p.x/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
                uv_probes+=1
    for path,digest in receipt['files'].items():assert BASE.sha(ROOT/path)==digest
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':receipt['files'],'safeReloadCompleted':True,
            'replacedShoulderActors':len(receipt['createdActors']),'retainedRampActors':len(retained),'uvProbes':uv_probes,
            'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
