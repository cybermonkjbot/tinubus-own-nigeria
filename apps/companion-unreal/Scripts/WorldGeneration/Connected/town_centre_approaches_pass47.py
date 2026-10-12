"""Fit road-to-source entrance ramps and omitted roadside slot shoulders."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_approach_aprons',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_aprons_pass38.py')
APRON=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(APRON)
BASE=APRON.BASE
IMPORT=APRON.IMPORT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Approaches47'
FOCUS=('station-road-4','station-road-10','college-lane-10')
TARGET=OUT/'town-centre-street-pass-47-entrance-approaches.json'
PREFIX='TC_Approach47_'
TAG='Baddie.TownCentreApproaches47'
SOURCE_DIRECTORY='approaches47-import-source'
READBACK=OUT/'town-centre-street-pass-47-saved-readback.json'
RETAINED_RECEIPT=OUT/'town-centre-street-pass-46-compounds.json'
SHOULDER_GEOMETRY=None


def plans():
    parcels,roads=BASE.canonical()
    prior=json.loads((OUT/'town-centre-street-pass-39-roadside-corridors.json').read_text())
    ground=json.loads((OUT/'town-centre-street-pass-22-canonical-ground.json').read_text())
    supports={r['label']:r for r in ground['createdActors']}
    result=[]
    for pid in FOCUS:
        parcel=parcels[pid];road=BASE.parcel_road(parcel,roads)
        entrance=BASE.ue_xy_from_source(parcel['entrance'][0],parcel['entrance'][2])
        pose=BASE.nearest_road_pose({'x':parcel['entrance'][0],'z':parcel['entrance'][2]},road)
        direction=BASE.normalize2(BASE.sub2(entrance,pose['point']))
        side=(-direction[1],direction[0])
        start=BASE.add2(pose['point'],BASE.mul2(direction,road['width']*50))
        length=BASE.dist2(start,entrance)
        assert length>470, 'Source entrance too close for the planned raised-pavement profile: '+pid
        slots=[r for r in prior['omittedConflictModules'] if r['reason']==[pid+':entrance']]
        assert slots
        # Complete all uniquely omitted footprint bands, not just the centre strip.
        def height(x,y):
            if x<180:middle=max(0,x*15/180)
            elif x<=286:middle=15
            else:middle=15-16*min(1,max(0,(x-286)/(length-286)))
            blend=min(1,max(0,(abs(y)-90)/70))
            return middle+(15-middle)*blend
        vertices,faces,polygons=[],[],[]
        for slot in slots:
            tokens=slot['sourceId'].split(':');index=int(tokens[2]);step=int(tokens[4]);sign=int(tokens[6]);name=tokens[7];offset=int(tokens[8])
            # Drain and cover share a footprint; one crossing surface replaces both.
            if name=='DrainCover1000':continue
            a,b=[BASE.ue_xy_from_source(*p) for p in road['points'][index:index+2]]
            tangent=BASE.normalize2(BASE.sub2(b,a));right=(-tangent[1],tangent[0])
            centre=BASE.add2(BASE.add2(a,BASE.mul2(tangent,step*100+50)),BASE.mul2(right,sign*(road['width']*50+offset)))
            bounds=prior['moduleBounds'][name]['extent']
            world=[BASE.add2(BASE.add2(centre,BASE.mul2(tangent,u)),BASE.mul2(right,v))
                   for u,v in ((-bounds[1],-bounds[0]),(bounds[1],-bounds[0]),(bounds[1],bounds[0]),(-bounds[1],bounds[0]))]
            poly=[]
            for point in world:
                delta=BASE.sub2(point,start)
                poly.append([sum(delta[i]*direction[i] for i in range(2)),sum(delta[i]*side[i] for i in range(2))])
            for sign_y in (-1,1):
                outside=APRON.clip(poly,1,90*sign_y,sign_y==1)
                # Split at profile changes and blend boundaries before triangulating.
                for lo,hi in ((0,180),(180,286),(286,length)):
                    section=APRON.clip(APRON.clip(outside,0,lo,True),0,hi,False)
                    for near,far in ((90,160),(160,10000)):
                        fitted=APRON.clip(APRON.clip(section,1,near*sign_y,sign_y==1),1,far*sign_y,sign_y==-1)
                        before=len(faces)
                        APRON.add_solid(vertices,faces,fitted,height)
                        if len(faces)>before:polygons.append({'excludedSourceId':slot['sourceId'],'module':name,'localPolygonCm':fitted})
        assert vertices and faces
        result.append({'parcelId':pid,'roadId':road['id'],'sourceEntrance':parcel['entrance'],'startCm':list(start),
                       'direction':list(direction),'lengthCm':length,'clearWidthCm':180,'profile':[[0,0],[180,15],[286,15],[length,-1]],
                       'excludedSlots':slots,'shoulderVertices':vertices,'shoulderFaces':faces,'shoulderPolygons':polygons,
                       'retainedSupport':supports['TC_Ground22_'+pid+'_entrance']})
    return result


def cameras():
    result=[]
    for p in plans():
        yaw=math.degrees(math.atan2(p['direction'][1],p['direction'][0]))
        start=p['startCm']
        result.append({'id':p['parcelId']+'-road-approach','locationCm':BASE.transform_local([*start,0],yaw,[-650,300,165]),
                       'rotation':{'pitch':-5,'yaw':yaw-18,'roll':0},'fov':78})
    return result


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    assert not TARGET.exists()
    planned=plans();actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith(PREFIX) for label in actors)
    for p in planned:
        r=p['retainedSupport'];a=actors[r['label']]
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()),r['locationCm']))<.05
    del a
    actors.clear();BASE.make_level_current(BASE.DETAIL_MAP)
    rows,files=[],{}
    for p in planned:
        pid=p['parcelId'];length=p['lengthCm'];yaw=math.degrees(math.atan2(p['direction'][1],p['direction'][0]))
        sections=[(0,180,0,15,'road-ramp'),(180,286,15,15,'pavement-deck'),(286,length,15,-1,'entrance-ramp')]
        sections.append((0,0,0,0,'shoulders'))
        for index,(a,b,z0,z1,role) in enumerate(sections):
            package=NS+'/Meshes/SM_'+pid.replace('-','_')+'_'+role.replace('-','_')
            geometry=(SHOULDER_GEOMETRY(p) if SHOULDER_GEOMETRY else (p['shoulderVertices'],p['shoulderFaces'])) if role=='shoulders' else None
            mesh,asset=IMPORT.import_section(package,b-a,180,z0,z1,supplied_geometry=geometry,source_directory=SOURCE_DIRECTORY)
            files.update(BASE.package_files(package))
            t={'locationCm':[*BASE.add2(p['startCm'],BASE.mul2(p['direction'],a)),0],
               'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}
            actor=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
            actor.set_actor_label(PREFIX+pid+'_'+role)
            actor.tags=[TAG,'Baddie.Source:'+pid+':'+role,'Baddie.Source:'+p['roadId'],'Baddie.NoRuntimeAuthority']
            c=actor.get_component_by_class(unreal.StaticMeshComponent);c.set_static_mesh(mesh);c.set_material(0,BASE.assets().load_asset(IMPORT.MATERIAL))
            c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
            rows.append({'label':actor.get_actor_label(),'sourceId':pid+':'+role,'parcelId':pid,'roadId':p['roadId'],
                         'role':role,'mesh':package,'materials':[IMPORT.MATERIAL],'asset':asset,'transform':t})
    del actor,c,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    receipt={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
             'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':BASE.district_selection()['source'],
             'plans':planned,'createdActors':rows,'files':files,'cameras':cameras(),'visualAcceptance':'pending',
             'limitations':['Supported static crossing surfaces; no working drainage or excavation',
                            'Fitted concrete shoulders substitute omitted roadside surface footprints, not underground culverts',
                            'No capsule traversal, navigation, accessibility or destination route acceptance']}
    with TARGET.open('x',encoding='utf-8') as stream:json.dump(receipt,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(TARGET),'parcels':len(planned),'actors':len(rows)}))


def inspect_saved(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    receipt=json.loads(TARGET.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    prior=json.loads(RETAINED_RECEIPT.read_text())
    for r in receipt['createdActors']+prior['createdActors']:
        a=actors[r['label']];t=a.get_actor_transform();c=a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),r['transform']['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-r['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d))<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==r['mesh']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(r['materials']))]==r['materials']
        assert str(c.get_collision_profile_name())=='BlockAll'
    for p in receipt['plans']:assert p['retainedSupport']['label'] in actors
    for path,digest in receipt['files'].items():assert BASE.sha(ROOT/path)==digest
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':receipt['files'],
            'safeReloadCompleted':True,'newApproachActors':len(receipt['createdActors']),'retainedCompoundActors':len(prior['createdActors']),
            'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
