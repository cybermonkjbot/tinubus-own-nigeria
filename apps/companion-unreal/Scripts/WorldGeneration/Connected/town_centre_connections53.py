"""Fit Arcade apartment entrance approaches and measured facade seam closures."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_connections53_approach',SCRIPTS/'town_centre_approaches_pass47.py')
APPROACH=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(APPROACH)
SPEC=importlib.util.spec_from_file_location('tc_connections53_fabric',SCRIPTS/'town_centre_apartments52.py')
FABRIC=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
BASE=APPROACH.BASE
IMPORT=APPROACH.IMPORT
APPROACH.FOCUS=FABRIC.FABRIC.FOCUS
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Connections53'
RECEIPT=OUT/'town-centre-street-pass-53-connections.json'
READBACK=OUT/'town-centre-street-pass-53-saved-readback.json'
PREFIX='TC_Connections53_'


def prepare(pid):
    FABRIC.FABRIC.guard(pid)
    result={'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
        'plans':APPROACH.plans(),'previewPoses':APPROACH.cameras()}
    with (OUT/'town-centre-street-pass-53-preflight.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print('Connection preflight prepared')


def execute(pid):
    FABRIC.FABRIC.guard(pid)
    assert not RECEIPT.exists() and not READBACK.exists()
    prior=json.loads(FABRIC.FABRIC.RECEIPT.read_text())
    plans=APPROACH.plans()
    assert plans==json.loads((OUT/'town-centre-street-pass-53-preflight.json').read_text())['plans']
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith(PREFIX) for label in actors)
    for row in prior['createdActors']:
        assert row['label'] in actors
    for p in plans:
        a=actors[p['retainedSupport']['label']]
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()),p['retainedSupport']['locationCm']))<.05
    actors.clear();del a
    # The native bay sits at X 3.5..28.5cm; the older 24cm posts did not seal its depth.
    mesh=BASE.assets().load_asset(BASE.v3_modules()['ResidentialBay']['mesh'])
    bounds=mesh.get_bounding_box()
    assert abs(bounds.min.x-3.5)<.01 and abs(bounds.max.x-28.5)<.01
    assert abs(bounds.min.y+378)<.01 and abs(bounds.max.y+12)<.01
    del mesh,bounds
    BASE.make_level_current(BASE.DETAIL_MAP)
    rows,files=[],{}

    def add(parcel,role,geometry,location,yaw,vertical=False):
        package=NS+'/Meshes/SM_'+parcel.replace('-','_')+'_'+role.replace('-','_')
        mesh,asset=IMPORT.import_section(package,0,0,0,0,supplied_geometry=geometry,
            source_directory='connections53-import-source',vertical_uv=vertical)
        files.update(BASE.package_files(package))
        t={'locationCm':location,'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}
        a=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location),BASE.rotator(t['rotation']))
        a.set_actor_label(PREFIX+parcel+'_'+role)
        a.tags=['Baddie.TownCentreConnections53','Baddie.Source:'+parcel+':'+role,'Baddie.NoRuntimeAuthority']
        c=a.get_component_by_class(unreal.StaticMeshComponent);c.set_static_mesh(mesh)
        c.set_material(0,BASE.assets().load_asset(IMPORT.MATERIAL))
        c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
        rows.append({'label':a.get_actor_label(),'parcelId':parcel,'sourceId':parcel+':'+role,
            'mesh':package,'asset':asset,'materials':[IMPORT.MATERIAL],'transform':t})

    for p in plans:
        yaw=math.degrees(math.atan2(p['direction'][1],p['direction'][0]))
        for start,end,z0,z1,role in ((0,180,0,15,'road-ramp'),(180,286,15,15,'pavement-deck'),
                                   (286,p['lengthCm'],15,-1,'entrance-ramp')):
            add(p['parcelId'],role,IMPORT.geometry(end-start,180,z0,z1),
                [*BASE.add2(p['startCm'],BASE.mul2(p['direction'],start)),0],yaw)
        add(p['parcelId'],'shoulders',(p['shoulderVertices'],p['shoulderFaces']),[*p['startCm'],0],yaw)
    for p in prior['plans']:
        vertices,faces=[],[]
        width,depth,height=p['widthCm'],p['depthCm'],p['floors']*300
        box=FABRIC.FABRIC.KIT.box
        for x in range(0,depth+1,390):
            for y in (-width/2,width/2):
                box(vertices,faces,x-16,x+16,y-30,y+30,0,height)
        for y in range(-width//2,width//2+1,390):
            for x in (0,depth):
                box(vertices,faces,x-30,x+30,y-16,y+16,0,height)
        add(p['parcelId'],'frame-depth-closures',(vertices,faces),p['originCm'],p['yaw'],True)
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
        'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
        'plans':plans,'createdActors':rows,'retainedApartmentActors':prior['createdActors'],'files':files,
        'previewPoses':prior['previewPoses']+APPROACH.cameras(),
        'visualAcceptance':'pending matched image inspection','runtimeTraversal':'not-inspected',
        'limitations':['Static support surfaces, not working drainage or admitted movement',
            'Forecourt finishes, outfalls, varied facade art and compounds incomplete']}
    with RECEIPT.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(RECEIPT),'newActors':len(rows),'retainedApartments':len(prior['createdActors'])}))


def inspect_saved(pid):
    FABRIC.FABRIC.guard(pid)
    data=json.loads(RECEIPT.read_text())
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    probes=0
    for row in data['createdActors']+data['retainedApartmentActors']:
        a=actors[row['label']];c=a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        t=a.get_actor_transform();expected=row['transform']
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),expected['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-expected['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d))<.001
        mesh=c.get_editor_property('static_mesh')
        assert mesh.get_path_name().split('.')[0]==row['mesh']
        assert str(c.get_collision_profile_name())=='BlockAll'
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(row['materials']))]==row['materials']
        if row in data['createdActors']:
            asset=row['asset'];assert BASE.sha(ROOT/asset['sourceObj'])==asset['sourceObjSha256']
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            d=mesh.get_static_mesh_description(0);vertical='(Xcm+Zcm)' in asset['nativeUVBasis']
            for i in range(d.get_vertex_instance_count()):
                vi=unreal.VertexInstanceID(i)
                if not d.is_vertex_instance_valid(vi):continue
                p=d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
                assert abs(uv.x-(p.x+(p.z if vertical else 0))/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
                probes+=1
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
        'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
        'safeReloadCompleted':True,'newActors':len(data['createdActors']),
        'retainedApartmentActors':len(data['retainedApartmentActors']),'uvProbes':probes,
        'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'readback':str(READBACK),'uvProbes':probes}))
